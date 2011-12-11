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
	LONG GetRexxVar(

/*  SYNOPSIS */
	struct RexxMsg *msg,
	CONST_STRPTR varname,
	char **value)

/*  FUNCTION
        Get a the value of the name rexx variable.

    INPUTS
        msg - A rexx message generated from a running rexx script
        varname - The name of the variable to get the value from
        value - a pointer to a string pointer that will be filled with
                a pointer to the value of the variable. This value
                not be changed. On AROS this pointer will also be an
                argstring so you can get the length with LengthArgstring.
        length - the length of the value argument

    RESULT
        0 when succes, otherwise a rexx error value is returned.

    NOTES
        On AROS the pointer returned in value is only valid until the next
        getrexxvar call on the same running script.

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
    msg2->rm_Action = RXGETVAR | 1;
    msg2->rm_Args[0] = (IPTR)CreateArgstring(varname, strlen(varname));
    if (msg2->rm_Args[0]==0) goto cleanup;
    
    PutMsg(rexxport, (struct Message *)msg2);
    msg3 = NULL;
    while (msg3!=msg2)
    {
	WaitPort(port);
	msg3 = (struct RexxMsg *)GetMsg(port);
	if (msg3!=msg2) ReplyMsg((struct Message *)msg3);
    }
    
    if (msg3->rm_Result1==RC_OK)
    {
	*value = (char *)msg3->rm_Result2;
	retval = RC_OK;
    }
    else retval = (LONG)msg3->rm_Result2;

cleanup:
    if (msg2!=NULL)
    {
	if (msg2->rm_Args[0]!=0) DeleteArgstring((UBYTE *)msg2->rm_Args[0]);
	DeleteRexxMsg(msg2);
    }
    if (port!=NULL) DeletePort(port);
    if (RexxSysBase!=NULL) CloseLibrary(RexxSysBase);

    return retval;
} /* SetRexxVar */
