/*
** wbstartup.c
**
** (c) 1998-2011 Guido Mersmann
**
** This file contains WBStartup handling. If you need wbmessage use
** wbmessage field to check if there was a message at all.
**
*/

/*************************************************************************/

#define SOURCENAME "wbstartup.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/exec.h>

#include <dos/dosextens.h>
#include <exec/ports.h>
#include <workbench/startup.h>

/*************************************************************************/

struct WBStartup *wbmessage;

/*************************************************************************/

/* /// WBMessage_Get()
**
*/

/*************************************************************************/

void WBMessage_Get(void)
{
struct Process *process;

    process = (struct Process *) FindTask(NULL);

    if( !(process->pr_CLI)) {
        WaitPort(&(process->pr_MsgPort));

        wbmessage = (struct WBStartup *) GetMsg(&(process->pr_MsgPort));

    }
}
/* \\\ */
/* /// WBMessage_Reply()
**
*/

/*************************************************************************/

void WBMessage_Reply(void)
{
    if( wbmessage) {

        Forbid();
        ReplyMsg( (struct Message *) wbmessage);
    }
}
/* \\\ */
