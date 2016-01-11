#include "Dummy.h"
#include "stdio.h"

void doMemEventProcessing(const int uid)
{
	printf("mem event:  %d\n", uid);
}

void doCompEventProcessing(const int uid)
{
	printf("comp event: %d\n", uid);
}
