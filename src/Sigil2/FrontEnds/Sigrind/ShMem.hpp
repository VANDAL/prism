#ifndef SGL_SIGRIND_SHMEM_H
#define SGL_SIGRIND_SHMEM_H

#include "ShMemData.h"
#include <string>

namespace sgl
{
namespace sigrind
{

class ShMem
{
	std::string tmp_file;
	SigrindSharedData* shared_mem;
public:
	ShMem(const std::string &tmp_dir);
	~ShMem();

	/* Get all events from Sigrind until it's signaled to stop by Sigrind */
	/* FIXME ML: Sigil2 can hang here if Sigrind exits unexpectedly before 
	 * signaling ShMem to finish. Implementing a named pipe allows us
	 * to detect this */
	void readFromSigrind();
};	

}; //end namespace sigrind
}; //end namespace sgl

#endif
