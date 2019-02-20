/* Taken from Callgrind */

#include "gn_clo.h"
#include "gn_fn.h"
#include "gn_bb.h"
#include "gn_debug.h"

//-------------------------------------------------------------------------------------------------
/** Static variable definitions **/
static ULong uniqueObjs;
static ULong uniqueFiles;
static ULong uniqueFns;
static ObjNode* allObjs[N_OBJ_ENTRIES];

static const HChar *anonname = "???";
static Addr runtimeResolveAddr = 0;
static int runtimeResolveLength = 0;
static const HChar *runtimeResolveName = "_dl_runtime_resolve";
/* N.B. see searchRuntimeResolve() */


struct chunk_t { int start, len; };
struct pattern
{
	// a code pattern is a list of tuples (start offset, length)
    const HChar* name;
    int len;
    struct chunk_t chunk[];
};



//-------------------------------------------------------------------------------------------------
/** Helper function definitions **/


static inline Addr bbAddr(BBInfo *bb) { return bb->offset + bb->obj->offset; }


#define HASH_CONSTANT (256)
static UInt strHash(const HChar *str, UInt tableSize)
{
    int hash = 0;
    for (; *str; str++)
        hash = (HASH_CONSTANT * hash + *str) % tableSize;
    return hash;
}


__attribute__((unused))    // Possibly;  depends on the platform.
static Bool checkCode(ObjNode* obj, UChar code[], struct pattern* pat)
{
	/* Scan for a pattern in the code of an ELF object.
	 * If found, return true and set runtime_resolve_{addr,length} */
	Bool found;
	Addr addr, end;
	int chunk, start, len;

	/* first chunk of pattern should always start at offset 0 and
	 * have at least 3 bytes */
	GN_ASSERT((pat->chunk[0].start == 0) && (pat->chunk[0].len >2));

	end = obj->start + obj->size - pat->len;
	addr = obj->start;
	while(addr < end) {
		found = (VG_(memcmp)( (void*)addr, code, pat->chunk[0].len) == 0);

		if (found) {
			chunk = 1;
			while(1) {
				start = pat->chunk[chunk].start;
				len   = pat->chunk[chunk].len;
				if (len == 0) break;

				GN_ASSERT(len >2);

				if (VG_(memcmp)( (void*)(addr+start), code+start, len) != 0) {
					found = False;
					break;
				}
				chunk++;
			}

			if (found) {
				if (VG_(clo_verbosity) > 1)
					VG_(message)(Vg_DebugMsg, "Found runtime_resolve (%s): "
								 "%s +%#lx=%#lx, length %d\n",
								 pat->name, obj->name + obj->last_slash_pos,
								 addr - obj->start, addr, pat->len);

				runtimeResolveAddr   = addr;
				runtimeResolveLength = pat->len;
				return True;
			}
		}
		addr++;
	}
	return False;
}


