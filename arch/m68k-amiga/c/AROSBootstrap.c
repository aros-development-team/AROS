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
 * As you can probably guess, you will need at least 2MB of
 * MEMF_LOCAL RAM to get this to work. We can't use MEMF_KICK,
 * since some MEMF_KICK ram may not be available until after
 * expansion.library processing.
 *
 * Also - no AROS specific code can go in here! We have to run
 * on AOS 1.3 and up.
 */
#define DEBUG 0

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#include <zlib.h>

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
#endif
#include <rom/dos/internalloadseg_elf.c>

struct DosLibrary *DOSBase;

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

static BSTR AllocBSTR(const char *name)
{
    UBYTE *bs;
    int len = strlen(name);

    if (len > 255)
    	return BNULL;


    bs = AllocMem(256+1, MEMF_ANY);
    if (bs == NULL)
    	return BNULL;

    bs[0] = len;
    bs[len+1] = 0;
    CopyMem(name, &bs[1], len);
    return MKBADDR(bs);
}

static void FreeBSTR(BSTR bstr)
{
    FreeMem(BADDR(bstr), 256+1);
}

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

static void WriteF(const char *fmt, ...)
{
   IPTR *args = (IPTR *)&fmt;
   BSTR bfmt = AllocBSTR(fmt);

   doBCPL(BCPL_WriteF, bfmt, args[1], args[2], args[3], &args[4], 26-3);

   FreeBSTR(bfmt);
}

/* For KS < 2.0, we need to call the BCPL ReadArgs,
 * since DOS/ReadArgs doesn't exist.
 */
static ULONG RdArgs(BSTR format, BPTR args, ULONG max_arg)
{
    return doBCPL(BCPL_RdArgs, format, args, max_arg, 0, NULL, 0);
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

    /* Clear bits 15-0, we're setting memory class explicitly */
    flags &= ~0x7fff;

    if (SysBase->LibNode.lib_Version >= 36) {
    	flags |= MEMF_LOCAL | MEMF_REVERSE;
    } else {
    	flags |= MEMF_CHIP;
    }

    size += sizeof(struct MemList);
    ml = AllocMem(size, flags);
    if (ml == NULL) {
    	WriteF("AOS: Failed to allocate %N bytes of type %X4\n", size, flags);
    } else {
	ml->ml_NumEntries = 1;
	ml->ml_ME[0].me_Addr = (APTR)ml;
	ml->ml_ME[0].me_Length = size;
    	AddTail(&mlist, (struct Node*)ml);
    }
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
    size += sizeof(struct MemList);
    Remove((struct Node*)addr);
    FreeMem(addr, size);

    AROS_USERFUNC_EXIT
}

/* Backcalls for InternalLoadSeg_ELF
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
static AROS_UFH3(APTR, elfAlloc,
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(ULONG, flags, D1),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    APTR ret;

    /* Clear bits 15-0, we're setting memory class explicitly */
    flags &= ~0x7fff;

    if (SysBase->LibNode.lib_Version >= 36) {
    	flags |= MEMF_LOCAL | MEMF_REVERSE;
    } else {
    	flags |= MEMF_CHIP;
    }

    ret = AllocMem(size, flags);
    if (ret == NULL)
    	WriteF("ELF: Failed to allocate %N bytes of type %X4\n", size, flags);

    return ret;

    AROS_USERFUNC_EXIT
}
static AROS_UFH3(void, elfFree,
	AROS_UFHA(APTR, addr, A1),
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    FreeMem(addr, size);

    AROS_USERFUNC_EXIT
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
    if ((gzf = gzopen(filename, "rb"))) {
    	gzbuffer(gzf, 65536);

    	WriteF("Loading '%S' into RAM...\n", bfilename);
    	rom = InternalLoadSeg_ELF((BPTR)gzf, BNULL, funcarray, NULL, DOSBase);
    	if (rom == BNULL) {
    	    WriteF("'%S': Can't parse\n", bfilename);
    	}

    	gzclose_r(gzf);
    } else {
    	WriteF("'%S': Can't open\n", bfilename);
    }
    FreeString(filename);
    return rom;
}

