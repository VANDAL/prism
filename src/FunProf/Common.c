#include "Prototype.h"
#include "Common.h"

/*********************************************************************/
/*                           Error Handling                          */
/*********************************************************************/
void exitAtError(const char* point_of_err, const char* msg);

static inline char* getErrFmtMsg(const char* point_of_err, const char* msg);

void exitAtError(const char* point_of_err, const char* msg){
#ifndef PROTOTYPE
	VG_(message)(Vg_FailMsg,"%s",getErrFmtMsg(msg));  
	VG_(message_flush)();                             
	VG_(exit)(1);
#else
	printf("%s",getErrFmtMsg(point_of_err, msg));  
	exit(1);
#endif
}


static inline
char* getErrFmtMsg(const char* point_of_err, const char* msg){
	char* err_msg = SGL_MALLOC("SGL.getErrFmtMsg.err_msg",MAX_MSG_SIZE*sizeof(char));
	SGL_SNPRINTF(err_msg,MAX_MSG_SIZE*sizeof(char),"SIGIL ERROR|%s:\t%s\n",point_of_err,msg);

	return err_msg;
} 
