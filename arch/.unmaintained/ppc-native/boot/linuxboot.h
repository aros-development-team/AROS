/*
 *  linux/arch/m68k/boot/amiga/linuxboot.h -- Generic routine to boot Linux/m68k
 *					      on Amiga, used by both Amiboot and
 *					      Amiga-Lilo.
 *
 *	Created 1996 by Geert Uytterhoeven
 *
 *
 *  This file is based on the original bootstrap code (bootstrap.c):
 *
 *	Copyright (C) 1993, 1994 Hamish Macdonald
 *				 Greg Harp
 *
 *		    with work by Michael Rausch
 *				 Geert Uytterhoeven
 *				 Frank Neumann
 *				 Andreas Schwab
 *
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of this archive
 *  for more details.
 */


#include <asm/setup.h>
#include <linux/zorro.h>


    /*
     *  Amiboot Version
     */

#define AMIBOOT_VERSION		"5.5"


    /*
     *  Amiga Bootinfo Definitions
     *
     *  All limits herein are `soft' limits, i.e. they don't put constraints
     *  on the actual parameters in the kernel.
     */

struct amiga_bootinfo {
    u_long machtype;			/* machine type = MACH_AMIGA */
    u_long cputype;			/* system CPU */
    u_long fputype;			/* system FPU */
    u_long mmutype;			/* system MMU */
    int num_memory;			/* # of memory blocks found */
    struct mem_info memory[NUM_MEMINFO];/* memory description */
    struct mem_info ramdisk;		/* ramdisk description */
    char command_line[CL_SIZE];		/* kernel command line parameters */
    u_long model;			/* Amiga Model */
    int num_autocon;			/* # of autoconfig devices found */
    struct ConfigDev autocon[ZORRO_NUM_AUTO];	/* autoconfig devices */
    u_long chip_size;			/* size of chip memory (bytes) */
    u_char vblank;			/* VBLANK frequency */
    u_char psfreq;			/* power supply frequency */
    u_short pad;
    u_long eclock;			/* EClock frequency */
    u_long chipset;			/* native chipset present */
    u_short serper;			/* serial port period */
};


    /*
     *  Parameters passed to linuxboot()
     */

struct linuxboot_args {
    struct amiga_bootinfo bi;	/* Initial values override detected values */
    const char *kernelname;
    const char *ramdiskname;
    int debugflag;
    int keep_video;
    int reset_boards;
    u_int baud;
    void (*puts)(const char *str);
    long (*getchar)(void);
    void (*putchar)(char c);
    void (*printf)(const char *fmt, ...);
    int (*open)(const char *path);
    int (*seek)(int fd, int offset);
    int (*read)(int fd, char *buf, int count);
    void (*close)(int fd);
    int (*filesize)(const char *path);
    void (*sleep)(u_long micros);
    int apus_boot;
    int checksum;
};


    /*
     *  Boot the Linux/m68k Operating System
     */

extern u_long linuxboot(const struct linuxboot_args *args);


    /*
     *  Amiga Models
     */

extern const char *amiga_models[];
extern const u_long first_amiga_model;
extern const u_long last_amiga_model;


    /*
     *	Exec Library Definitions
     */

#define TRUE	(1)
#define FALSE	(0)


struct List {
    struct Node *lh_Head;
    struct Node *lh_Tail;
    struct Node *lh_TailPred;
    u_char lh_Type;
    u_char l_pad;
};

struct MemChunk {
     struct MemChunk *mc_Next;	/* pointer to next chunk */
     u_long mc_Bytes;		/* chunk byte size    */
};

#define MEMF_PUBLIC	(1<<0)
#define MEMF_CHIP	(1<<1)
#define MEMF_FAST	(1<<2)
#define MEMF_LOCAL	(1<<8)
#define MEMF_CLEAR	(1<<16)
#define MEMF_REVERSE (1<<18)

struct MemHeader {
    struct Node mh_Node;
    u_short mh_Attributes;	/* characteristics of this region */
    struct MemChunk *mh_First;	/* first free region */
    void *mh_Lower;		/* lower memory bound */
    void *mh_Upper;		/* upper memory bound+1 */
    u_long mh_Free;		/* total number of free bytes */
};