static Bool searchRuntimeResolve(ObjNode* obj)
{
	/* _ld_runtime_resolve, located in ld.so, needs special handling:
	 * The jump at end into the resolved function should not be
	 * represented as a call (as usually done in callgrind with jumps),
	 * but as a return + call. Otherwise, the repeated existence of
	 * _ld_runtime_resolve in call chains will lead to huge cycles,
	 * making the profile almost worthless.
	 *
	 * If ld.so is stripped, the symbol will not appear. But as this
	 * function is handcrafted assembler, we search for it.
	 *
	 * We stop if the ELF object name does not seem to be the runtime linker
	 */
#if defined(VGP_x86_linux)
	static UChar code[] = {
		/* 0*/ 0x50, 0x51, 0x52, 0x8b, 0x54, 0x24, 0x10, 0x8b,
		/* 8*/ 0x44, 0x24, 0x0c, 0xe8, 0x70, 0x01, 0x00, 0x00,
		/*16*/ 0x5a, 0x59, 0x87, 0x04, 0x24, 0xc2, 0x08, 0x00 };
	/* Check ranges [0-11] and [16-23] ([12-15] is an absolute address) */
	static struct pattern pat = {
		"x86-def", 24, {{ 0,12 }, { 16,8 }, { 24,0}} };

	/* Pattern for glibc-2.8 on OpenSuse11.0 */
	static UChar code_28[] = {
		/* 0*/ 0x50, 0x51, 0x52, 0x8b, 0x54, 0x24, 0x10, 0x8b,
		/* 8*/ 0x44, 0x24, 0x0c, 0xe8, 0x70, 0x01, 0x00, 0x00,
		/*16*/ 0x5a, 0x8b, 0x0c, 0x24, 0x89, 0x04, 0x24, 0x8b,
		/*24*/ 0x44, 0x24, 0x04, 0xc2, 0x0c, 0x00 };
	static struct pattern pat_28 = {
		"x86-glibc2.8", 30, {{ 0,12 }, { 16,14 }, { 30,0}} };

	if (VG_(strncmp)(obj->name, "/lib/ld", 7) != 0) return False;
	if (checkCode(obj, code, &pat)) return True;
	if (checkCode(obj, code_28, &pat_28)) return True;
	return False;
#endif

#if defined(VGP_ppc32_linux)
	static UChar code[] = {
		/* 0*/ 0x94, 0x21, 0xff, 0xc0, 0x90, 0x01, 0x00, 0x0c,
		/* 8*/ 0x90, 0x61, 0x00, 0x10, 0x90, 0x81, 0x00, 0x14,
		/*16*/ 0x7d, 0x83, 0x63, 0x78, 0x90, 0xa1, 0x00, 0x18,
		/*24*/ 0x7d, 0x64, 0x5b, 0x78, 0x90, 0xc1, 0x00, 0x1c,
		/*32*/ 0x7c, 0x08, 0x02, 0xa6, 0x90, 0xe1, 0x00, 0x20,
		/*40*/ 0x90, 0x01, 0x00, 0x30, 0x91, 0x01, 0x00, 0x24,
		/*48*/ 0x7c, 0x00, 0x00, 0x26, 0x91, 0x21, 0x00, 0x28,
		/*56*/ 0x91, 0x41, 0x00, 0x2c, 0x90, 0x01, 0x00, 0x08,
		/*64*/ 0x48, 0x00, 0x02, 0x91, 0x7c, 0x69, 0x03, 0xa6, /* at 64: bl aff0 <fixup> */
		/*72*/ 0x80, 0x01, 0x00, 0x30, 0x81, 0x41, 0x00, 0x2c,
		/*80*/ 0x81, 0x21, 0x00, 0x28, 0x7c, 0x08, 0x03, 0xa6,
		/*88*/ 0x81, 0x01, 0x00, 0x24, 0x80, 0x01, 0x00, 0x08,
		/*96*/ 0x80, 0xe1, 0x00, 0x20, 0x80, 0xc1, 0x00, 0x1c,
		/*104*/0x7c, 0x0f, 0xf1, 0x20, 0x80, 0xa1, 0x00, 0x18,
		/*112*/0x80, 0x81, 0x00, 0x14, 0x80, 0x61, 0x00, 0x10,
		/*120*/0x80, 0x01, 0x00, 0x0c, 0x38, 0x21, 0x00, 0x40,
		/*128*/0x4e, 0x80, 0x04, 0x20 };
	static struct pattern pat = {
		"ppc32-def", 132, {{ 0,65 }, { 68,64 }, { 132,0 }} };

	if (VG_(strncmp)(obj->name, "/lib/ld", 7) != 0) return False;
	return checkCode(obj, code, &pat);
#endif

#if defined(VGP_amd64_linux)
	static UChar code[] = {
		/* 0*/ 0x48, 0x83, 0xec, 0x38, 0x48, 0x89, 0x04, 0x24,
		/* 8*/ 0x48, 0x89, 0x4c, 0x24, 0x08, 0x48, 0x89, 0x54, 0x24, 0x10,
		/*18*/ 0x48, 0x89, 0x74, 0x24, 0x18, 0x48, 0x89, 0x7c, 0x24, 0x20,
		/*28*/ 0x4c, 0x89, 0x44, 0x24, 0x28, 0x4c, 0x89, 0x4c, 0x24, 0x30,
		/*38*/ 0x48, 0x8b, 0x74, 0x24, 0x40, 0x49, 0x89, 0xf3,
		/*46*/ 0x4c, 0x01, 0xde, 0x4c, 0x01, 0xde, 0x48, 0xc1, 0xe6, 0x03,
		/*56*/ 0x48, 0x8b, 0x7c, 0x24, 0x38, 0xe8, 0xee, 0x01, 0x00, 0x00,
		/*66*/ 0x49, 0x89, 0xc3, 0x4c, 0x8b, 0x4c, 0x24, 0x30,
		/*74*/ 0x4c, 0x8b, 0x44, 0x24, 0x28, 0x48, 0x8b, 0x7c, 0x24, 0x20,
		/*84*/ 0x48, 0x8b, 0x74, 0x24, 0x18, 0x48, 0x8b, 0x54, 0x24, 0x10,
		/*94*/ 0x48, 0x8b, 0x4c, 0x24, 0x08, 0x48, 0x8b, 0x04, 0x24,
		/*103*/0x48, 0x83, 0xc4, 0x48, 0x41, 0xff, 0xe3 };
	static struct pattern pat = {
		"amd64-def", 110, {{ 0,62 }, { 66,44 }, { 110,0 }} };

	if ((VG_(strncmp)(obj->name, "/lib/ld", 7) != 0) &&
		(VG_(strncmp)(obj->name, "/lib64/ld", 9) != 0)) return False;
	return checkCode(obj, code, &pat);
#endif

	/* For other platforms, no patterns known */
	return False;
}


