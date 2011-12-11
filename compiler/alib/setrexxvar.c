/*
        Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <string.h>

/*****************************************************************************

    NAME */
	LONG SetRexxVar(

/*  SYNOPSIS */
	struct RexxMsg *msg,
	CONST_STRPTR varname,
	char *value,
        ULONG length)

/*  FUNCTION
        Set a the value of the name rexx variable.

    INPUTS
        msg - A rexx message generated from a running rexx script
        varname - The name of the variable to set the value
        value - a pointer to the beginning of the value to set
        length - the length of the value argument

    RESULT
        0 when succes, otherwise a rexx error value is returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CheckRexxMsg(), GetRexxVar()

    INTERNALS
        This function creates a rexx message that is sent to the AREXX
        port with a RXSETVAR command.


*****************************************************************************/
{
    struct Library *RexxSysBase = NULL;
    struct RexxMsg *msg2 = NULL, *msg3;
    struct MsgPort *port = NULL, *rexxport;
    LONG retval = ERR10_003;
    
    RexxSysBase = OpenLibrary("rexxsyslib.library", 0);
    if (RexxSysBase==NULL) goto cleanup;
    
    if (!IsRexxMsg(msg))
    {
	retval = ERR10_010;
	goto cleanup;
    }
    
    rexxport = FindPort("REXX");
    if (rexxport==NULL)
    {
	retval = ERR10_013;
	goto cleanup;
    }
  
    port = CreateMsgPort();
    if (port == NULL) goto cleanup;
    msg2 = CreateRexxMsg(port, NULL, NULL);
    if (msg2==NULL) goto cleanup;
    msg2->rm_Private1 = msg->rm_Private1;
    msg2->rm_Private2 = msg->rm_Private2;
    msg2->rm_Action = RXSETVAR | 2;
    msg2->rm_Args[0] = (IPTR)CreateArgstring(varname, strlen(varname));
    msg2->rm_Args[1] = (IPTR)CreateArgstring(value, length);
    if (msg2->rm_Args[0]==0 || msg2->rm_Args[1]==0) goto cleanup;
    
    PutMsg(rexxport, (struct Message *)msg2);
    msg3 = NULL;
    while (msg3!=msg2)
    {
	WaitPort(port);
	msg3 = (struct RexxMsg *)GetMsg(port);
	if (msg3!=msg2) ReplyMsg((struct Message *)msg3);
    }

    if (msg3->rm_Result1==RC_OK) retval = 0;
    else retval = (LONG)msg3->rm_Result2;

cleanup:
    if (msg2!=NULL)
    {
	if (msg2->rm_Args[0]!=0) DeleteArgstring((UBYTE *)msg2->rm_Args[0]);
	if (msg2->rm_Args[1]!=0) DeleteArgstring((UBYTE *)msg2->rm_Args[1]);
	DeleteRexxMsg(msg2);
    }
    if (port!=NULL) DeletePort(port);
    if (RexxSysBase!=NULL) CloseLibrary(RexxSysBase);

    return retval;
} /* SetRexxVar */
