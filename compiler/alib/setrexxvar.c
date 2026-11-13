/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include "alib_intern.h"

/*****************************************************************************

    NAME */
        LONG SetRexxVar(

/*  SYNOPSIS */
        struct RexxMsg *msg,
        CONST_STRPTR varname,
        char *value,
        ULONG length)

/*  FUNCTION
        Set the value of the named REXX variable.

    INPUTS
        msg - A REXX message generated from a running REXX script
        varname - The name of the variable to set the value
        value - a pointer to the beginning of the value to set
        length - the length of the value argument

    RESULT
        0 when success, otherwise a REXX error value is returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CheckRexxMsg(), GetRexxVar()

    INTERNALS
        This function creates a REXX message that is sent to the AREXX
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
    msg2->rm_Args[0] = (IPTR)CreateArgstring(varname, STRLEN(varname));
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