static ObjNode* newObjNode(DebugInfo *di, ObjNode *next)
{
	ObjNode *obj = VG_(malloc)("gn.file.newobjnode.1", sizeof(ObjNode));
	obj->name = di ? VG_(strdup)("gn.file.newobjnode.2", VG_(DebugInfo_get_filename)(di))
		: anonname;

	for (UInt i=0; i<N_FILE_ENTRIES; i++) {
		obj->files[i] = NULL;
	}

	obj->number = uniqueObjs++;
	obj->start  = di ? VG_(DebugInfo_get_text_avma)(di) : 0;
	obj->size   = di ? VG_(DebugInfo_get_text_size)(di) : 0;
	obj->offset = di ? VG_(DebugInfo_get_text_bias)(di) : 0;
	obj->next   = next;

	obj->last_slash_pos = 0;
	for (UInt i=0; obj->name[i] != '\0'; i++) {
		if (obj->name[i] == '/')
			obj->last_slash_pos = i+1;
	}

	if (runtimeResolveAddr == 0)
		searchRuntimeResolve(obj);

	return obj;
}


static inline FileNode* newFileNode(const HChar *filename, ObjNode *const obj,
                                    FileNode *const next)
{
    FileNode *file = VG_(malloc)("gn.file.newfilenode.1", sizeof(FileNode));
    file->name = VG_(strdup)("gn.file.newfilenode.2", filename);
    for (UInt i=0; i<N_FN_ENTRIES; i++)
        file->fns[i] = NULL;

    file->number = uniqueFiles++;
    file->obj    = obj;
    file->next   = next;

    return file;
}


static inline FnNode* newFnNode(const HChar *fnname, FileNode *const file,
                                FnNode *const next)
{
    FnNode *fn = VG_(malloc)("gn.file.newfnnode.1", sizeof(FnNode));
    fn->name = VG_(strdup)("gn.file.newfnnode.2", fnname);

    fn->number   = uniqueFns++;
    fn->file     = file;
    fn->next     = next;

    // TODO(soon) are these needed?
    fn->dump_before    = False;
    fn->dump_after     = False;
    fn->zero_before    = False;
    fn->toggle_collect = False;
    fn->skip           = False;
    fn->pop_on_jump    = False;
    fn->is_malloc      = False;
    fn->is_free        = False;
    fn->is_realloc     = False;

    fn->group               = 0;

    return fn;
}


