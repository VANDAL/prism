#include <iostream>
#include <iterator>
#include <sstream>
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
#include "Sigrind.hpp"

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

	int fd = open(shmem_file.c_str(), O_CREAT|O_RDWR|O_TRUNC, 0600);
	if ( fd == -1 )
	{
		std::perror("shared memory initialization");
		throw std::runtime_error("Sigrind shared memory file open failed");
	}

	unsigned long long bytes = write(fd,init.get(),sizeof(SigrindSharedData));
	if ( bytes != sizeof(SigrindSharedData) )
	{
		std::cerr << bytes << " bytes written\n";
		std::cerr << sizeof(SigrindSharedData) << " bytes written\n";
		std::perror("shared memory initialization");
		close(fd);
		throw std::runtime_error("Sigrind shared memory file write failed");
	}	

	shared_mem = reinterpret_cast<SigrindSharedData*>
		(mmap(nullptr, sizeof(SigrindSharedData), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));
	if (shared_mem == (void*) -1)
	{
		std::perror("shared memory initialization");
		close(fd);
		throw std::runtime_error("Sigrind mmap shared memory failed");
	}
	close(fd);
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
	/* try twice w/ delay, in case Valgrind hasn't started up yet */
	int i = 0;
	for (i = 0; i < 2; ++i)
	{
		int res = read(fullfd, &full_data, sizeof(full_data));

		if (res > 0)
		{
			break;
		}
		else if (res == 0)
		{
			/* give valgrind time to connect to pipe */
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
		else 
		{
			perror("Reading from full fifo");
			throw std::runtime_error("Could not read from Valgrind full fifo");
		}
	}
	if (i > 1)
	{
		throw std::runtime_error("No Valgrind writer detected on full fifo");
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
			EventManager::instance().addEvent(buf[i].mem);
			break;
		case SGL_COMP_TAG:
			EventManager::instance().addEvent(buf[i].comp);
			break;
		case SGL_SYNC_TAG:
			EventManager::instance().addEvent(buf[i].sync);
			break;
		default:
			break;
		}
	}
}

namespace
{
char* const* tokenizeOpts (const std::string &tmp_dir, const std::string &user_exec)
{
	assert( !user_exec.empty() );

	/* FIXME this does not account for quoted arguments with spaces
	 * Both whitespace and quote pairs should be delimiters */
	std::istringstream iss(user_exec);
	std::vector<std::string> tokens{
		std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>()};

	//                 program name + valgrind options + tmp_dir + user program options + null
	int vg_opts_size = 1            + 1                + 1       + tokens.size()        + 1;
	char** vg_opts = static_cast<char**>( malloc(vg_opts_size * sizeof(char*)) );

	int i = 0;
	vg_opts[i++] = strdup("valgrind");
	vg_opts[i++] = strdup("--tool=sigrind");
	vg_opts[i++] = strdup((std::string("--tmp-dir=") + tmp_dir).c_str());
	for (std::string token : tokens) 
	{
		vg_opts[i++] = strdup(token.c_str());
	}
	vg_opts[i] = nullptr;

	return vg_opts;
}

void startValgrind (
		const std::string &user_exec, 
		const std::string &sigrind_dir, 
		const std::string &tmp_dir
		) 
{
	std::string vg_exec = sigrind_dir + "/valgrind";

	/* execvp() expects a const char* const* */
	auto vg_opts = tokenizeOpts(tmp_dir, user_exec);

	/* kickoff Valgrind */
	if ( execvp(vg_exec.c_str(), vg_opts) == -1 )
	{
		std::perror("starting valgrind");
		throw std::runtime_error("Valgrind exec failed");
	}
}
}; //end namespace

void frontendSigrind (
		const std::string &user_exec, 
		const std::string &sigrind_dir, 
		const std::string &tmp_dir
		) 
{
	assert ( !(user_exec.empty() || sigrind_dir.empty() || tmp_dir.empty()) );

	try
	{
		Sigrind sigrind_iface(tmp_dir);

		pid_t pid = fork();
		if ( pid >= 0 )
		{
			if ( pid == 0 )
			{
				startValgrind(user_exec, sigrind_dir, tmp_dir);
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
		throw e;
	}
}
}; //end namespace sgl
