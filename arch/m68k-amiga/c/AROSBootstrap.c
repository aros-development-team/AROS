/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

/* This loads relocatable AROS ROM ELF images, and gzipped
 * image. Use either the aros-amiga-m68k-reloc.elf or
 * aros.elf.gz images. The fully linked aros-amiga-m68k.elf
 * image cannot be loaded by this application.
 *
 * As you can probably guess, you will need at least 1MB of
 * extra free RAM to get this to work. 
 *
 * Also - no AROS specific code can go in here! We have to run
 * on AOS 1.3 and up.
 */
#define DEBUG 1

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/expansion.h>
#include <exec/resident.h>
#include <aros/kernel.h>
#include <hardware/cpu/memory.h>
#include <libraries/configvars.h>

/* This much memory is wasted in single reset proof allocation */
#define ALLOCATION_EXTRA (sizeof(struct MemChunk) + sizeof(struct MemList))
#define ALLOCPADDING (sizeof(struct MemChunk) + 2 * sizeof(BPTR))

#define SS_STACK_SIZE	0x2000
#define MAGIC_FAST_SIZE 65536
/* This must match with start.c! */
#define ABS_BOOT_MAGIC 0x4d363802
struct BootStruct
{
    ULONG magic;
    struct TagItem *kerneltags;
    APTR ss_address;
    LONG ss_size;
    APTR magicfastmem;
    LONG magicfastmemsize;
};

#include <stddef.h> /* offsetof */
#include <string.h> /* memcpy, memset */
#include <zlib.h>

#if defined(DEBUG) && DEBUG > 1
#define DEFAULT_KERNEL_CMDLINE "sysdebug=InitCode mungwall"
#else
#define DEFAULT_KERNEL_CMDLINE "sysdebug=InitCode"
#endif

#define PROTO_KERNEL_H      /* Don't pick up AROS kernel hooks */
#define NO_SYSBASE_REMAP

#if DEBUG
#define AROS_DEBUG_H
#include <stdio.h>
#include <string.h>
static inline void bug(const char *fmt, ...)
{
    static char buff[256];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buff, sizeof(buff), fmt, args);
    va_end(args);

    Write(Output(), buff, strlen(buff));
}
#define D(x) x
#else
#define D(x) 
#endif

#include <loadseg.h>

struct DosLibrary *DOSBase;
struct Library *ExpansionBase;

static BOOL ROM_Loaded = FALSE;
static BOOL forceCHIP = FALSE;
static BOOL forceFAST = FALSE;
static BOOL debug_enabled = FALSE;
static struct List mlist;

/* KS 1.3 (and earlier) don't have a dos.library with
 * niceties such as VFPrintf nor ReadArgs.
 *
 * We need to use the BCPL routines to be able
 * to do these types of operations.
 */
#define BCPL_WriteS 73
#define BCPL_WriteF 74
#define BCPL_RdArgs 78

#undef  Printf
#define Printf    __Printf_NOT_AVAILABLE_UNDER_KS1_3
#undef  ReadArgs
#define ReadArgs  __ReadArgs_NOT_AVAILABLE_UNDER_KS1_3

/* BCPL can trash D5-D7 and A3, evidently.
 */
static void bcplWrapper(void)
{
    asm volatile (
    	"movem.l %d5-%d7/%a3,%sp@-\n"
    	"jsr (%a5)\n"
    	"movem.l %sp@+,%d5-%d7/%a3\n"
    );
}

static ULONG doBCPL(int index, ULONG d1, ULONG d2, ULONG d3, ULONG d4, const IPTR *arg, int args)
{
    struct Process *pr = (APTR)FindTask(NULL);
    APTR  func;
    ULONG *gv = pr->pr_GlobVec;
    ULONG ret;
    ULONG *BCPL_frame = AllocMem(1500, MEMF_ANY);
    if (BCPL_frame == NULL)
    	return 0;

    func = (APTR)gv[index];

    if (args != 0)
    	CopyMem(arg, &BCPL_frame[3 + 4], args * sizeof(ULONG));

    ret = AROS_UFC11(ULONG, bcplWrapper,
    	    AROS_UFCA(ULONG, 0, D0),  /* BCPL frame usage (args-3)*/
    	    AROS_UFCA(ULONG, d1, D1),
    	    AROS_UFCA(ULONG, d2, D2),
    	    AROS_UFCA(ULONG, d3, D3),
    	    AROS_UFCA(ULONG, d4, D4),
    	    AROS_UFCA(ULONG, 0,  A0),    /* System memory base */
    	    AROS_UFCA(ULONG *, &BCPL_frame[3], A1),
    	    AROS_UFCA(APTR, gv, A2),
    	    AROS_UFCA(APTR, func, A4),
    	    AROS_UFCA(APTR, DOSBase->dl_A5, A5),
    	    AROS_UFCA(APTR, DOSBase->dl_A6, A6));

    FreeMem(BCPL_frame, 1500);

    return ret;
}

#if DEBUG
static BSTR ConvertCSTR(const UBYTE *name)
{
    UBYTE *bname = AllocMem(256 + 1, MEMF_CLEAR);
    UWORD len = strlen(name), i;
    
    if (len > 255)
    	len = 255;
    bname[0] = len;
    strcpy(bname + 1, name);
    for (i = 0; i < len; i++) {
    	if (bname[1 + i] == 13 || bname[1 + i] == 10)
    	    bname[i + 1] = ' ';
    }
    return MKBADDR(bname);
}
static void FreeBSTR(BSTR bstr)
{
    FreeMem(BADDR(bstr), 256 + 1);
}
#endif

static UBYTE *ConvertBSTR(BSTR bname)
{
    UBYTE *name = BADDR(bname);
    UBYTE *s = AllocMem(256 + 1, MEMF_CLEAR);
    if (!s)
    	return NULL;
    CopyMem(name + 1, s, name[0]);
    return s;
}

static void FreeString(UBYTE *cstr)
{
    FreeMem(cstr, 256+1);
}

static void _WriteF(BPTR bfmt, ...)
{
   IPTR *args = (IPTR *)&bfmt;

   doBCPL(BCPL_WriteF, bfmt, args[1], args[2], args[3], &args[4], 26-3);
}

