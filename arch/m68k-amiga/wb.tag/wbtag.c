/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: wbtag.c

    Desc: WB1.x C:LoadWB ROM Workbench startup code
    Lang: english
*/
#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/ports.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <workbench/workbench.h>
#include <proto/workbench.h>

#define _STR(A) #A
#define STR(A) _STR(A)

#define NAME "workbench.task"
#define VERSION 40
#define REVISION 1
const TEXT WBTAG_NAME[] = NAME;
const TEXT WBTAG_VERSION_STRING[] = NAME " " STR(VERSION) "." STR(REVISION) " (" ADATE ")\n";

extern void Init(void);
void InitX(void)
{
	/* ROMTAG here because jump table must come immediately after it */
asm (
	"	.globl WBTAG_NAME\n"
	"	.globl WBTAG_VERSION_STRING\n"
	"	.globl RealInit\n"
	"	.globl wbtag_end\n"
	"	.text\n"
	"	.align 4\n"
	"	.word 0\n"
	/* ROMTAG */
	"wbtag:	.word 0x4afc\n"
	"	.long wbtag\n"
	"	.long wbtag_end\n"
	"	.byte 0\n"
	"	.byte 40\n"
	"	.byte 1\n"
	"	.byte -123\n"
	"	.long WBTAG_NAME\n"
	"	.long WBTAG_VERSION_STRING\n"
	"	.long Init\n"
	/* Strange jump table. Do not touch! \n */
	/* MUST come immediately after ROMTAG! */
	"Init:	.long 0\n"
		/* Old entry points */
	"	moveq #0,%d0\n"
	"	bra.s Init2\n"
	"	.long 0\n"
	"	moveq #1,%d0\n"
	"	bra.s Init2\n"
	"	.long 0\n"
		/* KS 1.2/1.3 entry point */
	"	moveq #2,%d0\n"
	"Init2:	move.l %d0,%sp@-\n"
	"	jsr RealInit\n"
	"	addq.l #4,%sp\n"
	"	rts\n"
);
}

ULONG RealInit(ULONG mode)
{
    struct Process *pr = (struct Process*)FindTask(0);
    struct Message *msg = NULL;
    struct Library *WorkbenchBase; 
    ULONG rc = 0;
    
    D(bug("wb.task %d\n", mode));
    if (mode == 2) {
	WaitPort(&pr->pr_MsgPort);
	msg = GetMsg(&pr->pr_MsgPort);
    }
    WorkbenchBase = TaggedOpenLibrary(TAGGEDOPEN_WORKBENCH);
    if (WorkbenchBase) {
    	rc = StartWorkbench(0, 0);
    	CloseLibrary(WorkbenchBase);
    }
    if (msg)
    	ReplyMsg(msg);
    D(bug("wb return code = %d\n", rc));
    return rc;
    
}
