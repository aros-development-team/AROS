/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: $(ARCH) init code for emulated (Unix) systems.
    Lang: english
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/resident.h>
#include <exec/execbase.h>

#include <proto/exec.h>

#define _XOPEN_SOURCE 600L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>

#include "../../../rom/exec/memory.h"

char *malloc_options;

extern const struct Resident
    Expansion_ROMTag,
    Exec_resident,
    Utility_ROMTag,
    Aros_ROMTag,
    OOP_ROMTag,
    HIDDCl_ROMTag,
    UXIO_ROMTag,
    HostLib_ROMTag,
    Graphics_ROMTag,
    Layers_ROMTag,
    Timer_ROMTag,
    Battclock_ROMTag,
    Keyboard_ROMTag,
    Gameport_ROMTag,
    Keymap_ROMTag,
    Input_ROMTag,
    Intuition_ROMTag,
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
    Packet_ROMTag,
    UXSer_ROMTag,
    UXPar_ROMTag,
    boot_resident,
    Con_ROMTag,
    Nil_ROMTag,
    Ram_ROMTag,
    Dosboot_ROMTag,
    GFX_ROMTag,
#if ENABLE_X11 == 1
    X11Cl_ROMTag,
#endif
    Bootmenu_ROMTag;

/* This list MUST be in the correct order (priority). */
static const struct Resident *romtagList[] =
{
    &Expansion_ROMTag,                  /* SingleTask,  110  */
    &Exec_resident,                     /* SingleTask,  126  */
    &Utility_ROMTag,                    /* ColdStart,   103  */
    &Aros_ROMTag,                       /* ColdStart,   102  */
    &Bootloader_ROMTag,			/* ColdStart,	100  */
    &OOP_ROMTag,                        /* ColdStart,   94   */
    &HIDDCl_ROMTag,                     /* ColdStart,   92   */
    &UXIO_ROMTag,                       /* ColdStart,   91   */
    &HostLib_ROMTag,                    /* ColdStart,   91   */
    &Graphics_ROMTag,                   /* ColdStart,   65   */
    &Layers_ROMTag,                     /* ColdStart,   60   */
    &Timer_ROMTag,                      /* ColdStart,   50   */
    &Battclock_ROMTag,                  /* ColdStart,   45   */
    &Keyboard_ROMTag,                   /* ColdStart,   44   */
    &Gameport_ROMTag,                   /* ColdStart,   44   */
    &Keymap_ROMTag,                     /* ColdStart,   40   */
    &Input_ROMTag,                      /* ColdStart,   30   */
    &Intuition_ROMTag,                  /* ColdStart,   15   */
    &GFX_ROMTag,			/* ColdStart,   10   */
#if ENABLE_X11 == 1
    &X11Cl_ROMTag,			/* ColdStart,   9    */
#endif
    &Cybergraphics_ROMTag,              /* ColdStart,   8    */
    &Console_ROMTag,                    /* ColdStart,   5    */
#if ENABLE_DBUS == 1
    &Dbus_ROMTag,                       /* ColdStart,   0    */
#endif
    &emul_handler_ROMTag,               /* ColdStart,   0    */
    &UXSer_ROMTag,                      /* ColdStart,   0    */
    &UXPar_ROMTag,                      /* ColdStart,   0    */

    /*
        NOTE: You must not put anything between these two; the code
        which initialized boot_resident will directly call
        Dos_resident and anything between the two will be skipped.
    */
    &boot_resident,                     /* ColdStart,  -50   */
    &Dos_ROMTag,                        /* None,       -120  */
    &LDDemon_resident,                  /* AfterDOS,   -123  */
    &Con_ROMTag,                        /* AfterDOS,   -124  */
    &Packet_ROMTag,                     /* AfterDOS,   -124  */
    &Nil_ROMTag,                        /* AfterDOS,   -125  */
    &Ram_ROMTag,                        /* AfterDOS,   -125  */    
    &Bootmenu_ROMTag,                   /* AfterDOS,   -127  */
    &Dosboot_ROMTag,                    /* AfterDOS,   -128  */
    NULL
};

/* So we can examine the memory */
static struct MemHeaderExt mhe;
static struct MemHeader *mh = &mhe.mhe_MemHeader;
UBYTE *memory, *space;
int memSize = 32;

extern void InitCore(void);
extern struct ExecBase *PrepareExecBase(struct MemHeader *mh);