#if DEBUG
static void _DWriteF(BPTR bfmt, ...)
{
   if (debug_enabled || DEBUG > 1) {
      IPTR *args = (IPTR *)&bfmt;

      doBCPL(BCPL_WriteF, bfmt, args[1], args[2], args[3], &args[4], 26-3);
   }
}
#define DWriteF(fmt, args...) _DWriteF(AROS_CONST_BSTR(fmt) ,##args )
#endif

#define WriteF(fmt, args...) _WriteF(AROS_CONST_BSTR(fmt) ,##args )

/* For KS < 2.0, we need to call the BCPL ReadArgs,
 * since DOS/ReadArgs doesn't exist.
 */
static ULONG RdArgs(BSTR format, BPTR args, ULONG max_arg)
{
    return doBCPL(BCPL_RdArgs, format, args, max_arg, 0, NULL, 0);
}

void meminfo(void)
{
    struct MemHeader *mh;
    ForeachNode(&SysBase->MemList, mh) {
        char bstr[256];
        bstr[0] = strlen(mh->mh_Node.ln_Name) & 0xff;
        strncpy(&bstr[1], mh->mh_Node.ln_Name, bstr[0]);
        WriteF("@$%X8-$%X8 ATTR $%X4 FREE $%X8 (%S)\n",
                mh->mh_Lower, mh->mh_Upper, mh->mh_Attributes,
                mh->mh_Free, MKBADDR(bstr));
    }
}

/* Allocate MMU page aligned memory chunks */

static APTR AllocPageAligned(ULONG *psize, ULONG flags)
{
    APTR ret;
    ULONG size;

    size = *psize;
    size += ALLOCPADDING;
    ret = AllocMem(size + 2 * PAGE_SIZE, flags);
    D(DWriteF("AllocPageAligned: $%X8, %X4 => %X8\n", size + 2 * PAGE_SIZE, flags, ret));
    if (ret == NULL)
    	return NULL;
    Forbid();
    FreeMem(ret, size + 2 * PAGE_SIZE);
    size = (size + PAGE_SIZE - 1) & PAGE_MASK;
    ret = AllocAbs(size, (APTR)(((((ULONG)ret) + PAGE_SIZE - 1) & PAGE_MASK)));
    Permit();
    if (ret == NULL)
    	return NULL;
    *psize = size;
    return ret;
}
static void FreePageAligned(APTR addr, ULONG size)
{
    FreeMem(addr, (size + PAGE_SIZE - 1) & PAGE_MASK);
}

/* Define these here for zlib so that we don't
 * pull in arosc.library.
 *
 * We can't use AllocVec, since it's only
 * been around since KS v36
 */
void *malloc(int size)
{
    ULONG *vec;

    size += sizeof(ULONG);

    vec = AllocMem(size, MEMF_ANY);
    if (vec == NULL) {
    	WriteF("libz: Failed to allocate %N bytes of type %X8\n", size, MEMF_ANY);
    	return NULL;
    }

    vec[0] = (ULONG)size;
    return &vec[1];
}

void free(void *ptr)
{
    ULONG *vec = ptr - sizeof(ULONG);
    FreeMem(vec, vec[0]);
}

int open(const char *name, int mode)
{
    return (int)Open(name, MODE_OLDFILE);
}

void close(int fd)
{
    Close((BPTR)fd);
}

int read(int fd, void *buff, size_t len)
{
    return Read((BPTR)fd, buff, (LONG)len);
}

off_t lseek(int fd, off_t offset, int whence)
{
    LONG mode = SEEK_SET;
    LONG err;

    switch (whence) {
    case SEEK_CUR: mode = OFFSET_CURRENT; break;
    case SEEK_SET: mode = OFFSET_BEGINNING; break;
    case SEEK_END: mode = OFFSET_END; break;
    default: return -1;
    }

    err = Seek((BPTR)fd, (LONG)offset, mode);
    if (err < 0)
    	return -1;

    return Seek((BPTR)fd, 0, OFFSET_CURRENT);       
}

static AROS_UFH4(LONG, aosRead,
	AROS_UFHA(BPTR,  file, D1),
	AROS_UFHA(void *, buf, D2),
	AROS_UFHA(LONG,  size, D3),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    return Read(file, buf, (unsigned)size);

    AROS_USERFUNC_EXIT
}
static AROS_UFH4(LONG, aosSeek,
	AROS_UFHA(BPTR,  file, D1),
	AROS_UFHA(LONG,   pos, D2),
	AROS_UFHA(LONG,  mode, D3),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT
    int whence;
    LONG ret;
    LONG oldpos;

    switch (mode) {
    case OFFSET_CURRENT  : whence = SEEK_CUR; break;
    case OFFSET_END      : whence = SEEK_END; break;
    case OFFSET_BEGINNING: whence = SEEK_SET; break;
    default: return -1;
    }

    oldpos = (LONG)Seek(file, 0, SEEK_CUR);

    ret = (LONG)Seek(file, (z_off_t)pos, whence);
    if (ret < 0)
    	return -1;

    return oldpos;

    AROS_USERFUNC_EXIT
}

static APTR aosAllocMem(ULONG size, ULONG flags, struct ExecBase *SysBase)
{
    struct MemList *ml;
    UBYTE *mem;

    /* Clear bits 15-0, we're setting memory class explicitly */
    flags &= ~0x7fff;

    if (SysBase->LibNode.lib_Version >= 36) {
    	flags |= MEMF_LOCAL | MEMF_REVERSE;
    } else {
    	flags |= MEMF_CHIP;
    }

    size += ALLOCATION_EXTRA;
    mem = AllocMem(size, flags | MEMF_CLEAR);
    if (mem == NULL) {
    	WriteF("AOS: Failed to allocate %N bytes of type %X8\n", size, flags);
    	meminfo();
    	return NULL;
    }

    ml = (struct MemList*)(mem + sizeof(struct MemChunk));
    ml->ml_NumEntries = 1;
    ml->ml_ME[0].me_Addr = (APTR)mem;
    ml->ml_ME[0].me_Length = size;
    AddTail(&mlist, (struct Node*)ml);

    return &ml[1];
}

