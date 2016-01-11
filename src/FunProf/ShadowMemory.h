#ifndef _SHADOW_MEMORY_H_
#define _SHADOW_MEMORY_H_

#include "Common.h"

/*
 * Shadow Memory tracks tracks shadow objects (SO) for an address. 
 * Normally, for Sigil, the SO is the function that last read or wrote 
 * to an address. An extended functionality of shadow memory is to 
 * track more primitive events on addresses, such as computation
 * and communication.  
 */

/* Making each SM 22 bits (4MB) seems
   to play nicely with malloc, 
   probably due to page boundaries */
#define ADDR_BITS	38
#define PM_BITS		16
#define SM_BITS		(ADDR_BITS-PM_BITS)

#define PM_SIZE		(1ULL<<PM_BITS)
#define SM_SIZE		(1ULL<<SM_BITS)
#define MAX_PRIMARY_ADDRESS ((1ULL<<ADDR_BITS) -1)

#endif
