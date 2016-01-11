#ifndef _SIGIL_H_
#define _SIGIL_H_


/*! \brief Capture an 'instruction' event
 *
 *	Sigil has little use for instruction book keeping. It only needs
 *	a count of the total number of instructions run. Sometimes more than
 *	one instruction is 'flushed' by Callgrind at a time so a
 *	'number of instructions' flushed parameter is used to track this
 *
 *	\param numIntr number of instructions flushed
 */
static inline 
void SGL_(captureInstrEvent) (int numInstr)
{
	SGL_(total_instrs) += numInstr;
}

/*! \brief Capture a 'data read' event
 *	
 *	A piece of data that is read from memory SHOULD have been written there.
 *	I.e. another function must have written that piece of data, assuming this
 *	code does not get outside input.
 *	Note that data reads are the only events considered to be"communication"
 *
 *	Five pieces of data are captured for this event:
 *		1. The function instance that wrote the data
 *		2. The function instance that is reading the data
 *		3. The number of unique bytes involved in the mem transaction
 *		4. T
 *		
 *		Additionally the total number of 'data read' events is incremented
 */
static inline
void SGL_(captureDREvent) (Addr ea, UInt64 datasize, int fid )
{
}

/*! \brief Capture a 'data write' event
 *	
 *	Five pieces of data are captured for this event:
 *		1. The function instance that wrote the data
 *		3. The number of unique bytes involved in the mem transaction
 *		4. T
 *		
 *		Additionally the total number of 'data write' events is incremented
 */
static inline
void SGL_(captureDWEvent) (Addr ea, UInt64 datasize)
{
}




/*! \brief Capture an 'integer operation' event
 *	
 *	1 pieces of data are captured for this event:
 *		1. The function executed the operation
 *		3. The number of unique bytes involved in the mem transaction
 *		4. T
 *		
 *		Additionally the total number of iops events is incremented
 *		and the number of iops in the function instance is incremented,
 *		that the event is from
 */
static inline
void SGL_(captureIopEvent) (Addr ea, UInt64 datasize)
{
}




/*! \brief Capture a 'floating point operation' event
 *	
 *	Five pieces of data are captured for this event:
 *		1. The function instance that wrote the data
 *		3. The number of unique bytes involved in the mem transaction
 *		4. T
 *		
 *		Additionally the total number of iops events is incremented
 *		and the number of iops in the function instance is incremented,
 *		that the event is from
 */
static inline
void SGL_(captureFlopEvent) (Addr ea, UInt64 datasize)
{
}


#endif