static AROS_UFH3(APTR, aosAlloc,
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(ULONG, flags, D1),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    
    return aosAllocMem(size, flags, SysBase);

    AROS_USERFUNC_EXIT
}
static AROS_UFH3(void, aosFree,
	AROS_UFHA(APTR, addr, A1),
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    
    addr -= sizeof(struct MemList);
    Remove((struct Node*)addr);
    addr -= sizeof(struct MemChunk);
    size += ALLOCATION_EXTRA;
    FreeMem(addr, size);

    AROS_USERFUNC_EXIT
}

/* Backcalls for LoadSegment
 * using the gzip backend.
 */
static AROS_UFH4(LONG, elfRead,
	AROS_UFHA(BPTR,  file, D1),
	AROS_UFHA(void *, buf, D2),
	AROS_UFHA(LONG,  size, D3),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT

    return gzread((gzFile)file, buf, (unsigned)size);

    AROS_USERFUNC_EXIT
}
static AROS_UFH4(LONG, elfSeek,
	AROS_UFHA(BPTR,  file, D1),
	AROS_UFHA(LONG,   pos, D2),
	AROS_UFHA(LONG,  mode, D3),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))
{
    AROS_USERFUNC_INIT
    int whence;
    LONG ret;
    LONG oldpos;

    switch (mode) {
    case OFFSET_CURRENT  : whence = SEEK_CUR; break;
    case OFFSET_END      : whence = SEEK_END; break;
    case OFFSET_BEGINNING: whence = SEEK_SET; break;
    default: return -1;
    }

    oldpos = (LONG)gzseek((gzFile)file, 0, SEEK_CUR);

    ret = (LONG)gzseek((gzFile)file, (z_off_t)pos, whence);
    if (ret < 0)
    	return -1;

    return oldpos;

    AROS_USERFUNC_EXIT
}

static APTR specialAlloc(ULONG size, ULONG flags, struct ExecBase *SysBase)
{
    APTR mem;
    struct MemList *ml;

    D(DWriteF("ELF: Attempt to allocate %N bytes of type %X8\n", size, flags));
    /* Since we don't know if we need to wrap the memory
     * with the KickMem wrapper until after allocation,
     * we always adjust the size as if we have to.
     */
    size += ALLOCATION_EXTRA;

    if (flags & MEMF_KICK) {
        D(DWriteF("MEMF_KICK %N\n", size));
        if (forceCHIP) {
            flags |= MEMF_CHIP;
        } else {
            /* Prefer MEMF_KICK | MEMF_FAST if available */
            flags |= MEMF_FAST;
        }
    }

    /* Hmm. MEMF_LOCAL is only available on v36 and later.
     * Use MEMF_CHIP if we have to.
     */
    if (flags & MEMF_LOCAL) {
        D(DWriteF("MEMF_LOCAL %N\n", size));
        if (forceFAST) {
            flags &= ~MEMF_LOCAL;
        } else if (SysBase->LibNode.lib_Version < 36 || forceCHIP) {
            flags &= ~MEMF_LOCAL;
            flags |= MEMF_CHIP;
        }
    }

    /* MEMF_31BIT is not available on AOS */
    if ((flags & MEMF_31BIT)) {
    	flags &= ~MEMF_31BIT;
    }

    /* If ROM allocation, always allocate from top of memory if possible */
    if ((flags & MEMF_PUBLIC) && SysBase->LibNode.lib_Version >= 36) {
    	flags |= MEMF_REVERSE;
    }

    D(DWriteF("ELF: Attempt to allocate %N bytes of type %X8\n", size, flags));
    mem = AllocPageAligned(&size, flags | MEMF_CLEAR);
    if (mem == NULL) {
        if ((flags & (MEMF_KICK | MEMF_FAST)) == (MEMF_KICK | MEMF_FAST)) {
            /* Couldn't allocate MEMF_KICK | MEMF_FAST, fall back to any memory */
            mem = AllocPageAligned(&size, MEMF_CLEAR);
        }
        if (mem == NULL) {
    	    D(DWriteF("ELF: Failed to allocate %N bytes of type %X8\n", size, flags));
    	    meminfo();
    	    return NULL;
        }
    }
    D(DWriteF("ELF: Got memory at %X8, size %N\n", (IPTR)mem, size));

    ml = (struct MemList*)(mem + sizeof(struct MemChunk));
    ml->ml_NumEntries = 1;
    ml->ml_ME[0].me_Addr = (APTR)mem;
    ml->ml_ME[0].me_Length = size;

    /* Add to the KickMem list */
    AddTail(&mlist, (struct Node*)ml);

    return &ml[1];

}