char *join_string(int argc, char **argv)
{
    char *str, *s;
    int j;
    int x = 0;

    for (j = 0; j < argc; j++)
	x += (strlen(argv[j]) + 1);
    D(printf("[Init] Allocating %lu bytes for string\n", x));
    str = malloc(x);
    if (str) {
	s = str;
	for (j = 0; j < argc; j++) {
	    strcpy(s, argv[j]);
	    s += strlen(s);
	    *s++ = ' ';
	}
	s[-1] = 0;
	D(printf("[Init] Joined line: %s\n", str));
    }
    return str;
}

extern char _start, _end;
char *BootLoader_Name = "FreeBSD";
char *Kernel_Args = NULL;
char **Kernel_ArgV;

/*
    This is where AROS is first called by whatever system loaded it,
    either some kind of boot loader, or a "parent" operating system.

    For boot loaded $(ARCH), you don't need to define main() like this,
    you can have it anyway your bootloader likes.
*/

int main(int argc, char **argv)
{
    struct ExecBase *SysBase;
    struct termios t;
    int psize = 0;
    int i = 0, x;
    BOOL mapSysBase = FALSE;

    while (i < argc)
    {
      if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
      {
        printf("AROS for FreeBSD\n");
        printf("usage: %s [options] [kernel arguments]\n",argv[0]);
	printf("Availible options:\n");
        printf(" -h                 show this page\n");
        printf(" -m <size>          allocate <size> Megabytes of memory for AROS\n");
        printf(" -M                 allows programs to read SysBase from Address $4; SysBase is");
        printf("                     found there in big endian format\n");
        printf(" --help             same as '-h'\n");
        printf(" --memsize <size>   same as '-m <size>'\n");
        printf(" --mapsysbase       same as '-M'\n");
        printf("\nPlease report bugs to the AROS development team. http://www.aros.org\n");
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
        break;
    }

    if (i < argc)
	Kernel_Args = join_string(argc - i, &argv[i]);
    Kernel_ArgV = argv;

    /*
    First up, set up the memory.

    If your memory starts at 0 (I think Linux does, FreeBSD doesn't),
    then you can allocate 4K at that address, and do whatever you want
    to make that invalid to trap NULL dereference errors.

    */
#ifdef __linux__
    space = malloc(4096);
    if(space)
    {
        int size = 4096/sizeof(ULONG);
        while(--size)
            *space++ = 0xDEADBEEF;
    }
#endif
    /*
        Magic, this makes FreeBSD's malloc() print out lots of extra
        debugging information, and more to the point, call abort()
        when something naughty happens.
    */
    malloc_options = "A";

    /* We allocate memSize megabytes, plus a little extra */
    memory = malloc((memSize << 20) + MEMCHUNK_TOTAL);
    if( !memory )
    {
         /*fprintf(stderr, "Cannot allocate any memory!\n");*/
         exit(20);
    }

    /* Prepare the first mem header */
    mh->mh_Node.ln_Name = "chip memory";
    mh->mh_Node.ln_Pri = -5;
    mh->mh_Attributes = MEMF_CHIP | MEMF_PUBLIC;
    mh->mh_First = (struct MemChunk *)
            (((IPTR)memory + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1));
    mh->mh_First->mc_Next = NULL;
    mh->mh_First->mc_Bytes = memSize << 20;
    mh->mh_Lower = memory;
    mh->mh_Upper = memory + MEMCHUNK_TOTAL + mh->mh_First->mc_Bytes;
    mh->mh_Free = mh->mh_First->mc_Bytes;

    /*
        This will prepare enough of ExecBase to allow us to
        call functions, it will also set up the memory list.
    */
    SysBase = PrepareExecBase(mh);

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
        mh->mh_Free = 0;
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
#if defined(__linux__) && defined(__mc68000__)
    if( mmap((APTR)0, getpagesize(), PROT_READ|PROT_WRITE,
        MAP_ANON|MAP_PRIVATE|MAP_FIXED, -1, 0) != (APTR)0 )
    {
        perror("mmap: Can't map page 0\n");
        exit(10);
    }

    *(APTR *)4 = SysBase;
    if(mprotect((APTR)0,getpagesize(), PROT_READ))
    {
        perror("mprotect");
        exit(10);
    }
#endif

    /* We might also be interested in using the BS key instead of the
       delete key, this will do that
    */
    tcgetattr(0, &t);
    t.c_cc[VERASE] = '\b';
#ifndef TCSASOFT
#   define TCSASOFT 0
#endif
    tcsetattr(0, TCSANOW|TCSASOFT, &t);

    /*  There is nothing more system dependant to set up,
        so lets start that ball rolling...
    
        The InitCode() call should never return in a working system.
    */
    SysBase->ResModules = romtagList;
    InitCode(RTF_SINGLETASK, 0);
    fprintf(stderr,"Returned from InitCode()\n");   
    return 1;
}