static Bool GN_(getDebugInfo)(/*IN*/  Addr instr,
                              /*OUT*/ const HChar **dirname,
                              /*OUT*/ const HChar **filename,
                              /*OUT*/ const HChar **fnname,
                              /*Optional OUT*/ DebugInfo **di)
{
    Bool result = True;
    UInt lineno;
    DiEpoch ep = VG_(current_DiEpoch)();

    if (di)
        *di = VG_(find_DebugInfo)(ep, instr);

    Bool foundFn = VG_(get_fnname)(ep, instr, fnname);
    Bool foundFileLine = VG_(get_filename_linenum)(ep, instr, filename, dirname, &lineno);

    if (!foundFn && !foundFileLine) {
        *filename = anonname;
        *fnname = anonname;
        result = False;
    }
    else if (!foundFn && foundFileLine) {
        *fnname = anonname;
    }
    else if (foundFn && !foundFileLine) {
        *filename = anonname;
    }

    return result;
}


static inline const HChar* getUnknownFnName(const BBInfo *bb)
{
    // just use the address if no name found
    static HChar buf[32];
    GN_ASSERT(sizeof(Addr) == 8);
    int p = VG_(sprintf)(buf, "%#016lx", (UWord)(bb->offset));
    VG_(sprintf)(buf+p, "%s",
                 (bb->sectKind == Vg_SectData) ? "[Data]" :
                 (bb->sectKind == Vg_SectBSS)  ? " [BSS]" :
                 (bb->sectKind == Vg_SectGOT)  ? " [GOT]" :
                 (bb->sectKind == Vg_SectPLT)  ? " [PLT]" : "");
    return buf;
}


static inline FnNode* getFnNodeInFile(FileNode *const file, const HChar *fnname)
{
    UInt fnnamehash = strHash(fnname, N_FN_ENTRIES);
    FnNode *fn = file->fns[fnnamehash];

    while (fn) {
        if (VG_(strcmp)(fnname, fn->name) == 0)
            break;
        fn = fn->next;
    }

    if (fn == NULL) {
        fn = newFnNode(fnname, file, file->fns[fnnamehash]);
        file->fns[fnnamehash] = fn;
    }

    return fn;
}


static inline FnNode* getFnNodeInSeg(DebugInfo *const di,
                                     const HChar *dirname,
                                     const HChar *filename,
                                     const HChar *fnname)
{
    ObjNode *obj = GN_(getObjNode)(di);
    FileNode *file = GN_(getFileNode)(obj, dirname, filename);
    FnNode *fn = getFnNodeInFile(file, fnname);

    return fn;
}


//-------------------------------------------------------------------------------------------------
/** File function definitions - External **/

FileNode* GN_(getFileNode)(ObjNode *const obj,
                           const HChar *dirname,
                           const HChar *filename)
{
    /* absolute path */
    HChar abspath[VG_(strlen)(dirname) + 1 + VG_(strlen)(filename) + 1];
    VG_(strcpy)(abspath, dirname);
    if (filename[0] != '\0')
        VG_(strcat)(abspath, "/");
    VG_(strcat)(abspath, filename);

    /* hash */
    UInt filenamehash = strHash(filename, N_FILE_ENTRIES);
    FileNode *file = obj->files[filenamehash];

    while (file) {
        if (VG_(strcmp)(filename, file->name) == 0)
            break;
        file = file->next;
    }

    if (file == NULL) {
        file = newFileNode(filename, obj, obj->files[filenamehash]);
        obj->files[filenamehash] = file;
    }

    return file;
}


