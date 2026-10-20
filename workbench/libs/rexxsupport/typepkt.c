/*
    Copyright (C) 1995-2006, The AROS Development Team. All rights reserved.

    Desc: Return the type of a RexxMsg
*/

#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <exec/types.h>
#include <rexx/storage.h>
#include <rexx/errors.h>

#include <ctype.h>
#include <stdlib.h>

#include "rexxsupport_intern.h"
#include "rxfunctions.h"

/*****************************************************************************

    NAME
        TYPEPKT

    SYNOPSIS
    result = TYPEPKT(message[, selector])

    FUNCTION
    Reads the action code from a Rexx message obtained through a
    binary message pointer.

    INPUTS
    message  - Pointer-sized binary argument string naming a RexxMsg.
    selector - Optional selector: A for argument count, C to test RXCOMM,
               or F to test RXFUNC. The first character is used.

    RESULT
    Without selector, a binary LONG containing rm_Action. With A,
    a single ASCII digit for the argument count. With C or F,
    the string "1" when the action matches or "0" otherwise.

    BUGS
    No message-pointer validation is performed. Unrecognized selectors
    leave the single-byte result uninitialized.

    SEE ALSO
    GETPKT, GETARG, REPLY

*****************************************************************************/

LONG rxsupp_typepkt(struct Library *RexxSupportBase, struct RexxMsg *msg, UBYTE **argstring)
{
    struct RexxMsg *msg2 = *(struct RexxMsg **)ARG1(msg);
    
    if ((msg->rm_Action & RXARGMASK) == 1)
    {
        *argstring = CreateArgstring((UBYTE *)&msg2->rm_Action, sizeof(msg2->rm_Action));
    }
    else /* 2 Args */
    {
        char c;
        
        switch (toupper(*(char *)ARG2(msg)))
        {
        case 'A':
            c = '0' + (char)(msg2->rm_Action & RXARGMASK);
            break;
            
        case 'C':
            c = ((msg2->rm_Action & RXCODEMASK) == RXCOMM) ? '1' : '0';
            break;
            
        case 'F':
            c = ((msg2->rm_Action & RXCODEMASK) == RXFUNC) ? '1' : '0';
            break;
        }
        
        *argstring = CreateArgstring(&c, 1);
    }
    
    return RC_OK;
}
