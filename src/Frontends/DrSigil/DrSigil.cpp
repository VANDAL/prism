#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cassert>

#include "whereami.h"

#include "Sigil2/Sigil.hpp"
#include "DrSigil.hpp"

/* Sigil2's DynamoRIO frontend forks DynamoRIO off as a separate process;
 * The DynamoRIO client sends the frontend dynamic events from the
 * application via shared memory */

namespace sgl
{

////////////////////////////////////////////////////////////
// Sigil2 - DynamoRIO IPC
////////////////////////////////////////////////////////////
DrSigil::DrSigil(int ipc_idx, std::string tmp_dir)
	: ipc_idx(ipc_idx)
	, curr_thread_id(-1)
	, shmem_file(tmp_dir + "/" + DRSIGIL_SHMEM_NAME + "-" + std::to_string(ipc_idx))
	, empty_file(tmp_dir + "/" + DRSIGIL_EMPTYFIFO_NAME + "-" + std::to_string(ipc_idx))
	, full_file(tmp_dir + "/" + DRSIGIL_FULLFIFO_NAME + "-" + std::to_string(ipc_idx))
{
	initShMem();
	makeNewFifo(empty_file.c_str());
	makeNewFifo(full_file.c_str());
}


DrSigil::~DrSigil()
{
	/* file cleanup */
	if(remove(shmem_file.c_str()) != 0 ||
			remove(empty_file.c_str()) != 0 ||
			remove(full_file.c_str()) != 0)
	{
		SigiLog::warn(std::string("deleting IPC files -- ").append(strerror(errno)));
	}
}


void DrSigil::initShMem()
{
	std::unique_ptr<DrSigilSharedData> init(new DrSigilSharedData());

	FILE *fd = fopen(shmem_file.c_str(), "wb+");
	if(fd == nullptr)
	{
		SigiLog::fatal(std::string("sigrind shared memory file open failed -- ").append(strerror(errno)));
	}

	/* XXX From write(2) man pages:
	 *
	 * On Linux, write() (and similar system calls) will transfer at most
	 * 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
	 * actually transferred.  (This is true on both 32-bit and 64-bit
	 * systems.)
	 *
	 * fwrite doesn't have this limitation */
	int count = fwrite(init.get(),sizeof(DrSigilSharedData), 1, fd);
	if( count != 1 )
	{
		fclose(fd);
		SigiLog::fatal(std::string("sigrind shared memory file write failed -- ").append(strerror(errno)));
	}	

	shared_mem = reinterpret_cast<DrSigilSharedData*>
		(mmap(nullptr, sizeof(DrSigilSharedData), PROT_READ|PROT_WRITE, MAP_SHARED, fileno(fd), 0));
	if(shared_mem == (void*) -1)
	{
		fclose(fd);
		SigiLog::fatal(std::string("sigrind mmap shared memory failed -- ").append(strerror(errno)));
	}
	fclose(fd);
}


void DrSigil::makeNewFifo(const char* path) const
{
	if( mkfifo(path, 0600) < 0 )
	{
		if(errno == EEXIST)
		{
			if( remove(path) != 0 )
			{
				SigiLog::fatal(std::string("sigil2 could not delete old fifos -- ").append(strerror(errno)));
			}
			if( mkfifo(path, 0600) < 0 )
			{
				SigiLog::fatal(std::string("sigil2 failed to create dynamorio fifos -- ").append(strerror(errno)));
			}
		}
		else
		{
			SigiLog::fatal(std::string("sigil2 failed to create dynamorio fifos -- ").append(strerror(errno)));
		}
	}
}


void DrSigil::connectDynamoRIO()
{
	/* Indefinitely wait for DynamoRIO to connect
	 * because DR will not connect until a corresponding
	 * thread is spawned for event generation.
	 *
	 * The event generation thread spawn time is undefined */
	emptyfd = open(empty_file.c_str(), O_WRONLY);

	/* XXX Sigil might get stuck blocking if DynamoRIO
	 * unexpectedly exits before connecting at this point */
	fullfd = open(full_file.c_str(), O_RDONLY);

	if(fullfd < 0)
	{
		SigiLog::fatal(std::string("sigil2 failed to open dynamorio fifos -- ").append(strerror(errno)));
	}
}


void DrSigil::disconnectDynamoRIO()
{
	munmap(shared_mem, sizeof(DrSigilSharedData));
	close(emptyfd);
	close(fullfd);
}


int DrSigil::readFullFifo()
{
	int res = read(fullfd, &full_data, sizeof(full_data));

	if(res == 0)
	{
		SigiLog::fatal("Unexpected end of fifo");
	}
	else if(res < 0)
	{
		SigiLog::fatal(std::string("could not read from dynamorio full fifo -- ").append(strerror(errno)));
	}

	return full_data;
}


void DrSigil::writeEmptyFifo(unsigned int idx)
{
	if(write(emptyfd, &idx, sizeof(idx)) < 0)
	{
		SigiLog::fatal(std::string("could not send dynamorio empty buffer status -- ").append(strerror(errno)));
	}
}


void DrSigil::produceFromBuffer(unsigned int idx, unsigned int used)
{
	assert(idx < DRSIGIL_BUFNUM);

	DrSigilEvent (&buf)[DRSIGIL_BUFSIZE] = shared_mem->buf[idx];

	for (unsigned int i=0; i<used; ++i)
	{
		/* Sigil2 backend indices start at '0' */
		assert(ipc_idx == (buf[i].thread_id) % num_threads);
		if(ipc_idx != (buf[i].thread_id) % num_threads)
		{
			SigiLog::fatal("IPC channel received incorrect thread");
		}

		/* The thread id is checked for every event,
		 * because there is no restriction on event
		 * ordering between different threads in DynamoRIO
		 *
		 * Thread switch events are generated here,
		 * instead of in DynamorRIO */
		if(buf[i].thread_id != curr_thread_id)
		{
			curr_thread_id = buf[i].thread_id;
			thread_swap_event.type = SGLPRIM_SYNC_SWAP;
			thread_swap_event.id = curr_thread_id;
			Sigil::instance().addEvent(thread_swap_event,ipc_idx);
		}

		switch(buf[i].ev.tag)
		{
		case EvTag::SGL_MEM_TAG:
			Sigil::instance().addEvent(buf[i].ev.mem,ipc_idx);
			break;
		case EvTag::SGL_COMP_TAG:
			Sigil::instance().addEvent(buf[i].ev.comp,ipc_idx);
			break;
		case EvTag::SGL_SYNC_TAG:
			Sigil::instance().addEvent(buf[i].ev.sync,ipc_idx);
			break;
		case EvTag::SGL_CXT_TAG:
			Sigil::instance().addEvent(buf[i].ev.cxt,ipc_idx);
			break;
		default:
			SigiLog::fatal("received unhandled event in sigrind");
			break;
		}
	}
}

void DrSigil::produceDynamoRIOEvents()
{
	/* DynamoRIO should have initialized by now */
	connectDynamoRIO();

	while (finished == false)
	{
		/* Each DynamoRIO thread sends event buffer metadata */
		unsigned int from_dynamorio = readFullFifo();

		unsigned int idx;
		unsigned int used;
		if(from_dynamorio == DRSIGIL_FINISHED)
		{
			std::cerr << "DrSigil finish detected for IPC " << ipc_idx << std::endl;
			/* DynamoRIO thread finished;
			 * partial leftover buffer */
			finished = true;
			idx = readFullFifo();
			used = readFullFifo();
		}
		else
		{
			/* full buffer */
			used = from_dynamorio;
			idx = readFullFifo();
		}

		/* send data to backend */
		produceFromBuffer(idx, used);

		/* tell DynamoRIO that the buffer is empty again */
		writeEmptyFifo(idx);
	}

	/* lets DynamoRIO know Sigil is finished */
	std::cerr << "DrSigil disconnecting IPC " << ipc_idx << std::endl;
	disconnectDynamoRIO();
}


////////////////////////////////////////////////////////////
// Launching DynamoRIO
////////////////////////////////////////////////////////////
namespace
{

char* const* tokenizeOpts (
		const std::vector<std::string> &user_exec,
		const std::vector<std::string> &args,
		const std::string &sigil_bin_dir,
		const std::string &tmp_dir,
		const uint16_t num_threads)
{
	assert(!user_exec.empty() && !tmp_dir.empty());

	/* format dynamorio options */
	//                 program name + dynamorio options + user program options + null
	int dr_opts_size = 1            + 7+args.size()     + user_exec.size()        + 1;
	char** dr_opts = static_cast<char**>( malloc(dr_opts_size * sizeof(char*)) );

	int i = 0;
	dr_opts[i++] = strdup("drrun");
	dr_opts[i++] = strdup("-root");
	dr_opts[i++] = strdup((sigil_bin_dir + "/dr").c_str());
	dr_opts[i++] = strdup("-c");

	/* FIXME hardcoding 64-bit and debug/release */
	dr_opts[i++] = strdup((sigil_bin_dir + ("/dr/tools/lib64/release/libdrsigil.so")).c_str());
	dr_opts[i++] = strdup(std::string("--num-frontend-threads=").append(std::to_string(num_threads)).c_str());
	dr_opts[i++] = strdup((std::string("--tmp-dir=").append(tmp_dir)).c_str());

	dr_opts[i++] = strdup("--");
	for (auto &arg : args)
	{
		dr_opts[i++] = strdup(arg.c_str());
	}
	for (auto &arg : user_exec)
	{
		dr_opts[i++] = strdup(arg.c_str());
	}
	dr_opts[i] = nullptr;

	return dr_opts;
}


std::pair<std::string, char *const *> configureDynamoRIO(
		const std::vector<std::string> &user_exec,
		const std::vector<std::string> &args,
		const std::string &tmp_path,
		const uint16_t num_threads)
{
	int len, dirname_len;
	len = wai_getExecutablePath(NULL, 0, &dirname_len);
	char path[len+1];

	if(len > 0)
	{
		wai_getExecutablePath(path, len, &dirname_len);
		path[dirname_len] = '\0';
	}
	else
	{
		SigiLog::fatal("couldn't find executable path");
	}

	/* FIXME hardcoding bin64; need to detect dynamically;
	 * maybe a config file that's generated at build time */
	std::string dr_exec = std::string(path) + ("/dr/bin64/drrun");

	/* execvp() expects a const char* const* */
	auto dr_opts = tokenizeOpts(user_exec, args, path, tmp_path, num_threads);

	return std::make_pair(dr_exec, dr_opts);
}

}; //end namespace


/* static init */
int sgl::DrSigil::num_threads = 1;

void DrSigil::start(
		const std::vector<std::string> &user_exec,
		const std::vector<std::string> &args,
		const uint16_t num_threads)
{
	assert(user_exec.empty() == false);

	DrSigil::num_threads = num_threads;

	/* check IPC path */
	char* tmp_path = getenv("TMPDIR");

	/* posix shmem typically uses /dev/shm */
	if(tmp_path == nullptr) tmp_path = strdup("/dev/shm");

	struct stat info;
	if(stat(tmp_path,&info) != 0)
	{
		SigiLog::fatal(std::string(tmp_path).append(
					" not found\n\tset environment var 'TMPDIR' to a tmpfs mount"));
	}

	/* set up dynamorio environment */
	auto dynamorio_args = configureDynamoRIO(user_exec, args, tmp_path, num_threads);

	/* DynamoRIO frontend has a multithreaded interface */
	std::vector<std::shared_ptr<DrSigil>> dr_ifaces;
	std::vector<std::thread> dr_iface_producers;

	/* set up interfaces to dynamorio */
	for(int i=0; i<num_threads; ++i)
	{
		dr_ifaces.push_back(std::make_shared<DrSigil>(i,tmp_path));
	}

	pid_t pid = fork();
	if(pid >= 0)
	{
		if(pid == 0)
		{
			/* kickoff DynamoRIO */
			int res = execvp(dynamorio_args.first.c_str(), dynamorio_args.second);
			if(res == -1)
			{
				SigiLog::fatal(std::string("starting dynamorio failed -- ").append(strerror(errno)));
			}
		}
		else
		{
			for(auto &iface : dr_ifaces)
			{
				dr_iface_producers.push_back(std::thread(&DrSigil::produceDynamoRIOEvents, iface.get()));
			}

			for(auto &thr : dr_iface_producers)
			{
				thr.join();
			}
		}
	}
	else
	{
		SigiLog::fatal(std::string("sigrind fork failed -- ").append(strerror(errno)));
	}
}

}; //end namespace sgl