ObjNode* GN_(getObjNode)(DebugInfo *di)
{
    ObjNode *obj;
    UInt namehash;
    const HChar *name;

    name = di ? VG_(DebugInfo_get_filename)(di) : anonname;
    namehash = strHash(name, N_OBJ_ENTRIES);
    obj = allObjs[namehash];

    while (obj) {
        if (VG_(strcmp)(name, obj->name) == 0)
            break;
        obj = obj->next;
    }

    if (obj == NULL) {
        obj = newObjNode(di, allObjs[namehash]);
        allObjs[namehash] = obj;
    }

    return obj;
}


ObjNode* GN_(getObjNodeByAddr)(Addr addr)
{
    ObjNode *obj;
    DebugInfo *di;
    PtrdiffT offset;

    DiEpoch ep = VG_(current_DiEpoch)();
    di = VG_(find_DebugInfo)(ep, addr);
    obj = GN_(getObjNode)(di);

	// check if object was remapped
	offset = di ? VG_(DebugInfo_get_text_bias)(di) : 0;
	if (obj->offset != offset) {
		Addr start = di ? VG_(DebugInfo_get_text_avma)(di) : 0;
		GN_ASSERT(obj->size == (di ? VG_(DebugInfo_get_text_size)(di) : 0));
		GN_ASSERT((PtrdiffT)(obj->start - start) == obj->offset - offset);
		obj->offset = offset;
		obj->start = start;
	}

	return obj;
}


FnNode* GN_(getFnNode)(BBInfo *bb)
{
    /* Return if already cached
     * This should be the common path */
    if (bb->fn)
        return bb->fn;

    /* Else, lookup function info in binary */

    const HChar *fnname, *filename, *dirname;
    DebugInfo *di;

    /* Find debug info for function  */
    GN_(getDebugInfo)(bbAddr(bb), &dirname, &filename, &fnname, &di);

    DiEpoch ep = VG_(current_DiEpoch)();
    if (VG_(strcmp)(fnname, anonname) == 0)
        fnname = getUnknownFnName(bb); // name correction
    else if (VG_(get_fnname_if_entry)(ep, bbAddr(bb), &fnname))
        bb->isFnEntry = True;

    /* HACK for correct _exit:
     * _exit is redirected to VG_(__libc_freeres_wrapper) by valgrind,
     * so we rename it back again :-) */
    static BBInfo *exitBB = NULL;
    if ((VG_(strcmp)(fnname, "vgPlain___libc_freeres_wrapper") == 0) && (exitBB != NULL))
        GN_(getDebugInfo)(bbAddr(exitBB), &dirname, &filename, &fnname, &di);
    if ((VG_(strcmp)(fnname, "_exit") == 0) && (exitBB == NULL))
        exitBB = bb;

    /* Special handling for dynamic library resolution function */
    if (runtimeResolveAddr &&
        (bbAddr(bb) >= runtimeResolveAddr) &&
        (bbAddr(bb) < runtimeResolveAddr + runtimeResolveLength)) {
        fnname = runtimeResolveName;
    }

    /* Look up function metadata node */
    FnNode *fn = getFnNodeInSeg(di, dirname, filename, fnname);

    /* Last initialization step requiring BBInfo */
    if (fn->initialized == False) {

        if (bb->sectKind == Vg_SectPLT)
            fn->skip = GN_(clo).skip_plt;

        if (VG_(strcmp)(fn->name, runtimeResolveName) == 0)
            fn->pop_on_jump = True;

        fn->is_malloc  = (VG_(strcmp)(fn->name, "malloc") == 0);
        fn->is_realloc = (VG_(strcmp)(fn->name, "realloc") == 0);
        fn->is_free    = (VG_(strcmp)(fn->name, "free") == 0);

        /* TODO(soon) update fn config (dump before/after, toggle collect, et al) */

        fn->initialized = True;
    }

    bb->fn = fn;
    return fn;
}


void GN_(initFn)()
{
    uniqueObjs = 0;
    uniqueFiles = 0;
    uniqueFns = 0;

    for (UInt i=0; i<N_OBJ_ENTRIES; ++i)
        allObjs[i] = NULL;
}
