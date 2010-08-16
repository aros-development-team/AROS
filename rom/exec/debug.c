/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal debugger.
    Lang: english
*/

#include <string.h>
#include "exec_intern.h"
#include <proto/exec.h>
#include <exec/types.h>

#include "etask.h"

/****************************************************************************************/

#define Prompt 		kprintf("SAD(%ld,%ld)>",SysBase->TDNestCnt,SysBase->IDNestCnt)

/*#define GetHead(l)      (void *)(((struct List *)l)->lh_Head->ln_Succ \
				? ((struct List *)l)->lh_Head \
				: (struct Node *)0)
#define GetSucc(n)      (void *)(((struct Node *)n)->ln_Succ->ln_Succ \
				? ((struct Node *)n)->ln_Succ \
				: (struct Node *)0)
*/
/****************************************************************************************/

char	GetK();
void	UnGetK();
ULONG	GetL(char*);
UWORD	GetW(char*);
UBYTE	GetB(char*);
int	get_irq_list(char *buf);

/****************************************************************************************/


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
    static char command[3] = {0, 0, 0};
    static char data[70];
    
    char 	*comm = &command[0];
    char 	*dat  = &data[0];

    RawIOInit();

    do
    {
	int i;

	Prompt;

	/* Get Command code */

	for(i = 0; i < 2; i++)
	{
	    key = GetK(SysBase);
	    kprintf("%c", key);
	    UnGetK();
	    command[i] = key;
	}
	command[2] = 0;
    
	kprintf(" ");
	i = 0;
    
	/* Now get data for command */
    
	do
	{
	    key = GetK(SysBase);
	    if (key != 10) kprintf("%c",key);
	    else kprintf("\n");
	    UnGetK();
	    if(key != ' ') data[i++]=key;
	} while(key !=10 && i < 70);
	data[i - 1] = 0;
    
	/* Reboot command */
	if (strcmp(comm, "RE") == 0 && strcmp(dat, "AAAAAAAA") == 0)
	    ColdReboot();
	/* Restart command */
	else if (strcmp(comm, "RS") == 0 && strcmp(dat, "FFFFFFFF") == 0)
	    ShutdownA(SD_ACTION_COLDREBOOT);
	/* Forbid command */
	else if (strcmp(comm, "FO") == 0)
	    Forbid();
	/* Permit command */
	else if (strcmp(comm, "PE") == 0)
	    Permit();
	/* Disable command */
	else if (strcmp(comm, "DI") == 0)
	    Disable();
	/* Show active task information */
	else if (strcmp(comm, "TI") == 0)
	{
	    struct Task *t = SysBase->ThisTask;

	    kprintf("Active task (%p = '%s'):\n"
			    "tc_Node.ln_Pri = %d\n"
			    "tc_SigAlloc = %04.4lx\n"
			    "tc_SPLower = %p\n"
			    "tc_SPUpper = %p\n"
			    "tc_Flags = %08.8lx\n"
			    "tc_SPReg = %p\n",
			    t, t->tc_Node.ln_Name,
			    t->tc_Node.ln_Pri,
			    t->tc_SigAlloc,
			    t->tc_SPLower,
			    t->tc_SPUpper,
			    t->tc_Flags,
			    t->tc_SPReg);				
	}
/*	else if (strcmp(comm,"RI") == 0)
	{
	    struct pt_regs *r = (struct pt_regs *)
			    GetIntETask(SysBase->ThisTask)->iet_Context;

	    kprintf("Active task's registers dump:\n"
			    "EAX=%p  ECX=%p  EDX=%p  EIP=%p\n"
			    "CS=%04.4lx  DS=%04.4lx  ES=%04.4lx\n"
			    "SS=%04.4lx  EFLAGS=%p\n",
			    r->eax, r->ecx, r->edx,
			    r->eip, r->xcs, r->xds, r->xes,
			    r->xss, r->eflags);				
	}*/
	/* Enable command */
	else if (strcmp(comm, "EN") == 0)
	    Enable();
	/* ShowLibs command */
	else if (strcmp(comm, "SL") == 0)
	{
	    struct Node * node;

	    kprintf("Available libraries:\n");

	    /* Look through the list */
	    for (node = GetHead(&SysBase->LibList); node; node = GetSucc(node))
	    {
		kprintf("0x%08.8lx : %s\n", node, node->ln_Name);
	    }
	}
	else if (strcmp(comm, "SI") == 0)
	{
	    char buf[512];
	    
	    kprintf("Available interrupts:\n");
	    
//	    get_irq_list(&buf);

//	    kprintf(buf);
	}
	/* ShowResources command */
	else if (strcmp(comm, "SR") == 0)
	{
	    struct Node * node;

	    kprintf("Available resources:\n");

	    /* Look through the list */
	    for (node = GetHead(&SysBase->ResourceList); node; node = GetSucc(node))
	    {
		kprintf("0x%08.8lx : %s\n", node, node->ln_Name);
	    }
	}
	/* ShowDevices command */
	else if (strcmp(comm,"SD") == 0)
	{
	    struct Node * node;

	    kprintf("Available devices:\n");

	    /* Look through the list */
	    for (node=GetHead(&SysBase->DeviceList); node; node = GetSucc(node))
	    {
		kprintf("0x%08.8lx : %s\n", node, node->ln_Name);
	    }
	}
	/* ShowTasks command */
	else if (strcmp(comm, "ST") == 0)
	{
	    struct Node * node;

	    kprintf("Task List:\n");

	    kprintf("0x%08.8lx T %d %s\n",SysBase->ThisTask,
		SysBase->ThisTask->tc_Node.ln_Pri,
		SysBase->ThisTask->tc_Node.ln_Name);

	    /* Look through the list */
	    for (node = GetHead(&SysBase->TaskReady); node; node = GetSucc(node))
	    {
		kprintf("0x%08.8lx R %d %s\n", node, node->ln_Pri, node->ln_Name);
	    }

	    for (node = GetHead(&SysBase->TaskWait); node; node = GetSucc(node))
	    {
		kprintf("0x%08.8lx W %d %s\n", node, node->ln_Pri, node->ln_Name);
	    }

	    kprintf("Idle called %d times\n", SysBase->IdleCount);
	}
	/* Help command */
	else if (strcmp(comm, "HE") == 0)
	{
	    kprintf("SAD Help:\n");
	    kprintf("RE AAAAAAAA - reboots AROS - ColdReboot()\n"
		    "RS FFFFFFFF - RESET\n"
		    "FO - Forbid()\n"
		    "PE - Permit()\n"
		    "DI - Disable()\n"
		    "EN - Enable()\n"
		    "DU - Dump most important registers\n"
			"SI - Show IRQ lines status\n"
			"TI - Show Active task info\n"
			"RI - Show registers inside task's context\n"
		    "AM xxxxxxxx yyyyyyyy - AllocVec - size=xxxxxxxx, "
		    "requiments=yyyyyyyy\n"
		    "FM xxxxxxxx - FreeVec from xxxxxxxx\n"
		    "RB xxxxxxxx - read byte from xxxxxxxx\n"
		    "RW xxxxxxxx - read word from xxxxxxxx\n"
		    "RL xxxxxxxx - read long from xxxxxxxx\n"
		    "WB xxxxxxxx bb - write byte bb at xxxxxxxx\n"
		    "WW xxxxxxxx wwww - write word wwww at xxxxxxxx\n"
		    "WL xxxxxxxx llllllll - write long llllllll at xxxxxxxx\n"
		    "RA xxxxxxxx ssssssss - read array(ssssssss bytes long) "
		    "from xxxxxxxx\n"
		    "RC xxxxxxxx ssssssss - read ascii (ssssssss bytes long) "
		    "from xxxxxxxx\n"
		    "QT 00000000 - quit SAD\n"
		    "SL - show all available libraries (libbase : libname)\n"
		    "SR - show all available resources (resbase : resname)\n"
		    "SD - show all available devices (devbase : devname)\n"
		    "ST - show tasks (T - this, R - ready, W - wait)\n"
		    "HE - this help.\n");
	}
	/* AllocMem command */
	else if (strcmp(comm, "AM") == 0)
	{ 
	    ULONG size = GetL(&data[0]);
	    ULONG requim = GetL(&data[8]);
	    
	    kprintf("Allocated at %08.8lx\n", AllocVec(size, requim));
	}
	/* FreeMem command */
	else if (strcmp(comm, "FM") == 0)
	{
	    APTR base = (APTR)GetL(&data[0]);
	    kprintf("Freed at %08.8lx\n", base);
	    FreeVec(base);
	}
	/* ReadByte */
	else if (strcmp(comm, "RB") == 0)
	    kprintf("Byte at %08.8lx:%02.8lx\n", GetL(&data[0]),
						 *(UBYTE*)(GetL(&data[0])));
	/* ReadWord */
	else if (strcmp(comm, "RW") == 0)
	    kprintf("Word at %08.8lx:%04.8lx\n", GetL(&data[0]),
						 *(UWORD*)(GetL(&data[0])));
	/* ReadLong */
	else if (strcmp(comm, "RL") == 0)
	    kprintf("Long at %08.8lx:%08.8lx\n", GetL(&data[0]),
						 *(ULONG*)(GetL(&data[0])));
	/* WriteByte */
	else if (strcmp(comm,"WB") == 0)
	{
	    kprintf("Byte at %08.8lx:%02.8lx\n", GetL(&data[0]),
						 GetB(&data[8]));
	    *(UBYTE*)(GetL(&data[0])) = GetB(&data[8]);
	}
	/* WriteWord */
	else if (strcmp(comm, "WW") == 0)
	{
	    kprintf("Word at %08.8lx:%04.8lx\n", GetL(&data[0]),
						 GetW(&data[8]));
	    *(UWORD*)(GetL(&data[0])) = GetW(&data[8]);
	}
	/* WriteLong */
	else if (strcmp(comm, "WL") == 0)
	{
	    kprintf("Long at %08.8lx:%08.8lx\n", GetL(&data[0]),
						 GetL(&data[8]));
	    *(ULONG*)(GetL(&data[0])) = GetL(&data[8]);
	}
	/* ReadArray */
	else if (strcmp(comm, "RA") == 0)
	{
	    ULONG 	ptr;
	    int 	cnt, t;
	    
	    kprintf("Array from %08.8lx (size=%08.8lx):\n", GetL(&data[0]),
							    GetL(&data[8]));
	    ptr = GetL(&data[0]);
	    cnt = (int)GetL(&data[8]);
	    for(t = 1; t <= cnt; t++)
	    {
		kprintf("%02.2lx ", *(UBYTE*)ptr);
		ptr++;
		if(!(t % 16)) kprintf("\n");
	    }
	    kprintf("\n");
	}
	/* ReadASCII */
	else if (strcmp(comm, "RC") == 0)
	{
	    ULONG 	ptr;
	    int 	cnt, t;
	    
	    kprintf("ASCII from %08.8lx (size=%08.8lx):\n", GetL(&data[0]),
							    GetL(&data[8]));
	    ptr = GetL(&data[0]);
	    cnt = (int)GetL(&data[8]);
	    for(t = 1; t <= cnt; t++)
	    {
		kprintf("%c",*(char*)ptr);
		ptr++;
		if(!(t % 70)) kprintf(" \n");
	    }
	    kprintf(" \n");
	}
	else if (strcmp(comm, "QT") == 0 && strcmp(dat, "00000000") == 0)
	{
	}
	else kprintf("?? Type HE for help\n");
	
    } while(strcmp(comm, "QT") != 0 || strcmp(dat, "00000000") != 0);
    
    kprintf("Quitting SAD...\n");
    
    AROS_LIBFUNC_EXIT
} /* Debug */