static AROS_UFH3(APTR, elfAlloc,
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(ULONG, flags, D1),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    
    return specialAlloc(size, flags, SysBase);

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(void, elfFree,
	AROS_UFHA(APTR, addr, A1),
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    /* If not page aligned, and the offset from the page boundary
     * is the sizeof(MemChunk) + sizeof(MemList) then we can assume
     * that it was a KickTag protected allocation
     */
    D(DWriteF("ELF: Free memory at %X8, size %N\n", (IPTR)addr, size));
    addr -= sizeof(struct MemList);
    Remove((struct Node*)addr);
    addr -= sizeof(struct MemChunk);
    size += ALLOCATION_EXTRA;
    D(DWriteF("ELF: FREE memory at %X8, size %N\n", (IPTR)addr, size));
    FreePageAligned(addr, size);

    AROS_USERFUNC_EXIT
}

/*
 * This routine is called from within libloadseg.a's ELF loader.
 * In dos.library it's responsible for collecting debug information from the loaded file.
 * Here it does nothing (FIXME ???)
 */
void register_elf(BPTR file, BPTR hunks, struct elfheader *eh, struct sheader *sh, struct DosLibrary *DOSBase)
{

}

static BPTR ROMLoad(BSTR bfilename)
{
    gzFile gzf;
    UBYTE *filename;
    BPTR rom = BNULL;
    SIPTR funcarray[] = {
    	(SIPTR)elfRead,
    	(SIPTR)elfAlloc,
    	(SIPTR)elfFree,
    	(SIPTR)elfSeek,
    };
    filename = ConvertBSTR(bfilename);
    if (!filename)
    	return BNULL;

    WriteF("Loading '%S' into RAM...\n", bfilename);
    if ((gzf = gzopen(filename, "rb"))) {
    	gzbuffer(gzf, 65536);

    	rom = LoadSegment((BPTR)gzf, BNULL, funcarray, NULL);
    	if (rom == BNULL) {
    	    WriteF("'%S': Can't parse, error %N\n", bfilename, IoErr());
    	}

    	gzclose_r(gzf);
    } else {
    	WriteF("'%S': Can't open\n", bfilename);
    }
    FreeString(filename);
    return rom;
}

/* Patch "picasso96/<driver>.chip" -> "<driver>.chip" so that OpenLibrary() finds it */
static void RTGPatch(struct Resident *r, BPTR seg)
{
    BOOL patched = FALSE;
    WORD len = strlen(r->rt_Name);
    const UBYTE *name = r->rt_Name + len - 5;
    if (len > 5 && (!stricmp(name, ".card") || !stricmp(name, ".chip"))) {
    	BPTR seglist = seg;
	while (seglist) {
	    ULONG *ptr = BADDR(seglist);
	    LONG len = ptr[-1] - sizeof(BPTR);
	    UBYTE *p = (UBYTE*)(ptr + 1);
    	    while (len > 0) {
    	    	if (len > 16 && !strnicmp(p, "libs:picasso96/", 15)) {
    	    	    memmove(p, p + 15, strlen(p + 15) + 1);
    	    	    patched = TRUE;
    	    	} else if (len > 10 && !strnicmp(p, "picasso96/", 10)) {
    	    	    memmove(p, p + 10, strlen(p + 10) + 1);
    	    	    patched = TRUE;
    	    	}
    	    	len--;
    	    	p++;
    	    }
	    seglist = *((BPTR*)BADDR(seglist));
	}
    }
    if (patched)
    	WriteF("Library path patched\n");
}

static BOOL PatchResidents(BPTR seg)
{
    const LONG ressize = offsetof(struct Resident, rt_Init) + sizeof(APTR);
    while(seg) {
	ULONG *ptr = BADDR(seg);
	UWORD *res;
	UWORD *end = (UWORD*)((ULONG)ptr + ptr[-1] - ressize);
    
	res = (UWORD*)(ptr + 1);
	while (res < end) {
	    if (*res == RTC_MATCHWORD && ((ULONG*)(res + 1))[0] == (ULONG)res) {
		struct Resident *r = (struct Resident*)res;
		r->rt_Flags |= 1 << 5;
		if (r->rt_EndSkip <= (APTR)res) {
#if DEBUG > 1
                    WriteF("Invalid rt_EndSkip: %X8, Resident: %X8\n", r->rt_EndSkip, r);
#endif
                    res += ressize / sizeof(WORD) - 1;
		} else {
                    res = (UWORD*)r->rt_EndSkip - 1;
                }
	    }
	    res++;
	}
	seg = (BPTR)ptr[0];
    }
    return TRUE;
}

#define RESLIST_CHUNK   100
#define LRF_NOPATCH      (1 << 0)        /* If set, don't patch the struct Resident */

static struct Resident **LoadResident(BPTR seg, struct Resident **reslist, ULONG *resleft, ULONG flags)
{
    const LONG ressize = offsetof(struct Resident, rt_Init) + sizeof(APTR);
    ULONG *ptr = BADDR(seg);
    UWORD *res;
    UWORD *end = (UWORD*)((ULONG)ptr + ptr[-1] - ressize);
    
    res = (UWORD*)(ptr + 1);
    while (res < end) {
        if (*res == RTC_MATCHWORD && ((ULONG*)(res + 1))[0] == (ULONG)res) {
            struct Resident *r = (struct Resident*)res;
            if (!(flags & LRF_NOPATCH)) {
                /* Set RTF_COLDSTART if no initialization flags set */
                if (!(r->rt_Flags & (RTF_COLDSTART | RTF_SINGLETASK | RTF_AFTERDOS)))
                    r->rt_Flags |= RTF_COLDSTART;
                if (r->rt_Pri < 10)
                    r->rt_Pri = 10;
                RTGPatch(r, seg);
            }

            if (*resleft == 0) {
                struct Resident **resnew;
                resnew = aosAllocMem(RESLIST_CHUNK * sizeof(struct Resident*), MEMF_CLEAR, SysBase);
                if (!resnew)
                    return 0;

                *reslist = (APTR)((IPTR)resnew | RESLIST_NEXT);
                reslist = resnew;
                *resleft = RESLIST_CHUNK - 1;
            }
            D(DWriteF("Resident structure found @%X8\n", r));
            *(reslist++) = r;
            (*resleft)--;

            if (r->rt_EndSkip <= (APTR)res)
		res += ressize / sizeof(WORD) - 1;
            else
                res = (UWORD*)r->rt_EndSkip - 1;
        }
        res++;
    }

    return reslist;
}

static struct Resident **LoadResidents(BPTR *namearray, struct Resident **resnext, ULONG *resleft)
{
    UBYTE i;
    SIPTR funcarray[] = {
    	(SIPTR)aosRead,
    	(SIPTR)aosAlloc,
    	(SIPTR)aosFree,
    	(SIPTR)aosSeek,
    };
   
    for (i = 0; namearray[i]; i++) {
	LONG stack;
	BPTR handle;
	BPTR seglist = BNULL;
	BPTR bname;
    	UBYTE *name;
    
    	bname = namearray[i];
    	name = ConvertBSTR(bname);
    	if (name) {
	    handle = Open(name, MODE_OLDFILE);
	    if (handle) {
		seglist = LoadSegment(handle, BNULL, funcarray, &stack);
		Close(handle);
	    }
	    if (seglist) {
	        BPTR seg;
		WriteF("Loaded '%S'\n", bname);
		for (seg = seglist; seg != BNULL; seg = *((BPTR*)BADDR(seg)))
		    resnext = LoadResident(seg, resnext, resleft, 0);
	    } else {
		WriteF("Failed to load '%S', error %N\n", bname, IoErr());
	    }
	    FreeString(name);
	}
    }

    return resnext;
}

#if DEBUG

#define SERDATR			0x18
#define SERDAT			0x30
#define INTREQ			0x9c
#define INTENA			0x9a
#define SERDATR_TBE		(1 << 13)	/* Tx Buffer Empty */
#define SERDAT_STP8		(1 << 8)
#define SERDAT_DB8(x)		((x) & 0xff)
#define SERPER_BASE_PAL		3546895
#define SERPER			0x32
#define SERPER_BAUD(base, x)	((((base + (x)/2))/(x)-1) & 0x7fff)	/* Baud rate */
#define INTF_TBE		0x0001

static inline void reg_w(ULONG reg, UWORD val)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	*r = val;
}
static inline UWORD reg_r(ULONG reg)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	return *r;
}
static void DebugInit(void)
{
	/* Set DTR, RTS, etc */
	volatile UBYTE *ciab_pra = (APTR)0xBFD000;
	volatile UBYTE *ciab_ddra = (APTR)0xBFD200;
	if (!debug_enabled)
		return;
	*ciab_ddra = 0xc0;  /* Only DTR and RTS are driven as outputs */
	*ciab_pra = 0;      /* Turn on DTR and RTS */

	/* Set the debug UART to 115200 */
	reg_w(SERPER, SERPER_BAUD(SERPER_BASE_PAL, 115200));
	/* Disable serial transmit interrupt */
	reg_w(INTENA, INTF_TBE);
}
static void DebugPutChar(register int chr)
{
	if (!debug_enabled)
		return;
	if (chr == '\n')
		DebugPutChar('\r');
	while ((reg_r(SERDATR) & SERDATR_TBE) == 0);
	reg_w(INTREQ, INTF_TBE);
	/* Output a char to the debug UART */
	reg_w(SERDAT, SERDAT_STP8 | SERDAT_DB8(chr));
}
static void DebugPutStr(register const char *buff)
{
	if (!debug_enabled)
		return;
	for (; *buff != 0; buff++)
		DebugPutChar(*buff);
}
#if 0
static void DebugPutDec(const char *what, ULONG val)
{
	int i, num;
	if (!debug_enabled)
		return;
	DebugPutStr(what);
	DebugPutStr(": ");
	if (val == 0) {
	    DebugPutChar('0');
	    DebugPutChar('\n');
	    return;
	}

	for (i = 1000000000; i > 0; i /= 10) {
	    if (val == 0) {
	    	DebugPutChar('0');
	    	continue;
	    }

	    num = val / i;
	    if (num == 0)
	    	continue;

	    DebugPutChar("0123456789"[num]);
	    val -= num * i;
	}
	DebugPutChar('\n');
}
static void DebugPutHex(const char *what, ULONG val)
{
	int i;
	if (!debug_enabled)
		return;
	DebugPutStr(what);
	DebugPutStr(": ");
	for (i = 0; i < 8; i ++) {
		DebugPutChar("0123456789abcdef"[(val >> (28 - (i * 4))) & 0xf]);
	}
	DebugPutChar('\n');
}
static void DebugPutHexVal(ULONG val)
{
	int i;
	if (!debug_enabled)
		return;
	for (i = 0; i < 8; i ++) {
		DebugPutChar("0123456789abcdef"[(val >> (28 - (i * 4))) & 0xf]);
	}
	DebugPutChar(' ');
}
#endif
#endif


