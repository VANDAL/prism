#ifndef SGL_SIGRIND_H
#define SGL_SIGRIND_H

#include <string>
#include <vector>
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

	/* multithreaded backend */
	const int num_threads;
	int be_idx;

	/* shared mem */
	SigrindSharedData* shared_mem;
public:
	Sigrind(int num_threads, std::string tmp_dir);
	~Sigrind();
	void produceSigrindEvents();

	static void start(
		const std::vector<std::string> &user_exec,
		const std::vector<std::string> &args,
		const uint16_t num_threads);

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
