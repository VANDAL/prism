#ifndef SGL_SIGRIND_H
#define SGL_SIGRIND_H

#include <string>
#include "SigrindIPC.h"

namespace sgl
{

class Sigrind
{
	const std::string shmem_file;
	const std::string empty_file;
	const std::string full_file;
	bool finished = false;

	/* fifos */
	int emptyfd;
	int fullfd;
	int full_data;

	/* shared mem */
	SigrindSharedData* shared_mem;
public:
	Sigrind(std::string tmp_dir);
	~Sigrind();
	void produceSigrindEvents();

private:
	void initShMem();
	void makeNewFifo(const char* path) const;
	void connectValgrind();

	int readFullFifo();
	void writeEmptyFifo(unsigned int idx);
	void produceFromBuffer(unsigned int idx, unsigned int used);
};

};

#endif