#define FAKEBASE 0x200
#define FAKEBASESIZE 558
#define COLDCAPTURE (FAKEBASE + FAKEBASESIZE + 16)

/* Theory of operation:

- create fake sysbase in low chip memory (We can't use original because it is not binary compatible with AROS one)
- set correct checksum and point ColdCapture to our routine (also in low chip)
- reset the system (autoconfig devices disappear, including most expansion RAM and ROM overlay disables chip RAM)
- original ROM code now disables ROM overlay and checks KS checksum and jumps to our ColdCapture routine
- above step is needed because in worst case ALL RAM disappear at reset and there
  are also accelerators that reset the CPU completely when reset instruction is executed.
- now we have control again, autoconfig devices are still unconfigured but at least we have chip ram.
- jump to AROS ROM code entry point which detects romloader mode and automatically reserves RAM used by "ROM" code
- AROS ROM creates new proper execbase and copies ColdCapture
- normal boot starts

*/

static UWORD GetSysBaseChkSum(struct ExecBase *sysbase)
{
     UWORD sum = 0;
     UWORD *p = (UWORD*)&sysbase->SoftVer;
     while (p <= &sysbase->ChkSum)
	sum += *p++;
     return sum;
}

static ULONG mySumKickData(struct ExecBase *sysbase, BOOL output)
{
    ULONG chksum = 0;
    BOOL isdata = FALSE;

    if (sysbase->KickTagPtr) {
    	IPTR *list = sysbase->KickTagPtr;
 	while(*list)
	{
   	    chksum += (ULONG)*list;
#if 0
	    if (output) {
	    	WriteF("%X8 %X8\n", list, *list);
	    	WriteF("CHK %X8\n", chksum);
	    }
#endif

            /* on amiga, if bit 31 is set then this points to another list of
             * modules rather than pointing to a single module. bit 31 is
             * inconvenient on architectures where code may be loaded above
             * 2GB. on these platforms we assume aligned pointers and use bit
             * 0 instead */
#ifdef __mc68000__
	    if(*list & 0x80000000) { list = (IPTR *)(*list & 0x7fffffff); continue; }
#else
            if(*list & 0x1) { list = (IPTR *)(*list & ~(IPTR)0x1); continue; }
#endif
	    list++;
	    isdata = TRUE;
   	}
    }

    if (sysbase->KickMemPtr) {
	struct MemList *ml = (struct MemList*)sysbase->KickMemPtr;
	while (ml) {
	    UBYTE i;
	    ULONG *p = (ULONG*)ml;
	    for (i = 0; i < sizeof(struct MemList) / sizeof(ULONG); i++)
	    	chksum += p[i];

#if 0
	    if (output) {
		WriteF("ML    %X8 %X8\n", ml, chksum);
		WriteF("NODE0 %X8 %X8\n", p[0], p[1]);
		WriteF("NODE2 %X8 %X8\n", p[2], p[3]);
		WriteF("ADDR  %X8 %X8\n", ml->ml_ME[0].me_Un.meu_Addr, ml->ml_ME[0].me_Length);
		WriteF("DATA0 %X8 %X8\n", p[6], p[7]);
	    }
#endif

	    ml = (struct MemList*)ml->ml_Node.ln_Succ;
	    isdata = TRUE;
	}
    }
    if (isdata && !chksum)
    	chksum--;
    return chksum;
}


