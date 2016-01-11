#include "gen.h"
#include <stdlib.h>
#include <time.h>

WorkloadGenerator::WorkloadGenerator()
{
	srand(time(NULL));
}

SglMemEv WorkloadGenerator::getRandMemEvent()
{
	SglMemEv ev;

	int randnum = rand();
	if (0 == (randnum % 2))
		ev.type = MemType::STORE;
	else if (1 == (randnum % 2))
		ev.type = MemType::LOAD;

	ev.addr = randnum;
	ev.size = randnum % 8;
	ev.alignment = (randnum % 3) * 2;
	return ev;
}

SglCompEv WorkloadGenerator::getRandCompEvent()
{
	SglCompEv ev;
	int randnum;

	randnum = rand();
	switch (randnum % 4)
	{
		case 0:
			ev.num_ops = UNARY;
			break;
		case 1:
			ev.num_ops = BINARY;
			break;
		case 2:
			ev.num_ops = TERNARY;
			break;
		case 3:
			ev.num_ops = QUARTERNARY;
			break;
		default:
			break;
	}

	ev.cost_size = 1;
	ev.cost_op = MULT;
	ev.cost_type = IOP;

	return ev;
}

SglCFEv WorkloadGenerator::getRandCFEvent()
{
//TODO
	SglCFEv ev;
	return ev;
}

SglCxtEv WorkloadGenerator::getRandCxtEvent()
{
//TODO
	SglCxtEv ev;
	return ev;
}

SglSyncEv WorkloadGenerator::getRandSyncEvent()
{
//TODO
	SglSyncEv ev;
	return ev;
}
