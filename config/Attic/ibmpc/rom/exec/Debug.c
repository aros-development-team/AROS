/*
    (C) 1999 AROS - The Amiga Replacement OS
    $Id$

    Desc: Start the internal debugger.
    Lang: english
*/
#include "exec_intern.h"
#include <proto/exec.h>
#include <exec/types.h>

#define Prompt kprintf("SAD(%ld,%ld)>",SysBase->TDNestCnt,SysBase->IDNestCnt)
char	GetK();
void	UnGetK();
ULONG	GetL(char*);
UWORD	GetW(char*);
UBYTE	GetB(char*);
ULONG	strcmp(char*,char*);

/*****************************************************************************

    NAME */

	AROS_LH1(void, Debug,

/*  SYNOPSIS */
	AROS_LHA(unsigned long, flags, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 19, Exec)

/*  FUNCTION
	Runs SAD - internal debuger.

    INPUTS
	flags	not used. Should be 0 now.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	18-01-99    initial PC version.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    char key;
    /* KeyCode -> ASCII conversion table */
    static char transl[] =
    { ' ', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', ' ', ' ', ' ',
      ' ', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', ' ', ' ', 10,
      ' ', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ' ', ' ', ' ',
      ' ', ' ', 'Z', 'X', 'C', 'V', 'B', 'N', 'M' };

    static char command[3]={0,0,0};
    static char data[70];
    
    char *comm=&command[0];
    char *dat=&data[0];

    do
    {
	int i;

	Prompt;

	/* Get Command code */

	for(i=0; i<2; i++)
	{
	    key=transl[GetK()-1];
	    kprintf("%c",key);
	    UnGetK();
	    command[i]=key;
	}
	command[2]=0;
    
	kprintf(" ");
	i=0;
    
	/* Now get data for command */
    
	do
	{
	    key=transl[((GetK()==0x39)?1:GetK())-1];
	    kprintf("%c",key);
	    UnGetK();
	    if(key!=' ') data[i++]=key;
	} while(key!=10 && i<70);
	data[i-1]=0;
    
	/* Reboot command */
	if (strcmp(comm,"RE")==0 && strcmp(dat,"AAAAAAAA")==0) ColdReboot();
	/* Restart command */
	else if (strcmp(comm,"RS")==0 && strcmp(dat,"FFFFFFFF")==0)
		asm("ljmp 0");
	/* Forbid command */
	else if (strcmp(comm,"FO")==0)
		Forbid();
	/* Permit command */
	else if (strcmp(comm,"PE")==0)
		Permit();
	/* Disable command */
	else if (strcmp(comm,"DI")==0)
		Disable();
	/* Enable command */
	else if (strcmp(comm,"EN")==0)
		Enable();
	/* AllocMem command */
	else if (strcmp(comm,"AM")==0)
	{ 
	    ULONG size=GetL(&data[0]);
	    ULONG requim=GetL(&data[8]);
	    kprintf("Allocated at %08.8lx\n",AllocVec(size,requim));
	}
	/* FreeMem command */
	else if (strcmp(comm,"FM")==0)
	{
	    APTR base=(APTR)GetL(&data[0]);
	    kprintf("Freed at %08.8lx\n",base);
	    FreeVec(base);
	}
	/* ReadByte */
	else if (strcmp(comm,"RB")==0)
	    kprintf("Byte at %08.8lx:%02.8lx\n",GetL(&data[0]),
						*(UBYTE*)(GetL(&data[0])));
	/* ReadWord */
	else if (strcmp(comm,"RW")==0)
	    kprintf("Word at %08.8lx:%04.8lx\n",GetL(&data[0]),
						*(UWORD*)(GetL(&data[0])));
	/* ReadLong */
	else if (strcmp(comm,"RL")==0)
	    kprintf("Long at %08.8lx:%08.8lx\n",GetL(&data[0]),
						*(ULONG*)(GetL(&data[0])));
	/* WriteByte */
	else if (strcmp(comm,"WB")==0)
	{
	    kprintf("Byte at %08.8lx:%02.8lx\n",GetL(&data[0]),
						GetB(&data[8]));
	    *(UBYTE*)(GetL(&data[0]))=GetB(&data[8]);
	}
	/* WriteWord */
	else if (strcmp(comm,"WW")==0)
	{
	    kprintf("Word at %08.8lx:%04.8lx\n",GetL(&data[0]),
						GetW(&data[8]));
	    *(UWORD*)(GetL(&data[0]))=GetW(&data[8]);
	}
	/* WriteLong */
	else if (strcmp(comm,"WL")==0)
	{
	    kprintf("Long at %08.8lx:%08.8lx\n",GetL(&data[0]),
						GetL(&data[8]));
	    *(ULONG*)(GetL(&data[0]))=GetL(&data[8]);
	}
	/* ReadArray */
	else if (strcmp(comm,"RA")==0)
	{
	    ULONG ptr;
	    int cnt,t;
	    kprintf("Array from %08.8lx (size=%08.8lx):\n",GetL(&data[0]),
							GetL(&data[8]));
	    ptr=GetL(&data[0]);
	    cnt=(int)GetL(&data[8]);
	    for(t=1;t<=cnt;t++)
	    {
		kprintf("%02.2lx ",*(UBYTE*)ptr);
		ptr++;
		if(!(t%16)) kprintf("\n");
	    }
	    kprintf("\n");
	}
	/* ReadASCII */
	else if (strcmp(comm,"RC")==0)
	{
	    ULONG ptr;
	    int cnt,t;
	    kprintf("ASCII from %08.8lx (size=%08.8lx):\n",GetL(&data[0]),
							GetL(&data[8]));
	    ptr=GetL(&data[0]);
	    cnt=(int)GetL(&data[8]);
	    for(t=1;t<=cnt;t++)
	    {
		kprintf("%c",*(char*)ptr);
		ptr++;
		if(!(t%70)) kprintf("\n");
	    }
	    kprintf("\n");
	}
	else kprintf("??\n");
    } while(strcmp(comm,"QT")!=0 || strcmp(dat,"00000000")!=0);
    
    kprintf("Quitting SAD...\n");
    
    AROS_LIBFUNC_EXIT
} /* Debug */

void UnGetK()
{
    register char temp asm ("al");
    do
    {
    /* Wait untill key is released */
	asm("movl	$0,	%eax");    
	asm("inb	$0x60,	%al");
    } while(!(temp & 0x80));
}

char GetK()
{
    register char temp asm ("al");
    do
    {
	asm("movl	$0,	%eax");    
	asm("inb	$0x60,	%al");
    } while(temp & 0x80);
    return(temp);
}

ULONG GetL(char* string)
{
    ULONG ret=0;
    int i;
    char digit;
    
    for(i=0;i<8;i++)
    {
	digit=(*string++) - '0';
	if (digit>9) digit-='A' - '0' - 10;
	ret=(ret<<4)+digit;
    }
    return(ret);
}

UWORD GetW(char* string)
{
    UWORD ret=0;
    int i;
    char digit;
    
    for(i=0;i<4;i++)
    {
	digit=(*string++) - '0';
	if (digit>9) digit-='A' - '0' - 10;
	ret=(ret<<4)+digit;
    }
    return(ret);
}

UBYTE GetB(char* string)
{
    UBYTE ret=0;
    int i;
    char digit;
    
    for(i=0;i<2;i++)
    {
	digit=(*string++) - '0';
	if (digit>9) digit-='A' - '0' - 10;
	ret=(ret<<4)+digit;
    }
    return(ret);
}
