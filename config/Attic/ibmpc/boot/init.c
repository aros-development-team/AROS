/*
    (C) 1997-98 AROS - The Amiga Research OS
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

#include "text.h"
#include "logo.h"

unsigned long Memory;	/* Size of whole memory */
unsigned long Memory24;	/* Size of DMA memory (24 bit) */
unsigned long ssp=-1;	/* System stack pointer */
unsigned long usp=-1;	/* User stack pointer */
unsigned long esp=-1;	/* Points to register set on stack */
int supervisor=1;	/* Supervisor mode flag */

struct ExecBase *SysBase=NULL;
struct MemHeader *mh;

int abs(int x)
{
    if (x<0)
	return -x;
    else
	return x;
}    

void assert(void *sth)
{
    if(sth==0)
    {
	puts_fg("\nPage zero fault. System halted...");
	while(1);
    }
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
    unsigned long temp;
    static char text[] = "Now booting AROS - The Amiga Research OS\n";
    static char text2[] = "\nOops! Kernel under construction...\n";

/* Get memory size. This code works even with 4GB of memory
   BIOS would have some troubles if you have more than 64MB */

    Memory=0x00100000;
    do
    {
	Memory+=0x10;				/* Step by 16 bytes */
	Memory24=*(unsigned long *)Memory;	/* Memory24 is temporary now */
	*(unsigned long *)Memory=0xDEADBEEF;
	temp=*(unsigned long *)Memory;
	*(unsigned long *)Memory=Memory24;
    } while (temp==0xDEADBEEF);
    Memory24=(Memory>0x01000000) ? 0x01000000 : Memory;
  
    showlogo();
    gotoxy(0,0);
    puts_fg(text);
    show_status();

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

    puts_fg(text2);
return 0;
}
