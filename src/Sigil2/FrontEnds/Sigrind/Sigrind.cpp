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

/* Sigil2's Valgrind frontend forks Valgrind off as a separate process;
 * Valgrind sends the frontend dynamic events from the application via shared memory */

namespace sgl
{

Sigrind::Sigrind(std::string tmp_dir) 
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
	if ( std::remove(shmem_file.c_str()) != 0 ||
			std::remove(empty_file.c_str()) != 0 ||
			std::remove(full_file.c_str()) != 0)
	{
		std::perror("deleting IPC files");
	}
}

void Sigrind::initShMem()
{
	std::unique_ptr<SigrindSharedData> init(new SigrindSharedData());

	FILE *fd = fopen(shmem_file.c_str(), "wb+");
	if ( fd == nullptr )
	{
		std::perror("shared memory initialization");
		throw std::runtime_error("Sigrind shared memory file open failed");
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
		std::perror("shared memory initialization");
		fclose(fd);
		throw std::runtime_error("Sigrind shared memory file write failed");
	}	

	shared_mem = reinterpret_cast<SigrindSharedData*>
		(mmap(nullptr, sizeof(SigrindSharedData), PROT_READ|PROT_WRITE, MAP_SHARED, fileno(fd), 0));
	if (shared_mem == (void*) -1)
	{
		std::perror("shared memory initialization");
		fclose(fd);
		throw std::runtime_error("Sigrind mmap shared memory failed");
	}
	fclose(fd);
}

void Sigrind::makeNewFifo(const char* path) const
{
	if ( mkfifo(path, 0600) < 0 )
	{
		if (errno == EEXIST)
		{
			if ( std::remove(path) != 0 )
			{
				std::perror(path);
				throw std::runtime_error("Sigrind could not delete old fifos");
			}
			if ( mkfifo(path, 0600) < 0 )
			{
				std::perror("mkfifo");
				throw std::runtime_error("Sigrind failed to create Valgrind fifos");
			}
		}
		else
		{
			std::perror("mkfifo");
			throw std::runtime_error("Sigrind failed to create Valgrind fifos");
		}
	}
}

void Sigrind::connectValgrind()
{
	/* XXX Sigil might get stuck waiting for Valgrind
	 * if Valgrind unexpectedly exits before trying
	 * to connect */
	emptyfd = open(empty_file.c_str(), O_WRONLY);
	fullfd = open(full_file.c_str(), O_RDONLY);
	if ( emptyfd < 1 || fullfd < 1 )
	{
		std::perror("open fifo");
		throw std::runtime_error("Sigrind failed to open Valgrind fifos");
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
		throw std::runtime_error("Unexpected end of fifo");
	}
	else if (res < 0)
	{
		perror("Reading from full fifo");
		throw std::runtime_error("Could not read from Valgrind full fifo");
	}

	return full_data;
}

void Sigrind::writeEmptyFifo(unsigned int idx)
{
	if (write(emptyfd, &idx, sizeof(idx)) < 0)
	{
		std::perror("write Empty");
		throw std::runtime_error("Could not send Valgrind empty buffer status");
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
			throw std::runtime_error("Received unhandled event in Sigrind");
			break;
		}
	}
}

namespace
{
char* const* tokenizeOpts (const std::string &tmp_dir, const std::string &user_exec)
{
	assert( !tmp_dir.empty() );

	/* FIXME this does not account for quoted arguments with spaces
	 * Both whitespace and quote pairs should be delimiters */
	std::istringstream iss(user_exec);
	std::vector<std::string> tokens{
		std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>()};

	//                 program name + valgrind options + tmp_dir + user program options + null
	int vg_opts_size = 1            + 2                + 1       + tokens.size()        + 1;
	char** vg_opts = static_cast<char**>( malloc(vg_opts_size * sizeof(char*)) );

	int i = 0;
	vg_opts[i++] = strdup("valgrind");
	vg_opts[i++] = strdup("--fair-sched=yes"); /* more reliable and reproducible 
												  thread interleaving; round robins
												  each thread instead of letting one
												  thread dominate execution */
	vg_opts[i++] = strdup("--tool=sigrind");
	vg_opts[i++] = strdup((std::string("--tmp-dir=") + tmp_dir).c_str());
	for (std::string token : tokens) 
	{
		vg_opts[i++] = strdup(token.c_str());
	}
	vg_opts[i] = nullptr;

	return vg_opts;
}

void startValgrind (const std::string &user_exec, const std::string &args, const std::string &tmp_path) 
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
		throw std::runtime_error("Couldn't find executable path");
	}

	/* check if function capture is available 
	 * (for multithreaded lib intercepts) */
	std::string sglwrapper(std::string(path) + std::string("/sglwrapper.so"));
	std::ifstream sofile(sglwrapper);
	if (sofile.good() == true)
	{
		const char *get_preload = getenv("LD_PRELOAD");
		std::string set_preload;
		if (get_preload == nullptr) set_preload = sglwrapper;
		else set_preload = std::string(get_preload) + std::string(":") + sglwrapper;
		setenv("LD_PRELOAD", set_preload.c_str(), true);
	}
	else
	{
		spdlog::get("sigil2-console")->info() << "'sglwrapper.so' not found";
		spdlog::get("sigil2-console")->info() << "Synchronization Events will not be detected";
	}
	sofile.close();

	/* HACK if the user decides to move the install folder, valgrind will
	 * get confused and require that VALGRIND_LIB be set.
	 * Set this variable for the user to avoid confusion */
	std::string vg_lib = std::string(path) + std::string("/vg/lib/valgrind");
	setenv("VALGRIND_LIB", vg_lib.c_str(), true);

	std::string vg_exec = std::string(path) + std::string("/vg/bin/valgrind");

	/* execvp() expects a const char* const* */
	auto vg_opts = tokenizeOpts(tmp_path, user_exec);

	/* kickoff Valgrind */
	if ( execvp(vg_exec.c_str(), vg_opts) == -1 )
	{
		std::perror("starting valgrind");
		throw std::runtime_error("Valgrind exec failed");
	}
}
}; //end namespace

void frontendSigrind (const std::string &user_exec, const std::string &args)
{
	assert (user_exec.empty() == false);

	/* check IPC path */
	char* tmp_path = std::getenv("TMPDIR");
	if (tmp_path == nullptr)
	{
		spdlog::get("sigil2-console")->info() << "'TMPDIR' not detected, defaulting to '/tmp'";
		tmp_path = strdup("/tmp");
	}

	try
	{
		Sigrind sigrind_iface(tmp_path);

		pid_t pid = fork();
		if ( pid >= 0 )
		{
			if ( pid == 0 )
			{
				startValgrind(user_exec, args, tmp_path);
			}
			else
			{
				sigrind_iface.produceSigrindEvents();
			}
		}
		else
		{
			std::perror("Sigrind frontend initialization");
			throw std::runtime_error("Sigrind fork failed");
		}
	}
	catch(std::runtime_error& e)
	{
		std::terminate();
	}
	catch(std::exception &e)
	{
		std::terminate();
	}
}
}; //end namespace sgl