/****************************************************************************************/

void UnGetK()
{
}

/****************************************************************************************/

char GetK(struct ExecBase *SysBase)
{
    int i;

    do
    {
        i = RawMayGetChar();
    } while(i == -1);
    
    return (char)i;
}

/****************************************************************************************/

ULONG GetL(char* string)
{
    ULONG 	ret = 0;
    int 	i;
    char 	digit;
    
    for(i = 0; i < 8; i++)
    {
	digit = (*string++) - '0';
	if (digit > 9) digit -= 'A' - '0' - 10;
	ret = (ret << 4) + digit;
    }
    
    return(ret);
}

/****************************************************************************************/

UWORD GetW(char* string)
{
    UWORD 	ret = 0;
    int 	i;
    char 	digit;
    
    for(i = 0; i < 4; i++)
    {
	digit = (*string++) - '0';
	if (digit > 9) digit -= 'A' - '0' - 10;
	ret = (ret << 4) + digit;
    }
    
    return(ret);
}

/****************************************************************************************/

UBYTE GetB(char* string)
{
    UBYTE 	ret = 0;
    int 	i;
    char 	digit;
    
    for(i = 0; i < 2; i++)
    {
	digit = (*string++) - '0';
	if (digit > 9) digit -= 'A' - '0' - 10;
	ret = (ret << 4) + digit;
    }
    
    return(ret);
}
