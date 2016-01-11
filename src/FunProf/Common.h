#ifndef _COMMON_H_
#define _COMMON_H_

#include "Prototype.h"

#ifndef PROTOTYPE
	#include "global.h" //Valgrind/Callgrind 

	#define SGL_(str)					VGAPPEND(vgSigil_##str)
	#define SGL_MALLOC(_cc,x)		VG_(malloc)(_cc,x)
	#define SGL_REALLOC(_cc,x,y)	VG_(realloc)(_cc,x,y)
	#define SGL_FREE(p)				VG_(free)(p)
	#define SGL_MEMCPY(d,s,size)	VG_(memcpy)(d,s,size)
	#define SGL_SNPRINTF(x,y,z,args...)	VG_(snprintf)(x,y,z,args)
	#define SGL_ASSERT(x)			
#else
	#define SGL_(str)					vgSigil_##str
	#define SGL_MALLOC(_cc,x)		malloc(x)
	#define SGL_REALLOC(_cc,x,y)	realloc(x,y)
	#define SGL_FREE(p)				free(p)
	#define SGL_MEMCPY(d,s,size)	memcpy(d,s,size)
	#define SGL_SNPRINTF(x,y,z,args...)	snprintf(x,y,z,args)
	#define SGL_ASSERT(x)			
#endif

#define	MAX_MSG_SIZE	1024


//typedef enum _IOPType IOPType;
//enum _IOPType
//{
//};
//
//typedef enum _FLOPType FLOPType;
//enum _FLOPType
//{
//};
//
//typedef enum _SyncType SyncType;
//enum _SyncType
//{
//};

/*********************************************************************/
/*                           Error Handling                          */
/*********************************************************************/
void exitAtError(const char* point_of_err, const char* msg);



#endif
