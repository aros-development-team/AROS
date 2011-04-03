
/*
** process.c
**
** (c) 1998-2011 Guido Mersmann
**
** This file contains all code needed to prevent "insert volume fuhbar:"
** requesters.
*/

/*************************************************************************/

#define SOURCENAME "process.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/exec.h>

#include <dos/dosextens.h>
#include <intuition/intuition.h>

/*************************************************************************/

struct Process *process_process;
struct Window  *process_windowptr;

/*************************************************************************/

/* /// Process_SetWindowPtr()
**
*/

/*************************************************************************/

void Process_SetWindowPtr ( struct Window *window )
{

    process_process = (struct Process *) FindTask( NULL );

    if( process_process->pr_Task.tc_Node.ln_Type == NT_PROCESS) {

        process_windowptr = process_process->pr_WindowPtr;
        process_process->pr_WindowPtr = window;
    } else {
        process_process = NULL;
    }

}
/* \\\ */
/* /// Process_ClearWindowPtr()
**
*/

/*************************************************************************/

void Process_ClearWindowPtr ( void )
{

    if( process_process ) {
        process_process->pr_WindowPtr = process_windowptr;
        process_process = NULL;
    }
}
/* \\\ */

