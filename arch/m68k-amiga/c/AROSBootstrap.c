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
 * MEMF_KICK RAM to get this to work.
 *
 * Also - no AROS specific code can go in here! We have to run
 * on AOS 1.3 and up.
 */
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#include <zlib.h>

#include <rom/dos/internalloadseg_elf.c>

struct DosLibrary *DOSBase;

/* Define these here for zlib so that we don't
 * pull in arosc.library.
 */
void *malloc(int size)
{
    return AllocVec(size, MEMF_ANY);
}

void free(void *ptr)
{
    FreeVec(ptr);
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

static struct MinList mlist;

static AROS_UFH3(APTR, elfAlloc,
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(ULONG, flags, D1),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    struct MemList *ml;

    size += sizeof(*ml);

    ml = AllocMem(size, flags | MEMF_KICK);

    ml->ml_NumEntries = 1;
    ml->ml_ME[0].me_Addr = (APTR)ml;
    ml->ml_ME[0].me_Length = size;

    AddTail(&mlist, ml);

    return &ml[1];

    AROS_USERFUNC_EXIT
}

static AROS_UFH3(void, elfFree,
	AROS_UFHA(APTR, addr, A1),
	AROS_UFHA(ULONG, size, D0),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    struct MemList *ml;

    addr -= sizeof(*ml);
    size += sizeof(*ml);

    Remove(addr);

    FreeMem(addr, size);

    AROS_USERFUNC_EXIT
}

static BPTR ROMLoad(const char *filename)
{
    gzFile gzf;
    BPTR rom = BNULL;
    SIPTR funcarray[] = {
    	(SIPTR)elfRead,
    	(SIPTR)elfAlloc,
    	(SIPTR)elfFree,
    	(SIPTR)elfSeek,
    };

    if ((gzf = gzopen(filename, "rb"))) {
    	gzbuffer(gzf, 65536);

    	rom = InternalLoadSeg_ELF((BPTR)gzf, BNULL, funcarray, NULL, DOSBase);
    	if (rom == BNULL) {
    	    Printf("%s: Can't parse\n", filename);
    	}

    	gzclose_r(gzf);
    } else {
    	Printf("%s: Can't open\n", filename);
    }

    return rom;
}

UWORD GetSysBaseChkSum(void)
{
    UWORD sum = 0;
    UWORD *p = (UWORD*)&SysBase->SoftVer;
    while (p <= &SysBase->ChkSum)
    	sum += *(p++);

    return sum;
}

void BootROM(BPTR romlist)
{
    APTR GfxBase;
    APTR entry;

    if (0 && (GfxBase = OpenLibrary("graphics.library", 0))) {
    	LoadView(NULL);
    	LoadView(NULL);
    	CloseLibrary(GfxBase);
    }

    entry = BADDR(romlist)+sizeof(ULONG);

    /* We're off in the weeds now. */
    Disable();

    /* Make list singly linked, and join
     * with the existing KickMem list
     */
    mlist.mlh_TailPred->mln_Succ = SysBase->KickMemPtr;

    SysBase->KickMemPtr = mlist.mlh_Head;
    SysBase->KickCheckSum = (APTR)SumKickData();

    SysBase->ColdCapture = entry;
    SysBase->ChkBase=~(IPTR)SysBase;
    SysBase->ChkSum = 0;
    SysBase->ChkSum = GetSysBaseChkSum() ^ 0xffff;

    ColdReboot();
}


int __nocommandline;

int main(void)
{
    int err = 1;

    NEWLIST(&mlist);

    DOSBase = (APTR)OpenLibrary("dos.library", 0);
    if (DOSBase != NULL) {
    	BPTR ROMSegList;
    	IPTR args[1] = {
    	    (IPTR)"aros.elf.gz",
    	};
    	struct RDArgs *rda;

    	if ((rda = ReadArgs("FILE", args, NULL)) != NULL) {

    	    ROMSegList = ROMLoad((const char *)args[0]);
    	    if (ROMSegList != BNULL) {
    	    	Printf("Successfully loaded ROM\n");

    	    	BootROM(ROMSegList);

    	    	UnLoadSeg(ROMSegList);
    	    } else {
    	    	Printf("Can't load ROM ELF file\n");
    	    }
    	    FreeArgs(rda);
    	} else {
    	    Printf("Can't parse arguments\n");
    	}
    	/* Load ROM image */

    	CloseLibrary((APTR)DOSBase);
    }
    return err;
}
