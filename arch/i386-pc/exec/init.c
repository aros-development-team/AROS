/*
    (C) 1997-1999 AROS - The Amiga Research OS
    $Id$

    Desc: Begining of AROS kernel
    Lang: English
*/

/*****************************************************************************

    FUNCTION
	This is the main file in Native PC AROS. The main function is called
	by head.S file. All this code is in flat 32-bit mode.

	This file will make up the exec base, memory and other stuff. We don't
	need any malloc functions, because there is no OS yet. To be honest:
	while main is running Native PC AROS is the only OS on our PC.

	Be careful. You cant use any stdio. Only your own functions are
	acceptable at this moment!!

*****************************************************************************/

#define AROS_USE_OOP
#define AROS_ALMOST_COMPATIBLE 1

#include <aros/config.h>
#include <exec/io.h>
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <proto/oop.h>
#include <proto/exec.h>

//#include <graphics/gfx.h>
//#include <intuition/intuition.h>
//#include <utility/tagitem.h>
//#include <proto/graphics.h>
//#include <proto/intuition.h>
#include <devices/keyboard.h>

#include <hidd/hidd.h>
#include <hidd/serial.h>
#include <hidd/irq.h>

#include <memory.h>

#include "traps.h"

void Handler(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);

void hidd_demo();

unsigned long Memory;	/* Size of whole memory */
unsigned long Memory24; /* Size of DMA memory (24 bit) */

extern ULONG GetCPUSpeed();
extern char* GetCPUName();
extern void DetectCPU();

extern ULONG SSP;

struct ExecBase *SysBase=NULL;
struct MemHeader *mh;

//struct Library *IntuitionBase=NULL;

extern int sprintf(char *,const char *,...);

ULONG GetMemSize();

extern const struct Resident
    Expansion_resident,
    Exec_resident,
    Utility_resident,
    Aros_resident,
    Mathieeesingbas_resident,
    BOOPSI_resident,
    OOP_resident,
    HIDD_resident,
    irqHidd_resident,
    Graphics_resident,
    Layers_resident,
    Timer_resident,
    Misc_resident,
    Battclock_resident,
    Keyboard_resident,
    Gameport_resident,
    Keymap_resident,
    Input_resident,
    Intuition_resident,
    hiddgraphics_resident,
    kbdHidd_resident,
    vgaHidd_resident,
    hiddserial_resident,
    pciHidd_resident,
    Console_resident,
    TrackDisk_resident,
    ide_resident,
    Workbench_resident,
    Mathffp_resident;

/* This list MUST be in the correct order (priority). */
static const struct Resident *romtagList[] =
{
    &Expansion_resident,            /* SingleTask,  110  */
    &Exec_resident,                 /* SingleTask,  105  */
    &Utility_resident,              /* ColdStart,   103  */
    &Aros_resident,                 /* ColdStart,   102  */
//    &Mathieeesingbas_resident,      /* ColdStart,   101  */
    &BOOPSI_resident,               /* ColdStart,   95	 */
    &OOP_resident,                  /* ColdStart,   94	 */
    &HIDD_resident,                 /* ColdStart,   92	 */
    &irqHidd_resident,              /* ColdStart,   90   */	// IRQ Hidd!
    &Graphics_resident,             /* ColdStart,   65	 */
    &Layers_resident,               /* ColdStart,   60   */
    &Timer_resident,                /* ColdStart,   50	 */
    &Misc_resident,                 /* ColdStart,   45   */
    &Battclock_resident,            /* ColdStart,   45	 */
    &Keyboard_resident,             /* ColdStart,   44	 */
    &Gameport_resident,             /* ColdStart,   43	 */
    &Keymap_resident,		    /* ColdStart,   40	 */
    &Input_resident,                /* ColdStart,   30	 */
    &Intuition_resident,            /* ColdStart,   10   */	
    &hiddgraphics_resident,	    /* ColdStart,   9    */
    &kbdHidd_resident,              /* ColdStart,   9    */
    &vgaHidd_resident,		    /* ColdStart,   9    */
    &hiddserial_resident,	    /* ColdStart,   9    */
    &pciHidd_resident,              /* ColdStart,   9    */
    &Console_resident,              /* ColdStart,   5	 */
#if !AROS_BOCHS_HACK
    /* BOCHS doesn't like something in there: error "unsupported CMOS read, address = 0x701" */
    &TrackDisk_resident,	    /* ColdStart,   4    */	//Trackdisk		
#endif
    &ide_resident,                  /* ColdStart,   4    */	//IDE device

//    &emul_handler_resident,		    /* ColdStart,   0	 */
//    &Workbench_resident,		    /* ColdStart,  -120  */
//    &Mathffp_resident,			    /* ColdStart,  -120  */

    /*
	NOTE: You must not put anything between these two; the code which
	initialized boot_resident will directly call Dos_resident and
	anything between the two will be skipped.
    */
//    &boot_resident,			    /* ColdStart,  -50	 */
//    &Dos_resident,			    /* None,	   -120  */
//    &LDDemon_resident,		    /* AfterDOS,   -125  */
//    &con_handler_resident,		    /* AfterDOS,   -126  */


    NULL
};

