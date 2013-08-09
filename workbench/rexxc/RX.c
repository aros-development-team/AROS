/*
    Copyright © 2007-2013, The AROS Development Team. All rights reserved.
    $Id$

    Run REXX scripts
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <proto/alib.h>

#include <string.h>
#include <ctype.h>

#ifndef __AROS__
#define IPTR void *
#endif

static struct RexxMsg *msg = NULL;
static struct MsgPort *rexxport = NULL, *replyport = NULL;
static BPTR out;
static BOOL closestdout = FALSE;
static BPTR olddir = (BPTR)-1;

static BOOL init(void)
{
#ifdef __AROS__
    out = ErrorOutput();
    if (out == BNULL)
#endif
        out = Output();
    
    rexxport = FindPort("REXX");
    if (rexxport == NULL)
    {
        if (SystemTags("RexxMast", SYS_Asynch, TRUE,
                SYS_Input, BNULL, SYS_Output, BNULL,
                TAG_DONE
            ) >= 0
           )
        {
            SystemTags("WaitForPort REXX", TAG_DONE);
        }
    }
    rexxport = FindPort("REXX");
    if (rexxport == NULL)
    {
	FPuts(out, "Could not start RexxMast; no Rexx interpreter seems to be installed\n");
	return FALSE;
    }
    
    replyport = CreatePort(NULL, 0);
    if (replyport == NULL)
    {
	FPuts(out, "Could not create a port\n");
	return FALSE;
    }
    
    msg = CreateRexxMsg(replyport, NULL, NULL);
    if (msg == NULL)
    {
	FPuts(out, "Could not create RexxMsg\n");
	return FALSE;
    }
    msg->rm_Action = RXCOMM | RXFF_RESULT;
    msg->rm_Stdin = Input();
    Flush(msg->rm_Stdin); /* Remove command line arguments */
    msg->rm_Stdout = Output();
    
    return TRUE;
}

void cleanup(void)
{
    if (closestdout)
        Close(msg->rm_Stdout);
    if (msg)
	DeleteRexxMsg(msg);
    if (replyport)
	DeletePort(replyport);
    if (olddir != (BPTR)-1)
	CurrentDir(olddir);
}

int main(int argc, char **argv)
{
    struct RexxMsg *reply;
    int ret;
    
    if (!init())
    {
	cleanup();
	return RC_ERROR;
    }
    
    if (argc == 1)
    {
	FPuts(out, "Usage: RX <filename> [arguments]\n"
	           "       RX \"commands\"\n");
	cleanup();
	return RC_ERROR;
    }

    if (argc == 0)
    {
	struct WBStartup *startup = (struct WBStartup *) argv;
        char *s = startup->sm_ArgList[1].wa_Name;
        
        if (startup->sm_NumArgs < 2)
        {
            cleanup();
            return RC_ERROR;
        }

	olddir = CurrentDir(startup->sm_ArgList[1].wa_Lock);
	out = msg->rm_Stdout = Open("CON:////RX Output/CLOSE/WAIT/AUTO", MODE_READWRITE);
        closestdout = TRUE;
        
        msg->rm_Args[0] = (IPTR)CreateArgstring(s, strlen(s));
        msg->rm_Action |= 1;
    }
    else
    {
	UBYTE *s;
	struct Process *me = (struct Process *)FindTask(NULL);
	ULONG length = 0;
	
	s = me->pr_Arguments;
	while(isspace(*s)) s++;
	
	if (*s == '"')
	{
	    s++;
	    while((s[length] != '"') && (s[length] != '\0')) length++;
	    if (length == 0)
	    {
		FPuts(out, "Empty command\n");
		cleanup();
		return RC_ERROR;
	    }
	    /* Lazy string termination like ARexx */
#if 0
	    if (s[length] == '\0')
	    {
		FPuts(out, "Unterminated string\n");
		cleanup();
		return RC_ERROR;
	    }
#endif
	    
	    msg->rm_Args[0] = (IPTR)CreateArgstring(s, length);
	    /* It is a literal command with 1 argument */
	    msg->rm_Action |= (RXFF_STRING | 1);
	}
	else if (*s == '\'')
	{
	    s++;
	    while((s[length] != '\'')
                  && (s[length] != '\0')
                  && (s[length] != '\n')
            )
                length++;
	    
	    msg->rm_Args[0] = (IPTR)CreateArgstring(s, length);
	    /* It is a literal command with 1 argument */
	    msg->rm_Action |= (RXFF_STRING | 1);
	}
	else
	{
            if (s[strlen(s)-1] == '\n')
                s[strlen(s)-1] = '\0';
            
            msg->rm_Args[0] = (IPTR)CreateArgstring(s, strlen(s));
	    msg->rm_Action |= 1;
	}
    }


    PutMsg(rexxport, (struct Message *)msg);
    do {
        reply = (struct RexxMsg *)WaitPort(replyport);
    } while (reply != msg);

    ret = msg->rm_Result1;
    if (msg->rm_Result1 == RC_OK)
	/* Less verbosity like ARexx, use "get Result2" */
#if 0
        FPrintf(out, "Script executed and returned: %ld\n", msg->rm_Result2);
#else
        ;
#endif
    else
        FPrintf(out, "Error executing script %ld/%ld\n",
                msg->rm_Result1, msg->rm_Result2
        );

    ClearRexxMsg(msg, msg->rm_Action & RXARGMASK);
    cleanup();
    
    return ret;
}