/* reset VBR and switch off MMU */
static void setcpu(void)
{
    asm(
	".chip 68040\n"
	"move.l	4,%a0\n"
	"move.w	296(%a0),%d1\n"
	"moveq	#0,%d0\n"
	"btst	#0,%d1\n"
	"beq.s	cpudone\n"
	/* clear VBR */
	"movec	%d0,%vbr\n"
	"btst	#1,%d1\n"
	"beq.s	cpudone\n"
	/* disable caches */
	"movec	%d0,%cacr\n"
	"btst	#3,%d1\n"
	"beq.s	not040\n"
	/* 68040/060 MMU */
	"movec	%d0,%tc\n"
	"movec	%d0,%dtt0\n"
	"movec	%d0,%dtt1\n"
	"movec	%d0,%itt0\n"
	"movec	%d0,%itt1\n"
	"cpusha	%bc\n"
	"bra.s	cpudone\n"
"not040: btst	#2,%d1\n"
	"beq.s	cpudone\n"
	/* 68030 MMU */
	"lea	zero(%pc),%a0\n"
	".long	0xf0104000\n"
	".long	0xf0100c00\n"
	".long	0xf0100800\n"
	"bra.s cpudone\n"
"zero:	.long	0,0\n"
"cpudone:\n"
    );
}

/* This is needed because KS clears ColdCapture before calling it
 * causing checksum mismatch in AROS exec check
 */
void coldcapturecode(void)
{
    asm(
	".long end - start\n"
	"start:\n"
	"bra.s 0f\n"
	"nop\n"               /* Align to start + 4 */
	".long 0x46414b45\n"  /* AROS_MAKE_ID('F','A','K','E') */
	"0:\n"
	"move.w	#0x440,0xdff180\n"
	"clr.l	0.w\n"
	"clr.l	4.w\n"

	// set early exceptions
	"move.w	#8,%a1\n"
	"lea	exception(%pc),%a0\n"
	"moveq	#64-2-1,%d0\n"
	"1:\n"
	"move.l	%a0,(%a1)+\n"
	"dbf	%d0,1b\n"

	"lea	0x200,%a0\n"
	"move.l	(%a0),0x7c.w\n" // restore NMI
	"lea	12(%a0),%a0\n"
	"move.l	%a0,4.w\n"
	"lea	start(%pc),%a1\n"
	"move.l	%a1,42(%a0)\n"  // ColdCapture
	"move.l	%a0,%d0\n"
	"not.l	%d0\n"
	"move.l	%d0,38(%a0)\n" // ChkBase
	"moveq	#0,%d1\n"
	"lea	34(%a0),%a0\n"
	"moveq	#24-1,%d0\n"
	"chk1:	add.w (%a0)+,%d1\n"
	"dbf	%d0,chk1\n"
	"not.w	%d1\n"
	"move.w	%d1,(%a0)\n" // ChkSum
	"move.l	start-4(%pc),%a0\n"
	"addq.l	#2,%a0\n"
	"moveq	#2,%d0\n"
	"cmp.w	#0x4ef9,(%a0)\n"
	"beq.s	.skip\n"
	// skip elf loader injected header
	"moveq	#4,%d0\n"
	"move.l	(%a0),%a0\n"
	".skip:\n"
	"move.l	0(%a0,%d0.w),%a0\n"
	"jmp	4(%a0)\n"
	"exception:\n"
	"move.w #0xff0,0xdff180\n"
	"move.w #0x000,0xdff180\n"
	"bra.s exception\n"
	"end:\n"
    );
}

/* Official reboot code from HRM 
 * All CPUs have at least 1 word prefetch,
 * jmp (a0) has been prefetched even if
 * reset disables all memory
 */
static void doreboot(void)
{
    asm volatile (
	"lea 0x01000000,%a0\n"
	"sub.l %a0@(-0x14),%a0\n"
	"move.l %a0@(4),%a0\n"
	"subq.l #2,%a0\n"
	".balign 8\n"   /* Workaround for E-UAE emulation bug */
	"reset\n"
	"jmp (%a0)\n"
    );
}

APTR entry, kicktags;
struct BootStruct *bootstruct;

static void supercode(void)
{
    ULONG *fakesys, *coldcapture, *coldcapturep;
    struct ExecBase *sysbase;
    ULONG *traps = 0;
    ULONG len;

#if DEBUG
    DebugPutStr("Entered Supervisor mode.\n");
#endif

    if ((SysBase->AttnFlags & 0xff) != 0)
    	setcpu();

#if DEBUG
    DebugPutStr("CPU setup done.\n");
#endif

    fakesys = (ULONG*)FAKEBASE;
    coldcapture = (ULONG*)COLDCAPTURE;
    coldcapture[-1] = (ULONG)entry;
    coldcapturep = (ULONG*)coldcapturecode;
    len = *coldcapturep++;
    memcpy (coldcapture, coldcapturep, len);

    *fakesys++ = traps[31]; // Level 7
    *fakesys++ = 0x4ef9 | (COLDCAPTURE >> 16);
    *fakesys++ = (COLDCAPTURE << 16) | 0x4e75;
    sysbase = (struct ExecBase*)fakesys; 

    memset(sysbase, 0, FAKEBASESIZE);
    /* Misuse DebugData as a kernel tag pointer,
     */
    if (bootstruct)
    	sysbase->DebugData = bootstruct;

    /* Detached node */
    sysbase->LibNode.lib_Node.ln_Pred = &sysbase->LibNode.lib_Node;
    sysbase->LibNode.lib_Node.ln_Succ = &sysbase->LibNode.lib_Node;

    /* Set up cold capture */
    sysbase->ColdCapture = coldcapture;
    sysbase->MaxLocMem = 512 * 1024;
    sysbase->ChkBase =~(IPTR)sysbase;
    sysbase->ChkSum = GetSysBaseChkSum(sysbase) ^ 0xffff;

    /* Propogate the existing OS's Kick Data */
    if (mlist.lh_Head->ln_Succ) {
    	sysbase->KickMemPtr = (APTR)mlist.lh_Head;
    	mlist.lh_TailPred->ln_Succ = SysBase->KickMemPtr;
    } else {
	sysbase->KickMemPtr = (APTR)SysBase->KickMemPtr;
    }
    if (kicktags) {
    	sysbase->KickTagPtr = kicktags;
    	if (SysBase->KickTagPtr) {
	     ULONG *p = kicktags;
	     while (*p)
		p++;
	     *p = 0x80000000 | (ULONG)SysBase->KickTagPtr;
	}
    } else {
    	sysbase->KickTagPtr = SysBase->KickTagPtr;
    }
    sysbase->KickCheckSum = (APTR)mySumKickData(sysbase, FALSE);

    traps[1] = (IPTR)sysbase;

#if DEBUG
    DebugPutStr("Rebooting.\n");
#endif

    // TODO: add custom cacheclear, can't call CacheClearU() because it may not work
    // anymore and KS 1.x does not even have it
    doreboot();
}

