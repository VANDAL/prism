#include "InstrumentationIface.h"
#include "OptionParser.hpp"

#include <iostream>
#include <cstdlib>

/* toy test for scalability profiling */

int main(int argc, char* argv[])
{
	sgl::OptionParser parser(argc, argv);
	parser.parse();

	unsigned long cnt=50000;
	char* end;
	if (argc < 2)
		return -1;
	else
		cnt = strtoul(argv[1], &end, 10)/5;

	SglMemEv memev;
	memev.size = 4;
	memev.begin_addr = 0x0000;

	SglCompEv compev;
	compev.type = CompCostType::COMP_IOP;

	for (unsigned int i = 0; i < cnt; ++i )
	{
		memev.begin_addr = memev.begin_addr += memev.size;
		memev.type = MEM_LOAD;
		SGLnotifyMem(memev);
		SGLnotifyComp(compev);
		SGLnotifyComp(compev);
		memev.type = MEM_STORE;
		SGLnotifyMem(memev);
		SGLnotifyMem(memev);
	}

	SGLnotifyFinish();
}
