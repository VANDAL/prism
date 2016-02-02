#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <memory>

#include "ShMem.hpp"
#include "Sigil2/EventManager.hpp" 

namespace sgl
{
namespace sigrind
{

ShMem::ShMem()
{
	std::unique_ptr<SigrindSharedData> init(new SigrindSharedData());
	atomic_init(&(init->sigrind_finish), (char)(false));
	atomic_init(&(init->head), (unsigned int)0);
	atomic_init(&(init->tail), (unsigned int)0);

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
	/* All data in shared memory should be volatile */

	while (atomic_load_explicit(&(shared_mem->sigrind_finish), memory_order_relaxed) == false)
	{
		//cache read
		unsigned int tail = atomic_load_explicit(&(shared_mem->tail), memory_order_acquire);

		while (tail != shared_mem->head)
		{
			EvTag tag = shared_mem->buf[tail].tag;
			switch(tag)
			{
			case SGL_MEM_TAG:
				EventManager::instance().addEvent(shared_mem->buf[tail].mem);
				break;
			case SGL_COMP_TAG:
				EventManager::instance().addEvent(shared_mem->buf[tail].comp);
				break;
			case SGL_SYNC_TAG:
				EventManager::instance().addEvent(shared_mem->buf[tail].sync);
				break;
			default:
				break;
			}

			if ( ++tail == SIGRIND_BUFSIZE )
			{
				tail = 0;
			}

			atomic_store_explicit(&(shared_mem->tail), tail, memory_order_relaxed);
		}
	}
}

}; // end namespace sigrind
}; //end namespace sgl
