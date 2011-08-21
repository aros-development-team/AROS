/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef  DOS_NEWCLIPROC_H
#define  DOS_NEWCLIPROC_H

#include <exec/execbase.h>
#include <aros/asmcall.h>

struct CliStartupMessage
{
    struct Message csm_Msg;
    BOOL           csm_Background;
    BOOL           csm_Asynch;
    BPTR           csm_CurrentInput;
    LONG           csm_ReturnCode;
    LONG           csm_CliNumber;
};

AROS_UFP2(LONG, NewCliProc,
AROS_UFPA(char *,argstr,A0),
AROS_UFPA(ULONG,argsize,D0));

#endif /* DOS_NEWCLIPROC_H */