void BootROM(BPTR romlist, struct Resident **reslist, struct BootStruct *BootS)
{
    APTR GfxBase;

    entry = BADDR(romlist)+sizeof(ULONG);
    kicktags = reslist;
    bootstruct = BootS;

#if DEBUG
    DebugPutStr("Booting..\n");
#endif

#if 0
     /* Debug testing code */
    if (mlist.lh_Head->ln_Succ) {
    	SysBase->KickMemPtr = (APTR)mlist.lh_Head;
    	mlist.lh_TailPred->ln_Succ = NULL;
    }
    if (kicktags) {
    	SysBase->KickTagPtr = kicktags;
    }
    mySumKickData(SysBase, TRUE);
    SysBase->KickTagPtr = 0;
    SysBase->KickMemPtr = 0;
    Delay(200);
#endif

    if ((GfxBase = OpenLibrary("graphics.library", 0))) {
    	LoadView(NULL);
    	WaitTOF();
    	WaitTOF();
    	CloseLibrary(GfxBase);
    }

    /* We're off in the weeds now. */
    Disable();

    Supervisor((ULONG_FUNC)supercode);
}

#if DEBUG
static void DumpKickMems(ULONG num, struct MemList *ml)
{
    if (num == 0)
    	DWriteF("Original KickMemList:\n");
    else
    	DWriteF("AROS KickMemList:\n");
    /* List is single-linked but last link gets cleared later, so test ln_Succ too */
    while (ml && ml->ml_Node.ln_Succ) {
    	WORD i;
    	DWriteF("%X8:%N\n", ml, ml->ml_NumEntries);
    	for (i = 0; i < ml->ml_NumEntries; i++) {
    	    DWriteF("  %N: %X8, %N\n", i, ml->ml_ME[i].me_Un.meu_Addr, ml->ml_ME[i].me_Length);
    	}
    	ml = (struct MemList*)ml->ml_Node.ln_Succ;
    }
    DWriteF("End of List\n");
}

static void DumpKickTags(ULONG num, struct Resident **list)
{
    if (num == 0)
    	DWriteF("Original KickTagList:\n");
    else
    	DWriteF("AROS KickTagList:\n");
    while (*list) {
    	BSTR bname;
    	if ((ULONG)list & 0x80000000) {
    	    DWriteF("Redirected to %X8\n", (ULONG)list & ~0x80000000);
    	    list = (struct Resident**)((ULONG)list & ~0x80000000);
    	    continue;
    	}
    	bname = ConvertCSTR((*list)->rt_IdString);
    	DWriteF("%X8: %X8 %S\n", list, *list, bname);
    	FreeBSTR(bname);
    	list++;
    }
    DWriteF("End of List\n");
}
#endif

#define HC_FORCEFAST 1
struct HardwareConfig
{
    UWORD manufacturer;
    UBYTE product;
    const UBYTE *name;
    UBYTE flags;
};

static const struct HardwareConfig hc[] =
{
    { 8512, 17, "Blizzard A1200 Accelerator", HC_FORCEFAST }, // Blizzard 1230 IV, 1240 or 1260.
    { 0 }
};

static void DetectHardware(void)
{
    struct ConfigDev *cd = NULL;
    int i;
    
    ExpansionBase = (APTR)OpenLibrary("expansion.library", 0);
    if (!ExpansionBase)
        return;
    while((cd = FindConfigDev(cd, -1, -1))) {
        for (i = 0; hc[i].manufacturer; i++) {
            if (cd->cd_Rom.er_Manufacturer == hc[i].manufacturer && cd->cd_Rom.er_Product == hc[i].product) {
                BSTR bname;
                bname = ConvertCSTR(hc[i].name);
                WriteF("%S: ", bname);
                if (hc[i].flags & HC_FORCEFAST) {
                    forceFAST = TRUE;
                    WriteF("ForceFast enabled");
                }
                WriteF("\n");
                FreeBSTR(bname);
            }
        }
    }
    CloseLibrary(ExpansionBase);
}

static struct Resident **ScanROMSegment(BPTR ROMSegList, struct Resident **resnext, ULONG *resleft)
{
    BPTR seg;
    /* Only scan 2nd ROM section */
    seg = *((BPTR*)BADDR(ROMSegList));
    return LoadResident(seg, resnext, resleft, LRF_NOPATCH);
}

#define KERNELTAGS_SIZE 10
#define CMDLINE_SIZE 512

static struct BootStruct *AllocBootStruct(UBYTE *cmdline)
{
    struct TagItem *tags;
    struct BootStruct *boots;
    UBYTE *cmd;
    
    boots = aosAllocMem(sizeof(struct BootStruct) + sizeof(struct TagItem) * KERNELTAGS_SIZE + CMDLINE_SIZE, MEMF_CLEAR, SysBase);
    if (!boots)
    	return NULL;
    tags = (struct TagItem*)(boots + 1);
    cmd = (UBYTE*)(tags + KERNELTAGS_SIZE);
    /* cmdline is a BCPL string! */
    if (cmdline && cmdline[0])
        CopyMem(cmdline + 1, cmd, cmdline[0]);
    else
    	strcpy(cmd, DEFAULT_KERNEL_CMDLINE);
    tags[0].ti_Tag = KRN_CmdLine;
    tags[0].ti_Data = (IPTR)cmd;
    tags[1].ti_Tag = TAG_DONE;

