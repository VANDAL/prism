#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory>

#include "ShMem.hpp"
#include "Sigil2/InstrumentationIface.h" 

namespace sgl
{
namespace sigrind
{

ShMem::ShMem()
{
	std::unique_ptr<SigrindSharedData> init(new SigrindSharedData());
	init->sigrind_finish = false;
	init->head = 0;
	init->tail = 0;

	//FIXME clean up file if there's an error...possibly register signal handler???

	int fd = open(SIGRIND_SHMEM_NAME, O_CREAT|O_RDWR|O_TRUNC, 0600);
	if ( fd == -1 )
	{
		std::perror("shared memory initialization");
		throw std::runtime_error("Sigrind shared memory file open failed");
	}

	int bytes = write(fd,init.get(),sizeof(SigrindSharedData));
	if ( bytes != sizeof(SigrindSharedData) )
	{
		std::perror("shared memory initialization");
		close( fd );
		throw std::runtime_error("Sigrind shared memory file write failed");
	}	

	shared_mem = reinterpret_cast<SigrindSharedData*>
		(mmap(nullptr, sizeof(SigrindSharedData), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));
	if (shared_mem == (void*) -1)
	{
		std::perror("shared memory initialization");
		close( fd );
		throw std::runtime_error("Sigrind mmap shared memory failed");
	}

	close( fd );
}

ShMem::~ShMem()
{
	munmap(shared_mem, sizeof(SigrindSharedData));
	if ( std::remove(SIGRIND_SHMEM_NAME) != 0 )
	{
		std::perror("deleting shared memory");
	}
}

/* TODO ML: we can save ourselves from burning cycles by
 * reverting to a buffering system, and having both 
 * Valgrind and Sigil2 block on a pipe for 
 * buffer full/empty notifications */
void ShMem::readFromSigrind()
{
	volatile unsigned int& tail = shared_mem->tail;
	volatile unsigned int& head = shared_mem->head;
	volatile char& sigrind_finish = shared_mem->sigrind_finish;
	BufferedSglEv (&buf)[SIGRIND_BUFSIZE] = shared_mem->buf;

	while (!sigrind_finish)
	{
		while ( tail != head )
		{
			BufferedSglEv& ev = buf[tail];
			switch(ev.tag)
			{
			case SGL_MEM_TAG:
				SGLnotifyMem(ev.mem_ev);
				break;
			case SGL_COMP_TAG:
				SGLnotifyComp(ev.comp_ev);
				break;
			case SGL_SYNC_TAG:
				SGLnotifySync(ev.sync_ev);
				break;
			default:
				break;
			}

			if ( ++tail == SIGRIND_BUFSIZE )
			{
				tail = 0;
			}
		}
	}
}

}; // end namespace sigrind
}; //end namespace sgl
