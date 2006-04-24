/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux init code for emulated (Unix) systems.
    Lang: english
*/


#include <exec/types.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/resident.h>
#include <exec/execbase.h>

#include <proto/exec.h>

#if (AROS_BIG_ENDIAN == 0)
#define SWAP(x) ((((ULONG)x >> 24) & 0x000000ff) |\
		 (((ULONG)x >> 8 ) & 0x0000ff00) |\
		 (((ULONG)x << 8 ) & 0x00ff0000) |\
		 (((ULONG)x << 24) & 0xff000000)    )
#else
#define SWAP(x) x
#endif

#define _XOPEN_SOURCE 600L /* for posix_memalign */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#define __USE_MISC /* for MAP_ANON */
#include <sys/mman.h>
#include <sys/termios.h>

#include "../../../rom/exec/memory.h"	/* From $(TOP)/rom/exec */

extern const struct Resident
    Expansion_ROMTag,
    Exec_resident,
    Utility_ROMTag,
    Aros_ROMTag,
/*    BOOPSI_resident,*/
    OOP_ROMTag,
    HIDDCl_ROMTag,
    UXIO_ROMTag,
    Graphics_ROMTag,
    Layers_ROMTag,
    Timer_ROMTag,
    Battclock_ROMTag,
    Keyboard_ROMTag,
    Gameport_ROMTag,
    Keymap_ROMTag,
    Input_ROMTag,
    Intuition_ROMTag,
    X11Cl_ROMTag,
    LinuxFB_ROMTag,
    Cybergraphics_ROMTag,
    Console_ROMTag,
#if ENABLE_DBUS == 1
    Dbus_ROMTag,
#endif
    Mathffp_ROMTag,
    Mathieeesingbas_ROMTag,
    Workbench_ROMTag,
    Dos_ROMTag,
    LDDemon_resident,
    emul_handler_ROMTag,
    UXSer_ROMTag,
    UXPar_ROMTag,
    boot_resident,
    Con_ROMTag,
    Nil_ROMTag,
    Ram_ROMTag,
    PCI_ROMTag,
    PCILx_ROMTag;


/* This list MUST be in the correct order (priority). */
static const struct Resident *romtagList[] =
{
    &Expansion_ROMTag,			/* SingleTask,  110  */
    &Exec_resident,			/* SingleTask,  105  */
    &Utility_ROMTag,			/* ColdStart,   103  */
    &Aros_ROMTag,			/* ColdStart,   102  */
    &Mathieeesingbas_ROMTag,            /* ColdStart,   101  */
#if 0
    &BOOPSI_resident,			/* ColdStart,   95	 */
#endif
    &OOP_ROMTag,			/* ColdStart,   94	 */
    &HIDDCl_ROMTag,			/* ColdStart,   92	 */
    &UXIO_ROMTag,			/* ColdStart,   91	 */
#if defined(__i386__) || defined(__x86_64__)
    &PCI_ROMTag,                        /* ColdStart,   90       */
//    &PCILx_ROMTag,                      /* ColdStart,   89       */
#endif
    &Graphics_ROMTag, 			/* ColdStart,   65	 */
    &Layers_ROMTag,			/* ColdStart,   60       */
    &Timer_ROMTag,			/* ColdStart,   50	 */
    &Battclock_ROMTag,			/* ColdStart,   45	 */
    &Keyboard_ROMTag,			/* ColdStart,   44	 */
    &Gameport_ROMTag,			/* ColdStart,   43	 */
    &Keymap_ROMTag,			/* ColdStart,   40	 */
    &Input_ROMTag,			/* ColdStart,   30	 */
    &Intuition_ROMTag,			/* ColdStart,   10	 */
    &X11Cl_ROMTag,			/* ColdStart,   9	 */
    &LinuxFB_ROMTag,			/* ColdStart,   9	 */
    &Cybergraphics_ROMTag,		/* ColdStart,   8	 */
    &Console_ROMTag,			/* ColdStart,   5	 */
#if ENABLE_DBUS == 1
    &Dbus_ROMTag,			/* ColdStart,   0	 */
#endif
    &emul_handler_ROMTag,		/* ColdStart,   0	 */
    &UXSer_ROMTag,	   	    	/* ColdStart,   0    */
    &UXPar_ROMTag,	   	    	/* ColdStart,   0    */
    &Workbench_ROMTag,			/* ColdStart,  -120  */
    &Mathffp_ROMTag,			/* ColdStart,  -120  */

    /*
	NOTE: You must not put anything between these two; the code
        which initialized boot_resident will directly call
        Dos_resident and anything between the two will be skipped.
    */
    &boot_resident,			/* ColdStart,  -50	 */
    &Dos_ROMTag,			/* None,	   -120  */
    &LDDemon_resident,			/* AfterDOS,   -125  */
    &Con_ROMTag,			/* AfterDOS,   -126  */
    &Nil_ROMTag,			/* AfterDOS,   -127	 */
    &Ram_ROMTag,			/* AfterDOS,   -128	 */
//    &Partition_ROMTag,                     

    NULL
};

