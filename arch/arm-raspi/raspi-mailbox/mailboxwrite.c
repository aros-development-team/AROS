/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/
#define DEBUG 1

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/mailbox.h>

#include "mailbox_private.h"
#include LC_LIBDEFS_FILE

VOID _MailboxWrite(void) {
}

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, MailboxWrite,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, mailbox, D0),

/*  LOCATION */
        struct mailbox_base *, MailboxBase, 7, Mailbox)
{
    AROS_LIBFUNC_INIT

    ObtainMailbox(mailbox);
    _MailboxWrite();
    ReleaseMailbox(mailbox);
    return 0;

    AROS_LIBFUNC_EXIT
}