struct ExecBase {
    u_char fill1[20];
    u_short Version;
    u_char fill2[274];
    u_short AttnFlags;
    u_char fill3[24];
    struct List MemList;
    u_char fill4[194];
    u_char VBlankFrequency;
    u_char PowerSupplyFrequency;
    u_char fill5[36];
    u_long ex_EClockFrequency;
    u_char fill6[60];
};

#define AFB_68020	(1)
#define AFF_68020	(1<<AFB_68020)
#define AFB_68030	(2)
#define AFF_68030	(1<<AFB_68030)
#define AFB_68040	(3)
#define AFF_68040	(1<<AFB_68040)
#define AFB_68881	(4)
#define AFF_68881	(1<<AFB_68881)
#define AFB_68882	(5)
#define AFF_68882	(1<<AFB_68882)
#define AFB_FPU40	(6)		/* ONLY valid if AFB_68040 or AFB_68060 */
#define AFF_FPU40	(1<<AFB_FPU40)	/* is set; also set for 68060 FPU */
#define AFB_68060	(7)
#define AFF_68060	(1<<AFB_68060)

struct Resident;


    /*
     *	Graphics Library Definitions
     */

struct GfxBase {
    u_char fill1[20];
    u_short Version;
    u_char fill2[194];
    u_short NormalDisplayRows;
    u_short NormalDisplayColumns;
    u_char fill3[16];
    u_char ChipRevBits0;
    u_char fill4[307];
};

#define GFXB_HR_AGNUS	(0)
#define GFXF_HR_AGNUS	(1<<GFXB_HR_AGNUS)
#define GFXB_HR_DENISE	(1)
#define GFXF_HR_DENISE	(1<<GFXB_HR_DENISE)
#define GFXB_AA_ALICE	(2)
#define GFXF_AA_ALICE	(1<<GFXB_AA_ALICE)
#define GFXB_AA_LISA	(3)
#define GFXF_AA_LISA	(1<<GFXB_AA_LISA)

    /*
     *	HiRes(=Big) Agnus present; i.e. 
     *	1MB chipmem, big blits (none of interest so far) and programmable sync
     */
#define GFXG_OCS	(GFXF_HR_AGNUS)
    /*
     *	HiRes Agnus/Denise present; we are running on ECS
     */
#define GFXG_ECS	(GFXF_HR_AGNUS|GFXF_HR_DENISE)
    /*
     *	Alice and Lisa present; we are running on AGA
     */
#define GFXG_AGA	(GFXF_AA_ALICE|GFXF_AA_LISA)

#define SETCHIPREV_BEST	(0xffffffff)
#define HIRES		(0x8000)

struct View;


    /*
     *	Amiga Shared Library/Device Functions
     */

extern const struct ExecBase *SysBase;

#define LVOAllocMem		(-0xc6)
#define LVOAllocVec		(-0x2ac)
#define LVOCacheControl		(-0x288)
#define LVODisable		(-0x78)
#define LVOEnable		(-0x7e)
#define LVOFindResident		(-0x60)
#define LVOFreeMem		(-0xd2)
#define LVOFreeVec		(-0x2b2)
#define LVOOpenresource		(-0x1f2)
#define LVOSuperState		(-0x96)
#define LVOSupervisor		(-0x1e)