static struct Resident *LoadFindResident(BPTR seglist)
{
    while (seglist) {
    	ULONG *ptr = BADDR(seglist);
    	UWORD *res;
    	LONG len = ptr[-1] * 4;
    	
 	res = (UWORD*)(ptr + 1);
    	while (len >= 26) {
    	    if (*res == 0x4afc && ((ULONG*)(res + 1))[0] == (ULONG)res)
    	    	return (struct Resident*)res;
    	    res++;
    	    len -= 2;
    	}
    	seglist = *((BPTR*)BADDR(seglist));
    }
    return NULL;	    	
}

static struct Resident **LoadResidents(ULONG *namearray)
{
    UBYTE rescnt, i;
    struct Resident **reslist = NULL;
    SIPTR funcarray[] = {
    	(SIPTR)aosRead,
    	(SIPTR)aosAlloc,
    	(SIPTR)aosFree,
    	(SIPTR)aosSeek,
    };
    rescnt = 0;
    for (i = 0; namearray[i]; i++) {
	struct Resident *resident;
	LONG stack;
	BPTR handle;
	BPTR seglist = BNULL;
	BPTR bname;
    	UBYTE *name;
    	
    	bname = (BPTR)namearray[i];
    	name = ConvertBSTR(bname);
    	if (name) {
	    handle = Open(name, MODE_OLDFILE);
	    if (handle) {
		seglist = InternalLoadSeg(handle, BNULL, (LONG_FUNC*)funcarray, &stack);
		Close(handle);
	    }
	    if (seglist) {
		WriteF("Loaded '%S'\n", bname);
		resident = LoadFindResident(seglist);
		if (resident) {
		    if (!reslist) {
			reslist = aosAllocMem(100 * sizeof(struct Resident*), MEMF_CLEAR, SysBase);
			if (!reslist)
			    return NULL;
		    }
		    WriteF("Resident structure found @%X8\n", resident);
		    reslist[rescnt++] = resident;
		}
	    } else {
		WriteF("Failed to load '%S'\n", bname);
	    }
	    FreeString(name);
	}
    }
    reslist[rescnt] = 0;
    return reslist;
}

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

static ULONG mySumKickData(struct ExecBase *sysbase)
{
    ULONG chksum = 0;

    if (sysbase->KickTagPtr) {
    	IPTR *list = sysbase->KickTagPtr;
 	while(*list)
	{
   	    chksum += (ULONG)*list;
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
   	}
    }

    if (sysbase->KickMemPtr) {
	struct MemList *ml = (struct MemList*)sysbase->KickMemPtr;
	while (ml->ml_Node.ln_Succ) {
	    UBYTE i;
	    ULONG *p = (ULONG*)ml;
	    for (i = 0; i < sizeof(struct MemList) / sizeof(ULONG); i++)
	    	chksum += p[i];
	    ml = (struct MemList*)ml->ml_Node.ln_Succ;
	}
    }

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
	"jmp	(%a0)\n"
	"end:\n"
    );
}

/* We have to copy the reboot code, as it must be in
 * MEMF_LOCAL RAM, otherwise the jmp after the
 * reset will vanish.
 *
 * DO NOT CALL THIS FUNCTION DIRECTLY!
 */
#define REBOOTBASE (12 * sizeof(ULONG))	/* Unused m68k exception vectors 12 and 13*/
#define REBOOTSIZE (4 * sizeof(UWORD))
static void rebootcode(void)
{
    asm volatile (
	"nop\n"
	"move.l #2,%a0\n"
	"reset\n"
	"jmp (%a0)\n"
    );
}

APTR entry, kicktags;

