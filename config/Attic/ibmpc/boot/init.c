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

	At this moment there are following C functions:
	strcmp, strcpy, memset	

*****************************************************************************/

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include "text.h"
#include "logo.h"

unsigned long Memory;	/* Size of whole memory */
unsigned long Memory24;	/* Size of DMA memory (24 bit) */
APTR ssp=(APTR)-1;	/* System stack pointer */
APTR usp=(APTR)-1;	/* User stack pointer */
APTR esp=(APTR)-1;	/* Points to register set on stack */
char supervisor=0;	/* Supervisor mode flag */

struct ExecBase *SysBase=NULL;
struct MemHeader *mh;

extern const struct Resident
    Expansion_resident,
    Exec_resident,
    Utility_resident,
    Aros_resident,
    Mathieeesingbas_resident,
    BOOPSI_resident;

static const struct Resident *romtagList[] =
{
    &Expansion_resident,		/* SingleTask,  110 */
    &Exec_resident,			/* SingleTask,	105 */
    &Utility_resident,			/* ColdStart,	103 */
    &Aros_resident,			/* ColdStart,	102 */
    &Mathieeesingbas_resident,		/* ColdStart,	101 */
    &BOOPSI_resident,			/* ColdStart,	 95 */
    NULL
};

void MakeInt();

int abs(int x)
{
    if (x<0)
	return -x;
    else
	return x;
}    

void put(char a)
{
    if(a) putc_fg(a);
}

void kprintf(char * txt, ...)
{
    Exec_RawDoFmt(txt, &((char *)txt)+1, (void *)&put, 0);
}

void assert(void *sth)
{
    if(!sth)
    {
	kprintf("\nPage zero fault. System halted...");
	while(1);
    }
}

void aros_print_not_implemented(char *name)
{
    kprintf("The function %s is not implemented.\n",name);
}

void _aros_not_implemented()
{
    puts("This function is unfortunately not implemented yet...\n");
}

void show_status(void)
{
unsigned char *p;
int d,i;

  p = KERNEL_DATA;
  puts_fg("\nAROS detected Hardware\nProcessortype: 80");
  d = p[0]-1;
  puti_fg(d);
  puts_fg("86\nAvailable Memory: ");
  d = Memory>>10;
  puti_fg(d);
  puts_fg("kB\n");
  puts_fg("Available DMA Memory: ");
  d = Memory24>>10;
  puti_fg(d);
  puts_fg("kB\n");
  puts_fg("Video: (");
  d = 80;
  puti_fg(d);
  puts_fg("x");
  d = 30;
  puti_fg(d);
  puts_fg(")\n");
  puts_fg("Pointing device: ");
  if(p[0x1ff] == 0xaa)
    puts_fg("installed\n");
  else
    puts_fg("not installed\n");
  puts_fg("HD Drive tables:\nhd0(0x80): 0x");
  for(i=0;i<0x10;i++)
  {
    d = p[0x80+i];
    if( d < 16 )
      putc_fg('0');
    putx_fg(d);
  }
  puts_fg("\nhd1(0x90): 0x");
  for(i=0;i<0x10;i++)
  {
    d = p[0x90+i];
    if( d < 16 )
      putc_fg('0');
    putx_fg(d);
  }
  putc_fg('\n');

}

int main()
{
    ULONG temp;
    static char text[] = "Now booting AROS - The Amiga Research OS\n";
    static char text2[] = "\nOops! Kernel under construction...\n";

/* Get memory size. This code works even with 4GB of memory
   BIOS would have some troubles if you have more than 64MB */

    Memory=0x00100000;
    do
    {
	Memory+=0x10;				/* Step by 16 bytes */
	Memory24=*(ULONG *)Memory;	/* Memory24 is temporary now */
	*(ULONG *)Memory=0xDEADBEEF;
	temp=*(ULONG *)Memory;
	*(ULONG *)Memory=Memory24;
    } while (temp==0xDEADBEEF);
    Memory24=(Memory>0x01000000) ? 0x01000000 : Memory;
  
    showlogo();
    gotoxy(0,0);
    puts_fg(text);
    show_status();

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

    MakeInt();	/* Init IRQ core */    

    if (!(ssp=AllocMem(4096,MEMF_PUBLIC)))	// Alloc 4kb supervisor stack
    {
	kprintf("Supervisor init failed!!!!\nSystem halted...");
	return -1;
    }
    ssp+=4096;

    SysBase->ResModules=romtagList;
    InitCode(RTF_SINGLETASK, 0);

    Debug(0);

    /*
	All done. In normal cases CPU should never reach this instuctions
	Do letter flashing to show working multitasking.
    */

    kprintf("If multitasking works you should see two flashing letters...\n");
    kprintf("Letter 'I' shows working init code loop, letter 'T' shows working TestTask\n");
    
    *(char*)0xb8092=73;
    *(char*)0xb8093=0x0f;
    
    while(1)
    {
	if (*(char*)0xb8092==73) *(char*)0xb8092=32; else *(char*)0xb8092=73;
    }

    puts_fg(text2);
return 0;
}
