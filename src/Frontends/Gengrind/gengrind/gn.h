#ifndef GN_H
#define GN_H

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_options.h"
#include "pub_tool_debuginfo.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcfile.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_libcproc.h"
#include "pub_tool_machine.h"
#include "pub_tool_mallocfree.h"
#include "Core/EventBuffer.h"

#define GN_ENABLE_DEBUG 1

#ifdef VGA_amd64
#include "VEX/pub/libvex_guest_amd64.h"
#endif

#define GN_(str) VGAPPEND(vgGengrind_,str)
/* The convention is to prepend global funcs/vars/structs with GN_(str).
 * file/local scoped funcs/vars/structs do not have a naming convention,
 * but can be prepended with 'gn'. */

#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)

//-------------------------------------------------------------------------------------------------
/** Platform specific defines **/
#if defined(VG_BIGENDIAN)
#define ENDNESS Iend_BE
#elif defined(VG_LITTLEENDIAN)
#define ENDNESS Iend_LE
#else
#error "Unknown endianness"
#endif

#ifdef VGA_amd64
#define OFFB_RIP offsetof(VexGuestAMD64State,guest_RIP)
#define IRCONST_PTR(...) IRConst_U64(__VA_ARGS__)
#define IRTYPE_PTR Ity_I64
#define IOP_ADD_PTR Iop_Add64
#define IOP_SUB_PTR Iop_Sub64
#define IOP_CMPLT_PTR Iop_CmpLT64U
#else
#error "Unsupported platform"
#endif

//-------------------------------------------------------------------------------------------------
/** Forward declare typedefs **/

/* BB lookup type declarations */
typedef struct _BBState BBState;
typedef struct _BBInfo BBInfo;
typedef struct _BBTable BBTable;

/* Callstack tracking type declarations */
typedef struct _CallStack CallStack;
typedef struct _CallEntry CallEntry;

/* Jumps type declarations */
typedef struct _JumpNode JumpNode;
typedef struct _JumpTable JumpTable;
typedef enum _GnJumpKind GnJumpKind;

/* Function lookup type declarations */
typedef struct _FnNode FnNode;
typedef struct _FileNode FileNode;
typedef struct _ObjNode ObjNode;


#endif
