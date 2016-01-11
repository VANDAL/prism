#include "Sigil.h"
#include "Logger.h"

void exitAtError(const HChar* point_of_err, const HChar* msg);

void writeToLog(const HChar* buf, Int len);
void initLogFile(const HChar* log_filename);
void initDebugFile(const HChar* debug_filename);

static inline void initFile(Bool* is_init, Int *fd, const HChar* filename);
static inline Int openFileForWriting(const HChar* filename);
static inline void exitAtOpenError(const HChar* filename);

static Int debug_fd = -1;
static Int log_fd = -1;
static Bool is_init_debug = false;
static Bool is_init_log = false ;


void writeToLog(const HChar* buf, Int len){
	if (is_init_log){
#ifndef PROTOTYPE
		VG_(write)(log_fd, (const void*)buf, len);
#else
		printf("%s",buf);
#endif
	}
	else
		exitAtError("Logger.writeToLog","Can't write! Log file not initialized!");
}


void initDebugFile(const HChar* debug_filename){
	initFile(&is_init_debug, &debug_fd, debug_filename);
}

void initLogFile(const HChar* log_filename){
	initFile(&is_init_log, &log_fd, log_filename);
}





/*********************************************************************/
/*                          Implementation                           */
/*********************************************************************/



static inline
/* I want to enforce that all files have static 
	initialized booleans and file descriptors */

void initFile(Bool* is_init, Int *fd, const HChar* filename){
	if (!*is_init){

		*fd = openFileForWriting(filename);
		*is_init = true;

	}
	else{
		HChar msg[MAX_MSG_SIZE];
//TODO The error message is misleading. The log or debug file
//			may be already initialized, but the 'filename' may not be
		SGL_SNPRINTF(msg,MAX_MSG_SIZE,"File already initialized: %s",filename);
		exitAtError("Logger.initFile",msg);
	}
}


static inline
Int openFileForWriting(const HChar* filename){

#ifndef PROTOTYPE
	Int fd = VG_(fd_open)(filename, VKI_O_CREAT|VKI_O_WRONLY|VKI_O_TRUNC,
					VKI_S_IRUSR|VKI_S_IWUSR);
#else
	Int fd = 0;
#endif

	if (-1 >= fd)
		exitAtOpenError(filename); 

	return fd;
}



static inline
void exitAtOpenError(const HChar* filename){
		HChar msg[MAX_MSG_SIZE];
		SGL_SNPRINTF(msg,MAX_MSG_SIZE,"Could not open file: %s",filename);
		exitAtError("Logger.exitAtOpenError",msg);
}