/************************************************************************************/

struct scr
{
	unsigned char sign;
	unsigned char attr;
};

static void callback(UBYTE chr, UBYTE *i)
{
	struct scr	*view = (struct scr *)0xb8000;

	if (chr)
	{
		view[(*i)++].sign = chr;
	}
}

/************************************************************************************/

#ifdef kprintf
#undef kprintf
#endif /* kprintf */

int main()
{
    char text0[] = "AROS - The Amiga Research OS\n";
    char text1[60];
    char text2[] = "\nOops! Kernel under construction...\n";

    OOP_Object *o;

/* Get memory size. This code works even with 4GB of memory
   BIOS would have some troubles if you have more than 64MB */

    Memory=GetMemSize();    
    Memory24=(Memory>0x01000000) ? 0x01000000 : Memory;

    Init_Traps();

    InitGfxAROS();
    DetectCPU();

    AROS_InfoText(0,(char*)&text0);
    sprintf((char*)&text1,"4.0 ROM (%s)\n",__DATE__);
    AROS_InfoText(1,(char*)&text1);
    sprintf((char*)&text1,"CPU: %s %ldMHz\n",
		GetCPUName(),
		GetCPUSpeed()/1000000);
    AROS_InfoText(2,(char*)&text1);
    sprintf((char*)&text1,"RAM size: %dMB\n",(int)Memory>>20);
    AROS_InfoText(3,(char*)&text1);
    
    /*
        Prepare first memory list. Our memory will statr at 0x00100000. First
        1MB is reserved for kernel use only. DO NOT use it please.
    */

    mh=(struct MemHeader*)0x00100000;
    mh->mh_Node.ln_Type = NT_MEMORY;
    mh->mh_Node.ln_Name = "chip memory";
    mh->mh_Node.ln_Pri = -5;
    mh->mh_Attributes = MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA |
			MEMF_KICK;
    mh->mh_First = (struct MemChunk *)((UBYTE*)mh+MEMHEADER_TOTAL);
    mh->mh_First->mc_Next = NULL;
    mh->mh_First->mc_Bytes = Memory24 - 0x00100000 - MEMHEADER_TOTAL;
    mh->mh_Lower = mh->mh_First;
    mh->mh_Upper = (APTR)Memory24;
    mh->mh_Free = mh->mh_First->mc_Bytes;

    /*
        We have to put somewhere in this function checking for ColdStart,
        CoolStart and many other Exec vectors!
    */

    /*
        It is OK to place ExecBase here. Remember that interrupt table starts
        at 0x0100UL address, so 4UL is quite safe.
        Even with MP this addr is OK for ExecBase. We may write an int handler
        which detects "read from 4UL" commands.
    */
    SysBase=(struct ExecBase*)PrepareExecBase(mh);
    *(APTR *)4=SysBase;

    /*
        Setup ChkBase (checksum for base ptr), ChkSum (for library)
        SysBase+ChkBase should be -1 otherwise somebody has destroyed ExecBase!
    */
    SysBase->ChkBase=~(ULONG)SysBase;
#warning TODO: SysBase->ChkSum=.....

    if (Memory>Memory24)
    {
        AddMemList(Memory-Memory24, MEMF_FAST | MEMF_PUBLIC | MEMF_KICK |
            MEMF_LOCAL, 10, (APTR)0x01000000, "fast memory");
    }

    if (!(SSP=AllocMem(4096,MEMF_PUBLIC)))      // Alloc 4kb supervisor stack
    {
        kprintf("Supervisor init failed!!!!\nSystem halted...");
        do {} while(1);
    }
    SSP+=4088;  /* Leave 2 longwords in case we would have to jump 
                   directly from IOPL0 to IOPL3 */

    SysBase->ResModules=romtagList;
    InitCode(RTF_SINGLETASK, 0);

    kprintf("Starting SAD\n");

	Debug(0);

    #define ioStd(x) ((struct IOStdReq *)x)

    /* Small kbdhidd test */
    {
	struct IORequest *io;
	struct MsgPort *mp;

	kprintf("Opening kbd.hidd\n");
	kprintf("Got: %08.8lx\n",OpenLibrary("kbd.hidd",0));
	mp=CreateMsgPort();
	io=CreateIORequest(mp,sizeof(struct IOStdReq));
	kprintf("Result of opening device %d\n",
	    OpenDevice("keyboard.device",0,io,0));
	kprintf("Doing CMD_HIDDINIT...\n");
	{
	    UBYTE *data;
	    data = AllocMem(100, MEMF_PUBLIC);
	    strcpy(data, "hidd.kbd.hw");
	    ioStd(io)->io_Command=32000;
	    ioStd(io)->io_Data=data;
	    ioStd(io)->io_Length=strlen(data);
	    DoIO(io);
	    kprintf("Got io_ERROR=%d\n",ioStd(io)->io_Error);
	    kprintf("Doing kbd reads...\n");
	}
    }
    
    hidd_demo();

    /*
	All done. In normal cases CPU should never reach this instructions
    */

	kprintf(text2);

	do {} while(1);
}
