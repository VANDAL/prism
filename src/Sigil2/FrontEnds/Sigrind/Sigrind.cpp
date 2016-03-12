#include <iterator>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <memory>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <cerrno>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "Sigil2/FrontEnds.hpp"
#include "Sigil2/EventManager.hpp" 
#include "Sigil2/InstrumentationIface.h" 
#include "Sigrind.hpp"
#include "spdlog.h"
#include "whereami.h"
#include "elfio/elfio.hpp"

/* Sigil2's Valgrind frontend forks Valgrind off as a separate process;
 * Valgrind sends the frontend dynamic events from the application via shared memory */

//TODO got a little carried away with throwing errors...

namespace sgl
{

using namespace std;

Sigrind::Sigrind(string tmp_dir) 
	: shmem_file(tmp_dir + "/" + SIGRIND_SHMEM_NAME)
	, empty_file(tmp_dir + "/" + SIGRIND_EMPTYFIFO_NAME)
	, full_file(tmp_dir + "/" + SIGRIND_FULLFIFO_NAME)

{
	initShMem();
	makeNewFifo(empty_file.c_str());
	makeNewFifo(full_file.c_str());
}

Sigrind::~Sigrind()
{
	/* disconnect from Valgrind */
	munmap(shared_mem, sizeof(SigrindSharedData));
	close(emptyfd);
	close(fullfd);

	/* file cleanup */
	if ( remove(shmem_file.c_str()) != 0 ||
			remove(empty_file.c_str()) != 0 ||
			remove(full_file.c_str()) != 0)
	{
		perror("deleting IPC files");
	}
}

void Sigrind::initShMem()
{
	unique_ptr<SigrindSharedData> init(new SigrindSharedData());

	FILE *fd = fopen(shmem_file.c_str(), "wb+");
	if ( fd == nullptr )
	{
		perror("shared memory initialization");
		throw runtime_error("Sigrind shared memory file open failed");
	}

	/* XXX From write(2) man pages:
	 *
	 * On Linux, write() (and similar system calls) will transfer at most
	 * 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
	 * actually transferred.  (This is true on both 32-bit and 64-bit
	 * systems.) 
	 *
	 * fwrite doesn't have this limitation */
	int count = fwrite(init.get(),sizeof(SigrindSharedData), 1, fd);
	if ( count != 1 )
	{
		perror("shared memory initialization");
		fclose(fd);
		throw runtime_error("Sigrind shared memory file write failed");
	}	

	shared_mem = reinterpret_cast<SigrindSharedData*>
		(mmap(nullptr, sizeof(SigrindSharedData), PROT_READ|PROT_WRITE, MAP_SHARED, fileno(fd), 0));
	if (shared_mem == (void*) -1)
	{
		perror("shared memory initialization");
		fclose(fd);
		throw runtime_error("Sigrind mmap shared memory failed");
	}
	fclose(fd);
}

void Sigrind::makeNewFifo(const char* path) const
{
	if ( mkfifo(path, 0600) < 0 )
	{
		if (errno == EEXIST)
		{
			if ( remove(path) != 0 )
			{
				perror(path);
				throw runtime_error("Sigil2 could not delete old fifos");
			}
			if ( mkfifo(path, 0600) < 0 )
			{
				perror("mkfifo");
				throw runtime_error("Sigil2 failed to create Valgrind fifos");
			}
		}
		else
		{
			perror("mkfifo");
			throw runtime_error("Sigil2 failed to create Valgrind fifos");
		}
	}
}

void Sigrind::connectValgrind()
{
	int tries = 0;
	do 
	{
		emptyfd = open(empty_file.c_str(), O_WRONLY|O_NONBLOCK);

		if (emptyfd < 0) 
		{
			this_thread::sleep_for(chrono::milliseconds(500));
		}
		else /* connected */ 
		{
			break;
		}

		++tries;
	} 
	while (tries < 4);

	if (tries == 4 || emptyfd < 0)
	{
		throw runtime_error("Sigil2 failed to connect to Valgrind");
	}

	/* XXX Sigil might get stuck blocking if Valgrind
	 * unexpectedly exits before connecting at this point */
	fullfd = open(full_file.c_str(), O_RDONLY);

	if (fullfd < 0)
	{
		perror("open fifo");
		throw runtime_error("Sigil2 failed to open Valgrind fifos");
	}
}

void Sigrind::produceSigrindEvents()
{
	/* Valgrind should have started by now */
	connectValgrind();

	while (finished == false)
	{
		/* Valgrind sends event buffer metadata */
		unsigned int from_valgrind = readFullFifo();

		unsigned int idx;
		unsigned int used;
		if (from_valgrind == SIGRIND_FINISHED)
		{
			/* Valgrind finished;
			 * partial leftover buffer */
			finished = true;
			idx = readFullFifo();
			used = readFullFifo();
		}
		else
		{
			/* full buffer */
			idx = from_valgrind;
			used = SIGRIND_BUFSIZE;
		}

		produceFromBuffer(idx, used);

		/* tell Valgrind that the buffer is empty again */
		writeEmptyFifo(idx);
	}
}

int Sigrind::readFullFifo()
{
	int res = read(fullfd, &full_data, sizeof(full_data));

	if (res == 0)
	{
		throw runtime_error("Unexpected end of fifo");
	}
	else if (res < 0)
	{
		perror("Reading from full fifo");
		throw runtime_error("Could not read from Valgrind full fifo");
	}

	return full_data;
}

void Sigrind::writeEmptyFifo(unsigned int idx)
{
	if (write(emptyfd, &idx, sizeof(idx)) < 0)
	{
		perror("write Empty");
		throw runtime_error("Could not send Valgrind empty buffer status");
	}
}

void Sigrind::produceFromBuffer(unsigned int idx, unsigned int used)
{
	assert (idx < SIGRIND_BUFNUM);

	BufferedSglEv (&buf)[SIGRIND_BUFSIZE] = shared_mem->buf[idx];

	for (unsigned int i=0; i<used; ++i)
	{
		switch(buf[i].tag)
		{
		case SGL_MEM_TAG:
			SGLnotifyMem(buf[i].mem);
			break;
		case SGL_COMP_TAG:
			SGLnotifyComp(buf[i].comp);
			break;
		case SGL_SYNC_TAG:
			SGLnotifySync(buf[i].sync);
			break;
		default:
			throw runtime_error("Received unhandled event in Sigrind");
			break;
		}
	}
}

namespace
{
char* const* tokenizeOpts (
		const vector<string> &user_exec,
		const vector<string> &args,
		const string &tmp_dir)
{
	assert( !tmp_dir.empty() );

	/* Check that the binary was compiled with GCC 4.9.2. */
	/* Naively assume the first option is the user binary.
	 * ML: KS says that OpenMP is only guaranteed to work for
	 * GCC 4.9.2. Pthreads should work for most recent GCC
	 * releases. Cannot check if file exists because it is
	 * not guaranteed that this string is actually the binary */
	ELFIO::elfio reader;
	bool is_gcc_compatible = false;
	string gcc_version_needed("4.9.2");
	string gcc_version_found;
	if (reader.load(user_exec[0]) != 0)
	{
		ELFIO::Elf_Half sec_num = reader.sections.size();
		for (int i=0; i<sec_num; ++i) 
		{
			ELFIO::section* psec = reader.sections[i];
			if (psec->get_name().compare(".comment") == 0)
			{
				const char* p = reader.sections[i]->get_data();
				if (p != nullptr)
				{
					/* Check for "GCC: (GNU) X.X.X" */
					string comment(p);
					size_t pos = comment.find_last_of(')');
					if (pos+2 < comment.size()) 
					{
						gcc_version_found = comment.substr(pos+2);
						if (gcc_version_found.compare(gcc_version_needed) == 0) 
						{
							is_gcc_compatible = true;
						}
					}
				}
				break;
			}
		}
	}

	if (is_gcc_compatible == false)
	{
		spdlog::get("sigil2-console")->info() 
			<< '\'' << user_exec[0] << '\'' << ":";
		spdlog::get("sigil2-console")->info() 
			<< "GCC version " << gcc_version_needed << " not detected";
		if (gcc_version_found.empty() == false) spdlog::get("sigil2-console")->info() 
			<< "GCC version " << gcc_version_found << " found";
		spdlog::get("sigil2-console")->info() 
			<< "OpenMP synchronization events may not be captured";
		spdlog::get("sigil2-console")->info() 
			<< "Pthread synchronization events are probably fine";
	}

	/* format valgrind options */
	//                 program name + valgrind options + tmp_dir + user program options + null
	int vg_opts_size = 1            + 2+args.size()    + 1       + user_exec.size()        + 1;
	char** vg_opts = static_cast<char**>( malloc(vg_opts_size * sizeof(char*)) );

	int i = 0;
	vg_opts[i++] = strdup("valgrind");
	vg_opts[i++] = strdup("--fair-sched=yes"); /* more reliable and reproducible 
												  thread interleaving; round robins
												  each thread instead of letting one
												  thread dominate execution */
	vg_opts[i++] = strdup("--tool=sigrind");
	vg_opts[i++] = strdup((string("--tmp-dir=") + tmp_dir).c_str());
	for (auto &arg : args)
	{
		vg_opts[i++] = strdup(arg.c_str());
	}
	for (auto &arg : user_exec)
	{
		vg_opts[i++] = strdup(arg.c_str());
	}
	vg_opts[i] = nullptr;

	return vg_opts;
}

pair<string, char *const *> configureValgrind(
		const vector<string> &user_exec,
		const vector<string> &args,
		const string &tmp_path)
{
	/* check for valgrind directory
	 * TODO hardcoded, check args instead */

	int len, dirname_len;
	len = wai_getExecutablePath(NULL, 0, &dirname_len);
	char path[len+1];

	if (len > 0)
	{
		wai_getExecutablePath(path, len, &dirname_len);
		path[dirname_len] = '\0';
	}
	else
	{
		throw runtime_error("Couldn't find executable path");
	}

	/* check if function capture is available 
	 * (for multithreaded lib intercepts) */
	string sglwrapper(string(path) + string("/libsglwrapper.so"));
	ifstream sofile(sglwrapper);
	if (sofile.good() == true)
	{
		const char *get_preload = getenv("LD_PRELOAD");
		string set_preload;
		if (get_preload == nullptr) set_preload = sglwrapper;
		else set_preload = string(get_preload) + string(":") + sglwrapper;
		setenv("LD_PRELOAD", set_preload.c_str(), true);
	}
	else
	{
		/* If the wrapper library is in LD_PRELOAD, 
		 * but not in the sigil2 directory,
		 * then this warning can be ignored */
		spdlog::get("sigil2-console")->info() << "'sglwrapper.so' not found";
		spdlog::get("sigil2-console")->info() 
			<< "Synchronization events will not be detected without the wrapper library loaded";
	}
	sofile.close();

	/* HACK if the user decides to move the install folder, valgrind will
	 * get confused and require that VALGRIND_LIB be set.
	 * Set this variable for the user to avoid confusion */
	string vg_lib = string(path) + string("/vg/lib/valgrind");
	setenv("VALGRIND_LIB", vg_lib.c_str(), true);

	string vg_exec = string(path) + string("/vg/bin/valgrind");

	/* execvp() expects a const char* const* */
	auto vg_opts = tokenizeOpts(user_exec, args, tmp_path);

	return make_pair(vg_exec, vg_opts);
}
}; //end namespace


void frontendSigrind (
		const vector<string> &user_exec,
		const vector<string> &args)
{
	assert (user_exec.empty() == false);

	/* check IPC path */
	char* tmp_path = getenv("TMPDIR");
	if (tmp_path == nullptr)
	{
		spdlog::get("sigil2-console")->info() << "'TMPDIR' not detected, defaulting to '/tmp'";
		tmp_path = strdup("/tmp");
	}

	try
	{
		/* set up interface to valgrind */
		Sigrind sigrind_iface(tmp_path);

		/* set up valgrind environment */
		auto valgrind_args = configureValgrind(user_exec, args, tmp_path);

		pid_t pid = fork();
		if ( pid >= 0 )
		{
			if (pid == 0)
			{
				/* kickoff Valgrind */
				int res = execvp(valgrind_args.first.c_str(), valgrind_args.second);
				if (res == -1)
				{
					perror("starting valgrind");
					throw runtime_error("Valgrind exec failed");
				}
			}
			else
			{
				sigrind_iface.produceSigrindEvents();
			}
		}
		else
		{
			perror("Sigrind frontend initialization");
			throw runtime_error("Sigrind fork failed");
		}
	}
	catch(runtime_error& e)
	{
		terminate();
	}
	catch(exception &e)
	{
		terminate();
	}
}
}; //end namespace sgl
