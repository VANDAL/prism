#ifndef _SIGIL_MASTER_
#define _SIGIL_MASTER_

#include "Common.h"
#include "FnMgmt.h"
#include "IRMgmt.h"

/** 
 * This file in charge of delegating notifications with 
 *
 * Sigil is designed to interface with 9 types of dynamic
 * instrumentation and intermediate representation (IR)
 * 'primitives':
 *	1) Function Call
 *	2) Function Return
 *	3) IOP (IR)
 *	4) FLOP (IR)
 *	5) data read (IR)
 *	6) data write (IR)
 *	7) instruction (ISA)
 *	8) thread swap
 *	9) synchronization primitive
 *
 * Longjmps and other flow control are not considered
 */

/* Modify this interface to add capture functionality */

/*****************/
/* Function Call */
/*****************/
void (* _notifyFunctionCall())(UInt32 fid, const char* name);

/*******************/
/* Function Return */
/*******************/
void (* _notifyFunctionReturn())();

/*********************/
/* Integer Operation */
/*********************/
void (* _notifyIOP())(IOPType type, UInt32 size);

/****************************/
/* Floating Point Operation */
/****************************/
void (* _notifyFLOP())(FLOPType type, UInt32 size);

/*************/
/* Data Read */
/*************/
void (* _notifyRead())(UInt64 addr, UInt64 bytes);

/**************/
/* Data Write */
/**************/
void (* _notifyWrite())(UInt64 addr, UInt64 bytes);

/*******************/
/* ISA Instruction */
/*******************/
void (* _notifyInstr())(UInt64 addr, UInt64 bytes);

/***************/
/* Thread Swap */
/***************/
void (* _notifyThreadSwap())(UInt32 tid);

/*****************************/
/* Synchronization Primitive */
/*****************************/
void (* _notifySync())(UInt32 tid);

/************************************************/
/* Final clean up function after trace complete */
/************************************************/
void (* _finishSGL())();

#endif