static __inline void *AllocMem(u_long byteSize, u_long requirements)
{
    register void *_res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register u_long d0 __asm("d0") = byteSize;
    register u_long d1 __asm("d1") = requirements;

    __asm __volatile ("jsr a6@(-0xc6)"
		      : "=r" (_res)
		      : "r" (_base), "r" (d0), "r" (d1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void *AllocVec(u_long byteSize, u_long requirements)
{
    register void *_res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register u_long d0 __asm("d0") = byteSize;
    register u_long d1 __asm("d1") = requirements;

    __asm __volatile ("jsr a6@(-0x2ac)"
		      : "=r" (_res)
		      : "r" (_base), "r" (d0), "r" (d1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline u_long CacheControl(u_long cacheBits, u_long cacheMask)
{
    register u_long _res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register u_long d0 __asm("d0") = cacheBits;
    register u_long d1 __asm("d1") = cacheMask;

    __asm __volatile ("jsr a6@(-0x288)"
		      : "=r" (_res)
		      : "r" (_base), "r" (d0), "r" (d1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline void Disable(void)
{
    register const struct ExecBase *_base __asm("a6") = SysBase;

    __asm __volatile ("jsr a6@(-0x78)"
		      : /* no output */
		      : "r" (_base)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void Enable(void)
{
    register const struct ExecBase *_base __asm("a6") = SysBase;

    __asm __volatile ("jsr a6@(-0x7e)"
		      : /* no output */
		      : "r" (_base)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline struct Resident *FindResident(const u_char *name)
{
    register struct Resident *_res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register const u_char *a1 __asm("a1") = name;

    __asm __volatile ("jsr a6@(-0x60)"
		      : "=r" (_res)
		      : "r" (_base), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return _res;
}

static __inline void FreeMem(void *memoryBlock, u_long byteSize)
{
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register void *a1 __asm("a1") = memoryBlock;
    register u_long d0 __asm("d0") = byteSize;

    __asm __volatile ("jsr a6@(-0xd2)"
		      : /* no output */
		      : "r" (_base), "r" (a1), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void FreeVec(void *memoryBlock)
{
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register void *a1 __asm("a1") = memoryBlock;

    __asm __volatile ("jsr a6@(-0x2b2)"
		      : /* no output */
		      : "r" (_base), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline void *OpenResource(const u_char *resName)
{
    register void *_res  __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register const u_char *a1 __asm("a1") = resName;

    __asm __volatile ("jsr a6@(-0x1f2)"
		      : "=r" (_res)
		      : "r" (_base), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return _res;
}

static __inline void *SuperState(void)
{
    register void *_res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;

    __asm __volatile ("jsr a6@(-0x96)"
		      : "=r" (_res)
		      : "r" (_base)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}

static __inline u_long Supervisor(u_long (*userfunc)(void))
{
    register u_long _res __asm("d0");
    register const struct ExecBase *_base __asm("a6") = SysBase;
    register u_long (*d7)() __asm("d7") = userfunc;

    __asm __volatile ("exg d7,a5;"
		      "jsr a6@(-0x1e);"
		      "exg d7,a5"
		      : "=r" (_res)
		      : "r" (_base), "r" (d7)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}


extern const struct ExpansionBase *ExpansionBase;

#define LVOFindConfigDev	(-0x48)

static __inline struct ConfigDev *FindConfigDev(struct ConfigDev *oldConfigDev,
						long manufacturer, long product)
{
    register struct ConfigDev *_res __asm("d0");
    register const struct ExpansionBase *_base __asm("a6") = ExpansionBase;
    register struct ConfigDev *a0 __asm("a0") = oldConfigDev;
    register long d0 __asm("d0") = manufacturer;
    register long d1 __asm("d1") = product;

    __asm __volatile ("jsr a6@(-0x48)"
		      : "=r" (_res)
		      : "r" (_base), "r" (a0), "r" (d0), "r" (d1)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}


extern const struct GfxBase *GfxBase;

#define LVOLoadView		(-0xde)
#define LVOSetChipRev		(-0x378)

static __inline void LoadView(struct View *view)
{
    register const struct GfxBase *_base __asm("a6") = GfxBase;
    register struct View *a1 __asm("a1") = view;

    __asm __volatile ("jsr a6@(-0xde)"
		      : /* no output */
		      : "r" (_base), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
}

static __inline u_long SetChipRev(u_long want)
{
    register u_long _res __asm("d0");
    register const struct GfxBase *_base __asm("a6") = GfxBase;
    register u_long d0 __asm("d0") = want;

    __asm __volatile ("jsr a6@(-0x378)"
		      : "=r" (_res)
		      : "r" (_base), "r" (d0)
		      : "a0", "a1", "d0", "d1", "memory");
    return(_res);
}


/*
 * PowerUp interface
 */

struct PPCLibBase {
    u_char fill1[20];
    u_short Version;
    u_char fill2[394];
};
	
extern struct PPCLibBase *PPCLibBasePTR;

static __inline void* PPCRunObject(void* object, void* args)
{
    register void *_res  __asm("d0");
    register const struct PPCLibBase *_base __asm("a6") = PPCLibBasePTR;
    register void *a0 __asm("a0") = object;
    register void *a1 __asm("a1") = args;

    __asm __volatile ("jsr a6@(-0x2a)"
		      : "=r" (_res)
		      : "r" (_base), "r" (a0), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return _res;
}

static __inline void* PPCCreateTask(void* object, void* tags)
{
    register void *_res  __asm("d0");
    register const struct PPCLibBase *_base __asm("a6") = PPCLibBasePTR;
    register void *a0 __asm("a0") = object;
    register void *a1 __asm("a1") = tags;

    __asm __volatile ("jsr a6@(-0x54)"
		      : "=r" (_res)
		      : "r" (_base), "r" (a0), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");
    return _res;
}

static __inline void PPCUnloadObject(void* object)
{
    register const struct PPCLibBase *_base __asm("a6") = PPCLibBasePTR;
    register const u_char *a0 __asm("a0") = object;

    __asm __volatile ("jsr a6@(-0x24)"
		      : /* no output */
		      : "r" (_base), "r" (a0)
		      : "a0", "a1", "d0", "d1", "memory");
}

#include <utility/tagitem.h>

static __inline void* PPCLoadObjectTagList(struct TagItem*Tags)
{
 
   register void *_res __asm("d0");
   register const struct PPCLibBase *_base __asm("a6") = PPCLibBasePTR;
   register struct TagItem *a0 __asm("a0") = Tags;
   __asm volatile ("jsr a6@(-0x198:W)"
   : "=r" (_res)
   : "r" (_base), "r" (a0)
   : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");
   return _res;
}

static __inline void* PPCGetTaskAttrs(void* task, struct TagItem* attr)
{
    register const struct PPCLibBase *_base __asm("a6") = PPCLibBasePTR;
    register void *a0 __asm("a0") = task;
    register void *a1 __asm("a1") = attr;

    __asm __volatile ("jsr   a6@(-0x84)\n\t"
	                  "movel d0,a0     \n\t"
		      : "=r" (a0)
		      : "r" (_base), "r" (a0), "r" (a1)
		      : "a0", "a1", "d0", "d1", "memory");

	return a0;
}

static __inline void* PPCCreateMessage(void* body, int len)
{
    register const struct PPCLibBase *_base __asm("a6") = PPCLibBasePTR;
    register void *a0 __asm("a0") = body;
    register int d0 __asm("d0") = len;

    __asm __volatile ("jsr   a6@(-0x126)\n\t"
	                  "movel d0,a0     \n\t"
		      : "=r" (a0)
		      : "r" (_base), "r" (d0), "r" (a0)
		      : "a0", "a1", "d0", "d1", "memory");

	return a0;
}

static __inline int PPCSendMessage(void* port, void* msg, void* data,
                                     int len, int id)
{
    register const struct PPCLibBase *_base __asm("a6") = PPCLibBasePTR;
    register void *a0 __asm("a0") = port;
    register void *a1 __asm("a1") = msg;
    register void *a2 __asm("a2") = data;
    register int d0 __asm("d0") = len;
    register int d1 __asm("d1") = id;

    __asm __volatile ("jsr   a6@(-0x14a)\n\t"
		      : "=r" (d0)
		      : "r" (_base), "r" (d0), "r" (d1), "r" (a0), "r" (a1), "r" (a2)
		      : "a0", "a1", "d0", "d1", "memory");

	return d0;
}

    /*
     *	Bootstrap Support Functions
     */

static __inline void disable_mmu(void)
{
    if (SysBase->AttnFlags & AFF_68040)
	__asm __volatile ("moveq #0,d0;"
			  ".long 0x4e7b0003;"	/* movec d0,tc */
			  ".long 0x4e7b0004;"	/* movec d0,itt0 */
			  ".long 0x4e7b0005;"	/* movec d0,itt1 */
			  ".long 0x4e7b0006;"	/* movec d0,dtt0 */
			  ".long 0x4e7b0007"	/* movec d0,dtt1 */
			  : /* no outputs */
			  : /* no inputs */
			  : "d0");
    else {
	__asm __volatile ("subl #4,sp;"
			  "pmove tc,sp@;"
			  "bclr #7,sp@;"
			  "pmove sp@,tc;"
			  "addl #4,sp");
	if (SysBase->AttnFlags & AFF_68030)
	    __asm __volatile ("clrl sp@-;"
			      ".long 0xf0170800;"	/* pmove sp@,tt0 */
			      ".long 0xf0170c00;"	/* pmove sp@,tt1 */
			      "addql #4,sp");
    }
}