/* So we can examine the memory */
static struct MemHeaderExt mhe;
struct MemHeader *mh  = &mhe.mhe_MemHeader;
UBYTE *memory, *space;
int memSize = 8;

extern void InitCore(void);
extern struct ExecBase *PrepareExecBase(struct MemHeader *mh);

extern APTR __libc_malloc(size_t);
extern VOID __libc_free(APTR);
extern APTR __libc_calloc(size_t, size_t);
extern APTR __libc_realloc(APTR mem, size_t newsize);


static APTR myAlloc(struct MemHeaderExt *mhe, ULONG size, ULONG *flags)
{
    APTR ret;
    
    if (flags && (*flags & MEMF_CLEAR))
        ret = __libc_calloc(1, size);
    else
        ret = __libc_malloc(size);
    
    if (ret)
    {
        if (flags) *flags &= ~MEMF_CLEAR;
        mhe->mhe_MemHeader.mh_Free -= size;
    }

    return ret;
}

static VOID myFree(struct MemHeaderExt *mhe, APTR mem, ULONG size)
{
    mhe->mhe_MemHeader.mh_Free += size;

    return __libc_free(mem);    
}

static ULONG myAvail(struct MemHeaderExt *mhe, ULONG flags)
{
    if (flags & MEMF_TOTAL)
        return memSize << 20;
	 
    return mhe->mhe_MemHeader.mh_Free;
}

BOOL use_hostmem = FALSE;

APTR malloc(size_t size)
{
    if (use_hostmem)
        return AllocVec(size, MEMF_ANY);

    return __libc_malloc(size);
}

VOID free(APTR mem)
{
    if (use_hostmem)
        return FreeVec(mem);

    return __libc_free(mem);
}

APTR calloc(size_t n, size_t size)
{
    if (use_hostmem)
        return AllocVec(size * n, MEMF_CLEAR);

    return __libc_calloc(n, size);
}


static APTR ReAllocVec(APTR old, size_t size, ULONG flags)
{
    APTR new = AllocVec(size, flags);
    
    if (new)
    {
        ULONG oldsize = *(ULONG *)((char *)old - AROS_ALIGN(sizeof(ULONG)));
	
	memcpy(new, old, oldsize > size ? size : oldsize);
        FreeVec(old);
        old = new;
    }
    
    return old;
}

APTR realloc(APTR mem, size_t size)
{
    if (use_hostmem)
        return ReAllocVec(mem, size, MEMF_ANY);

    return __libc_realloc(mem, size);
}

/*
    This is where AROS is first called by whatever system loaded it,
    either some kind of boot loader, or a "parent" operating system.

    For boot loaded $(ARCH), you don't need to define main() like this,
    you can have it anyway your bootloader likes.
*/

extern char _start, _end;

