/* Helpers to get info for functions from the binary,
 * such as scanning ELF sections and debug info */

#ifndef GN_FILE_H
#define GN_FILE_H

#include "gn.h"


//-------------------------------------------------------------------------------------------------
/** File type definitions **/

/* Quite arbitrary fixed hash sizes */
#define   N_OBJ_ENTRIES         47
#define  N_FILE_ENTRIES         53
#define    N_FN_ENTRIES         87

struct _FileNode {
   HChar*     name;
   FnNode*   fns[N_FN_ENTRIES];
   UInt       number;
   ObjNode*  obj;
   FileNode* next;
};


struct _ObjNode {
    /* If an object is dlopened multiple times, we hope that <name> is unique;
     * <start> and <offset> can change with each dlopen, and <start> is
     * zero when object is unmapped (possible at dump time).  */

   const HChar* name;
   UInt       last_slash_pos;

   Addr       start;  /* Start address of text segment mapping */
   SizeT      size;   /* Length of mapping */
   PtrdiffT   offset; /* Offset between symbol address and file offset */

   FileNode* files[N_FILE_ENTRIES];
   UInt       number;
   ObjNode*  next;
};


struct _FnNode {
    /* the <number> of fn_node, file_node and obj_node are for compressed dumping
     * and a index into the dump boolean table and fn_info_table */

    HChar*    name;
    UInt      number;
    FileNode* file;     /* reverse mapping for 2nd hash */
    FnNode*   next;

    Bool initialized    : 1;
    Bool dump_before    : 1;
    Bool dump_after     : 1;
    Bool zero_before    : 1;
    Bool toggle_collect : 1;
    Bool skip           : 1; // currently unused, see Callgrind
    Bool pop_on_jump    : 1; // currently unused, see Callgrind

    Bool is_malloc      : 1;
    Bool is_realloc     : 1;
    Bool is_free        : 1;

    Int  group;
    Int  separate_callers;
    Int  separate_recursions;
    /* TODO(someday) These are not used right now
     * see Callgrind */
};


//-------------------------------------------------------------------------------------------------
/** File function declarations **/

FnNode* GN_(getFnNode)(BBInfo *bb);
ObjNode* GN_(getObjNode)(DebugInfo *di);
ObjNode* GN_(getObjNodeByAddr)(Addr addr);
FileNode* GN_(getFileNode)(ObjNode *const obj, const HChar *dirname, const HChar *filename);
void GN_(initFn)(void);

#endif
