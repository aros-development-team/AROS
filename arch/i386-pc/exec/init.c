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

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <proto/exec.h>

unsigned long Memory;	/* Size of whole memory */
unsigned long Memory24; /* Size of DMA memory (24 bit) */
APTR ssp=(APTR)-1;      /* System stack pointer */
APTR usp=(APTR)-1;      /* User stack pointer */
APTR esp=(APTR)-1;      /* Points to register set on stack */
char supervisor=0;	/* Supervisor mode flag */

struct ExecBase *SysBase=NULL;
struct MemHeader *mh;

void bzero(void * ptr,int len);
void kprintf(const char * string,...);
struct ExecBase * PrepareExecBase(struct MemHeader * mh);

/* External functions */

extern void InitGfxAROS();
extern void AROS_InfoText(int num, char * text);

extern ULONG GetCPUSpeed();
extern char* GetCPUName();
extern void DetectCPU();

extern int sprintf(char *,const char *,...);

extern const struct Resident
    Expansion_resident,
    Exec_resident,
    Utility_resident,
    Aros_resident,
    BOOPSI_resident,
    OOP_resident,
    HIDD_resident,
    Graphics_resident,
    Layers_resident,
    Timer_resident,
    Battclock_resident,
    Keyboard_resident,
    Gameport_resident,
    Keymap_resident,
    Input_resident,
//    Intuition_resident,
//    Console_resident,
    Mathffp_resident,
    Mathieeesingbas_resident,
    Workbench_resident;
//    Dos_resident,
//    LDDemon_resident,
//    emul_handler_resident,
//    con_handler_resident;

/* This list MUST be in the correct order (priority). */
static const struct Resident *romtagList[] =
{
    &Expansion_resident,		    /* SingleTask,  110  */
    &Exec_resident,			    /* SingleTask,  105  */
    &Utility_resident,			    /* ColdStart,   103  */
    &Aros_resident,			    /* ColdStart,   102  */
    &Mathieeesingbas_resident,              /* ColdStart,   101  */
    &BOOPSI_resident,			    /* ColdStart,   95	 */
    &OOP_resident,			    /* ColdStart,   94	 */
    &HIDD_resident,			    /* ColdStart,   92	 */
    &Graphics_resident, 		    /* ColdStart,   65	 */
    &Layers_resident,			    /* ColdStart,   60   */
    &Timer_resident,			    /* ColdStart,   50	 */
    &Battclock_resident,		    /* ColdStart,   45	 */
    &Keyboard_resident,			    /* ColdStart,   44	 */
    &Gameport_resident,			    /* ColdStart,   43	 */
    &Keymap_resident,			    /* ColdStart,   40	 */
    &Input_resident,			    /* ColdStart,   30	 */
//    &Intuition_resident,		    /* ColdStart,   10	 */
//    &Console_resident,			    /* ColdStart,   5	 */
//    &emul_handler_resident,		    /* ColdStart,   0	 */
    &Workbench_resident,		    /* ColdStart,  -120  */
    &Mathffp_resident,			    /* ColdStart,  -120  */

    /*
	NOTE: You must not put anything between these two; the code which
	initialized boot_resident will directly call Dos_resident and
	anything between the two will be skipped.
    */
//    &boot_resident,			    /* ColdStart,  -50	 */
//    &Dos_resident,			    /* None,	   -120  */
//    &LDDemon_resident,			    /* AfterDOS,   -125  */
//    &con_handler_resident,		    /* AfterDOS,   -126  */

    NULL
};

void MakeInt();

void __bzero(void * ptr,int len)
{
    bzero(ptr,len);
}

int abs(int x)
{
    if (x<0)
	return -x;
    else
	return x;
}

/*void _aros_not_implemented()
{
    puts("This function is unfortunately not implemented yet...\n");
}*/

int main()
{
    ULONG temp;
    char text0[] = "AROS - The Amiga Research OS\n";
    char text1[60];
    char text2[] = "\nOops! Kernel under construction...\n";

/* Get memory size. This code works even with 4GB of memory
   BIOS would have some troubles if you have more than 64MB */

    Memory=0x00100000;
    do
    {
	Memory+=0x10;	/* Step by 16 bytes. If it's too slow you can adjust it */
	Memory24=*(ULONG *)Memory;      /* Memory24 is temporary now */
	*(ULONG *)Memory=0xDEADBEEF;
	temp=*(ULONG *)Memory;
	*(ULONG *)Memory=Memory24;
    } while (temp==0xDEADBEEF);
    Memory24=(Memory>0x01000000) ? 0x01000000 : Memory;

    DetectCPU();

    InitGfxAROS();
    
    AROS_InfoText(0,(char*)&text0);
    AROS_InfoText(1,"4.0 ROM\n");
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
    mh->mh_Node.ln_Name = "24bit memory";
    mh->mh_Node.ln_Pri = -5;
    mh->mh_Attributes = MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA |
			MEMF_KICK;
    mh->mh_First = (struct MemChunk *)((UBYTE *)mh + sizeof(struct MemHeader));
    mh->mh_First->mc_Next = NULL;
    mh->mh_First->mc_Bytes = Memory24 - 0x00100000 - sizeof(struct MemHeader);
    mh->mh_Lower = mh->mh_First;
    mh->mh_Upper = (APTR)Memory24;
    mh->mh_Free = mh->mh_First->mc_Bytes;

    /*
	We have to put somewhere in this function checking for ColdStart,
	CoolStart and many other Exec vectors!
    */

    /*
	It is OK to place ExecBase here. Remember that interrupt table starts
	at 8UL address, so 4UL is quite safe.
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
    /* TODO: SysBase->ChkSum=..... */

    if (Memory>Memory24)
    {
	AddMemList(Memory-Memory24, MEMF_FAST | MEMF_PUBLIC | MEMF_KICK |
		    MEMF_LOCAL, 10, (APTR)0x01000000, "fast memory");
    }

    MakeInt();  /* Init IRQ core */

    if (!(ssp=AllocMem(4096,MEMF_PUBLIC)))      // Alloc 4kb supervisor stack
    {
	kprintf("Supervisor init failed!!!!\nSystem halted...");
	return -1;
    }
    ssp+=4096;

    SysBase->ResModules=romtagList;
    InitCode(RTF_SINGLETASK, 0);

    /* Enter SAD */
    Debug(0);

    /*
	All done. In normal cases CPU should never reach this instuctions
    */

    kprintf(text2);
return 0;
}