static void supercode(void)
{
    ULONG *fakesys, *coldcapture, *coldcapturep;
    struct ExecBase *sysbase;
    void (*reboot)(void) = (APTR)REBOOTBASE;
    ULONG *traps = 0;
    ULONG len;

    if ((SysBase->AttnFlags & 0xff) != 0)
    	setcpu();
    fakesys = (ULONG*)FAKEBASE;
    coldcapture = (ULONG*)COLDCAPTURE;
    coldcapture[-1] = (ULONG)entry;
    coldcapturep = (ULONG*)coldcapturecode;
    len = *coldcapturep++;
    memcpy (coldcapture, coldcapturep, len);
    memcpy (rebootcode, (APTR)reboot, REBOOTSIZE);

    *fakesys++ = traps[31]; // Level 7
    *fakesys++ = 0x4ef9 | (COLDCAPTURE >> 16);
    *fakesys++ = (COLDCAPTURE << 16) | 0x4e75;
    sysbase = (struct ExecBase*)fakesys; 

    memset(sysbase, 0, FAKEBASESIZE);

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
    if (SysBase->KickTagPtr) {
 	sysbase->KickTagPtr = kicktags;
    } else {
	sysbase->KickTagPtr = kicktags;
    }
    sysbase->KickCheckSum = (APTR)mySumKickData(sysbase);

    traps[1] = (IPTR)sysbase;
    // TODO: add custom cacheclear, can't call CacheClearU() because it may not work
    // anymore and KS 1.x does not even have it
    reboot();
}

void BootROM(BPTR romlist, struct Resident **reslist)
{
    APTR GfxBase;

    if ((GfxBase = OpenLibrary("graphics.library", 0))) {
    	LoadView(NULL);
    	LoadView(NULL);
    	CloseLibrary(GfxBase);
    }

    entry = BADDR(romlist)+sizeof(ULONG);
    kicktags = reslist;

    /* We're off in the weeds now. */
    Disable();

    Supervisor((ULONG_FUNC)supercode);
}

__startup static AROS_ENTRY(int, startup,
	  AROS_UFHA(char *, argstr, A0),
	  AROS_UFHA(ULONG, argsize, D0),
	  struct ExecBase *, SysBase)
{
    AROS_USERFUNC_INIT

    /* See if we're already running on AROS.
     */
    if (OpenResource("kernel.resource"))
    	return RETURN_OK;
    	
    NEWLIST(&mlist);
    DOSBase = (APTR)OpenLibrary("dos.library", 0);
    if (DOSBase != NULL) {
    	BPTR ROMSegList;
    	BSTR name = BNULL;
    	BSTR format = BNULL;
    	ULONG *args = NULL;

    	if ((name = AllocBSTR("aros.elf.gz")) &&
    	    (format = AllocBSTR(",,,,,,,,,")) && /* this can't be the best way.. */
    	    (args = AllocMem(sizeof(ULONG) * 100, MEMF_ANY))) {
	    args[0] = name;

	    RdArgs(format, MKBADDR(args), 50);
	    if (!IoErr()) {
	    	/* Load ROM image */
	    	if (args[0] == BNULL)
	    	    args[0] = name;

		ROMSegList = ROMLoad(args[0]);
		if (ROMSegList != BNULL) {
		    struct Resident **ResidentList;
		    WriteF("Successfully loaded ROM\n");

		    ResidentList = LoadResidents(&args[1]);

		    WriteF("Booting...\n");
		    Delay(50);

		    BootROM(ROMSegList, ResidentList);

		    UnLoadSeg(ROMSegList);
		} else {
		    WriteF("Can't load ROM ELF file %S\n", args[0]);
		}
	    } else {
		WriteF("Can't parse arguments\n");
	    }
	}

   	FreeBSTR(name);
    	FreeBSTR(format);
    	FreeMem(args, sizeof(ULONG) * 100);
    	CloseLibrary((APTR)DOSBase);
    }

    return RETURN_OK;

    AROS_USERFUNC_EXIT
}