    boots->magic = ABS_BOOT_MAGIC;
    boots->kerneltags = tags;
    if (forceFAST) {
        UBYTE *addr = specialAlloc(SS_STACK_SIZE + MAGIC_FAST_SIZE + 1, MEMF_FAST | MEMF_REVERSE, SysBase); /* +1 = guaranteed extra page at the end */
        if (addr) {
            addr = (UBYTE*)(((ULONG)(addr + PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1));
            boots->ss_address = addr;
            boots->ss_size = SS_STACK_SIZE;
            /* magic fast mem pool for early allocations that normally can't be allocated from fast memory */
            boots->magicfastmem = boots->ss_address + SS_STACK_SIZE;
            boots->magicfastmemsize = MAGIC_FAST_SIZE;
        }
    }
    return boots;
}

__startup static AROS_ENTRY(int, startup,
	  AROS_UFHA(char *, argstr, A0),
	  AROS_UFHA(ULONG, argsize, D0),
	  struct ExecBase *, SysBase)
{
    AROS_USERFUNC_INIT

    APTR lowmem;

    /* See if we're already running on AROS.
     */
    if (OpenResource("kernel.resource"))
    	return RETURN_OK;

    /* Allocate some low MEMF_CHIP ram, as a buffer
     * against overlapping allocations with the initial
     * stack.
     */
    lowmem = AllocMem(PAGE_SIZE, MEMF_CHIP);

    NEWLIST(&mlist);
    DOSBase = (APTR)OpenLibrary("dos.library", 0);
    if (DOSBase != NULL) {
    	BPTR ROMSegList;
    	BSTR name = AROS_CONST_BSTR("aros.elf");
    	enum { ARG_ROM = 16, ARG_CMD = 17, ARG_FORCECHIP = 18, ARG_FORCEFAST = 19, ARG_DEBUG = 20, ARG_MODULES = 0 };
    	/* It would be nice to use the '/M' switch, but that
    	 * is not supported under the AOS BCPL RdArgs routine.
    	 *
    	 * So only 16 modules are supported
    	 */
    	BSTR format = AROS_CONST_BSTR(",,,,,,,,,,,,,,,,ROM/K,CMD/K,FORCECHIP/S,FORCEFAST/S,DEBUG/S");
    	/* Make sure the args are in .bss, not stack */
    	static ULONG args[16 + 5 + 256] __attribute__((aligned(4))) = { };

    	WriteF("AROSBootstrap " ADATE "\n");
        args[0] = name;

        RdArgs(format, MKBADDR(args), sizeof(args)/sizeof(args[0]));
#if DEBUG > 1
        DWriteF("ROM: %S\n", args[ARG_ROM]);
        DWriteF("CMD: %S\n", args[ARG_CMD]);
        DWriteF("FORCECHIP: %N\n", args[ARG_FORCECHIP]);
        DWriteF("FORCEFAST: %N\n", args[ARG_FORCEFAST]);
        DWriteF("MOD: %S\n", args[0]);
        DWriteF("   : %S\n", args[1]);
        DWriteF("   : %S\n", args[2]);
        DWriteF("   : %S\n", args[3]);

#endif
        forceCHIP = args[ARG_FORCECHIP] ? TRUE : FALSE;
        forceFAST = args[ARG_FORCEFAST] ? TRUE : FALSE;
        debug_enabled = args[ARG_DEBUG] ? TRUE : FALSE;

        /* Blizzard A1200 accelerator boards have strange MAP ROM
         * feature, even when it is disabled, ROM is copied to
         * last 512K of RAM, so we have to make sure all our
         * allocations ignore this region.
         * Blizzard Fast RAM memory type is plain MEMF_FAST.
         */
        if (SysBase->LibNode.lib_Version >= 37 && !AvailMem(MEMF_KICK | MEMF_FAST)) {
            if (AvailMem(MEMF_FAST)) {
                WriteF("Reserving non-MEMF_KICK Fast RAM\n");
                int i;
                /* Allocate in PAGE_SIZE byte chunks, it is smallest allocated size */
                for (i = 0; i < 524288 / PAGE_SIZE; i++)
                    AllocMem(PAGE_SIZE, MEMF_FAST | MEMF_REVERSE);
            }
        }

        meminfo();
        DetectHardware();

#if DEBUG
        DebugInit();
#endif

        if (!IoErr()) {
            /* Load ROM image */
            if (args[ARG_ROM] == BNULL)
                args[ARG_ROM] = name;

            ROMSegList = ROMLoad(args[ARG_ROM]);
            if (ROMSegList != BNULL) {
                ULONG resleft = 0;
                struct Resident **ResidentList;
                struct BootStruct *BootS;
                WriteF("Successfully loaded ROM\n");
                ROM_Loaded = TRUE;
                struct Resident **resnext, *reshead = NULL;

		PatchResidents(ROMSegList);
                resnext = &reshead;
                resnext = ScanROMSegment(ROMSegList, resnext, &resleft);
                resnext = LoadResidents(&args[ARG_MODULES], resnext, &resleft);
                BootS = AllocBootStruct(BADDR(args[ARG_CMD]));
                ResidentList = (APTR)((IPTR)reshead & ~RESLIST_NEXT);

#if DEBUG
		DumpKickMems(0, SysBase->KickMemPtr);
		DumpKickMems(1, (struct MemList*)mlist.lh_Head);
		DumpKickTags(0, SysBase->KickTagPtr);
		DumpKickTags(1, ResidentList);
#endif
		WriteF("Booting...\n");
                Delay(50);

                BootROM(ROMSegList, ResidentList, BootS);

                UnLoadSeg(ROMSegList);
            } else {
                WriteF("Can't load ROM ELF file %S\n", args[ARG_ROM]);
            }
        } else {
            WriteF("Can't parse arguments, error %N\n", IoErr());
	}

    	CloseLibrary((APTR)DOSBase);
    }

    FreeMem(lowmem, PAGE_SIZE);

    return IoErr();

    AROS_USERFUNC_EXIT
}
