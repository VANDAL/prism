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
	init->buffer_full = false;
	init->leftover = 0;

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

void ShMem::readFromSigrind()
{
	while (!shared_mem->sigrind_finish)
	{
		while (!shared_mem->buffer_full)
		{
			if (shared_mem->sigrind_finish)
			{
				goto end;
			}
		}

		flush_sigrind_to_sigil(SIGRIND_BUFSIZE);
		shared_mem->buffer_full = false;
	}

end:	flush_sigrind_to_sigil(shared_mem->leftover);
}

void ShMem::flush_sigrind_to_sigil(unsigned int size)
{
	for (unsigned int i=0; i<size; ++i)
	{
		BufferedSglEv* ev = &shared_mem->buf[i];
		switch(ev->tag)
		{
		case SGL_MEM_TAG:
			SGLnotifyMem(ev->mem_ev);
			break;
		case SGL_COMP_TAG:
			SGLnotifyComp(ev->comp_ev);
			break;
		case SGL_SYNC_TAG:
			SGLnotifySync(ev->sync_ev);
			break;
		default:
			break;
		}
	}
}

}; // end namespace sigrind
}; //end namespace sgl
