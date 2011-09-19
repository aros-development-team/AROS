/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef  DOS_NEWCLIPROC_H
#define  DOS_NEWCLIPROC_H

#include <dos/cliinit.h>

/* Shared code between CliInitRun() and CliInitNewcli()
 */
ULONG internal_CliInitAny(struct DosPacket *dp, APTR DOSBase);

#endif /* DOS_NEWCLIPROC_H */
