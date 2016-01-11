#ifndef MSG_BUIDLER_H
#define MSG_BUIDLER_H


typedef struct VGBufState VGBufState;
struct VGBufState
{
	int size = 0;
	int used = 0;
	int prim_type = 0;
	unsigned char msg[128];
};

//Deserialize that msg and hook into the sigil back end
void sendToSgl( VGBufState data );

//Build up a sigil 'msg' created in from the valgrind server, here in the client
void parseSglMsg ( unsigned char* buf, int nbuf, VGBufState& buf_state );

#endif