int main(int argc, char **argv)
{
    /*	Well, if you want you can take in command line arguments here,
	but that is not necessary, or perhaps rather complex...

	eg: say you wished to allow people to specify the root directory
	    arosshell --rootdir /usr/local/AROS --memsize 4

	For an example, you could ask how much memory you want for the
	system, chip/fast whatever...
    */

    struct ExecBase *SysBase;
    struct termios t;
    int psize = 0;
    int i = 0, x;
    int ticrate = 100;
    BOOL mapSysBase   = FALSE;
    BOOL _use_hostmem = FALSE;

    while (i < argc)
    {
      if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
      {
        printf
	(
	    "AROS for Linux\n"
            "usage: %s [options]\n"
            " -h                 show this page\n"
            " -m <size>          allocate <size> Megabytes of memory for AROS\n"
            " -M                 allows programs to read SysBase from Address $4; SysBase is\n"
            "                    found there in big endian format\n"
	    " -t <value>         timer ticks per second. Must be Multiple of 50; max value\n"
	    "                    working on 2.4 kernels is 100; on 2.6 kernels it is 1000;\n"
	    "                    default is 100.\n"
            " --help             same as '-h'\n"
            " --memsize <size>   same as '-m <size>'\n"
            " --mapsysbase       same as '-M'\n"
	    " --tickrate <value> same as '-t <value>'\n"
	    " --hostmem          Let AROS use the host operating system's facilities to\n"
	    "                    manage memory, rather than the AROS' builtin ones.\n"
            "\n"
	    "Please report bugs to the AROS development team. http://www.aros.org/\n",
	    argv[0]
	);
        return 0;
      }
      else
      if (!strcmp(argv[i], "--memsize") || !strcmp(argv[i], "-m"))
      {
        i++;
        x = 0;
        memSize = 0;
        while ((argv[i])[x] >= '0' && (argv[i])[x] <= '9')
        {
          memSize = memSize * 10 + (argv[i])[x] - '0';
          x++;
        }
        i++;
      }
      else
      if (!strcmp(argv[i], "--mapsysbase") || !strcmp(argv[i], "-M"))
      {
        mapSysBase = TRUE;
        i++;
      }
      else
      if (!strcmp(argv[i], "--ticrate") || !strcmp(argv[i], "-t"))
      {
        i++;
        x = 0;
        ticrate = 0;
        while ((argv[i])[x] >= '0' && (argv[i])[x] <= '9')
        {
          ticrate = ticrate * 10 + (argv[i])[x] - '0';
          x++;
        }
        i++;
      }
      else
      if (!strcmp(argv[i], "--hostmem"))
      {
        _use_hostmem = TRUE;
        i++;
      }
      else
        i++;
    }

    /*
    First up, set up the memory.

    If your memory starts at 0 (I think Linux does, FreeBSD doesn't),
    then you can allocate 4K at that address, and do whatever you want
    to make that invalid to trap NULL dereference errors.

    */
/*
#if defined(__linux__) && defined(__mc68000__)
*/
    if (TRUE == mapSysBase)
    {
      psize = getpagesize();
      space = mmap((APTR)0, (memSize << 20), PROT_READ|PROT_WRITE,
  		   MAP_ANON|MAP_PRIVATE|MAP_FIXED, -1, 0);
      if (space != (UBYTE *)-1)
      {
	int size = psize/sizeof(ULONG);
	memory = (UBYTE *)((ULONG)space + psize);
	while(--size)
	    ((ULONG *)space)[size] = 0xDEADBEEF;
      }
      else
      {
	perror("mmap");
	exit(20);
      }
    }
    else
/*
#else
*/
    if (!_use_hostmem)
    {
      /* We allocate memSize megabytes */
      if( posix_memalign(&memory, getpagesize(), (memSize << 20)) != 0 )
      {
	 /*fprintf(stderr, "Cannot allocate any memory!\n");*/
	 exit(20);
      }
      /* Make whole AROS memory area executable */
      if (mprotect(memory, memSize << 20, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
	 perror("mprotect failed");
    }
    else
    {
        /* The host system is going to allocate memory through AROS,
	   allocate some more memory for it.  */
        memSize += 8;
    }
/*
#endif
*/
    /* Prepare the first mem header */
    mh->mh_Node.ln_Type  = NT_MEMORY;
    mh->mh_Node.ln_Name  = "chip memory";
    mh->mh_Node.ln_Pri   = -5;
    mh->mh_Attributes    = MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL |
			   MEMF_24BITDMA | MEMF_KICK;
			    
    if (_use_hostmem)
    {
        mh->mh_Attributes |= MEMF_MANAGED;
        mh->mh_First       = NULL;
	mh->mh_Lower       = (char *)&_end + 1;
	mh->mh_Upper       = (APTR)(~(IPTR)0 / 2); /* Should use getrlimit here. */
	mh->mh_Free        = memSize << 20;
	
	((struct MemHeaderExt *)mh)->mhe_Alloc = myAlloc;
        ((struct MemHeaderExt *)mh)->mhe_Free  = myFree;
        ((struct MemHeaderExt *)mh)->mhe_Avail = myAvail;
    }
    else
    {
        mh->mh_First           = (struct MemChunk *)memory;
        mh->mh_First->mc_Next  = NULL;
        mh->mh_First->mc_Bytes = (memSize << 20) - psize;
        
	mh->mh_Lower           = mh->mh_First;
        mh->mh_Upper           = (APTR)(memory + (memSize << 20) - psize);
        mh->mh_Free            = mh->mh_First->mc_Bytes;
    }

    /*
	This will prepare enough of ExecBase to allow us to
	call functions, it will also set up the memory list.
    */
    SysBase = PrepareExecBase(mh);
    SysBase->PowerSupplyFrequency = ticrate / SysBase->VBlankFrequency;
    
    use_hostmem = _use_hostmem;
    
    /* ROM memory header. This special memory header covers all ROM code and data sections
     * so that TypeOfMem() will not return 0 for addresses pointing into the kernel.
     */
    if ((mh = (struct MemHeader *)AllocMem(sizeof(struct MemHeader), MEMF_PUBLIC)))
    {
	/* These symbols are provided by the linker on most platforms */
	mh->mh_Node.ln_Type = NT_MEMORY;
	mh->mh_Node.ln_Name = "rom memory";
	mh->mh_Node.ln_Pri = -128;
	mh->mh_Attributes = MEMF_KICK;
	mh->mh_First = NULL;
	mh->mh_Lower = (APTR)&_start;
	mh->mh_Upper = (APTR)&_end;
	mh->mh_Free = 0;			/* Never allocate from this chunk! */
	Forbid();
	Enqueue(&SysBase->MemList, &mh->mh_Node);
    }
    /* Ok, lets start up the kernel, we are probably using the UNIX
       kernel, or a variant of that (see config/unix).
    */
    InitCore();

    /* On Linux/m68k where we can run old Amiga binaries, we should
       put SysBase at location 4. On other systems, DON'T DO THIS.
    */
/*
#if defined(__linux__) && defined(__mc68000__)
*/
    if (TRUE == mapSysBase)
    {
      *(APTR *)4 = (APTR)SWAP(SysBase);
      if (mprotect((APTR)0, psize, PROT_READ))
      {
	perror("mprotect");
	exit(10);
      }
    }
/*
#endif
*/
    /* We might also be interested in using the BS key instead of the
       delete key, this will do that
    */
#if 0 /* !defined(__x86_64__) */
    tcgetattr(0, &t);
    t.c_cc[VERASE] = '\b';
#ifndef TCSASOFT
#   define TCSASOFT 0
#endif
    tcsetattr(0, TCSANOW|TCSASOFT, &t);
#endif
    /*	There is nothing more system dependant to set up,
	so lets start that ball rolling...

	The InitCode() call should never return in a working system.
    */
    SysBase->ResModules = romtagList;
    InitCode(RTF_SINGLETASK, 0);
    fprintf(stderr,"Returned from InitCode()\n");
    return 1;
}
