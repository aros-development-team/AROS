/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
    Lang: english
*/

/*****************************************************************************************/

#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif
#ifndef DOS_EXALL_H
#   include <dos/exall.h>
#endif
#ifndef DOS_DOSEXTENS_H
#   include <dos/dosextens.h>
#endif

#ifndef LAYOUT_H
#   include "layout.h"
#endif

/*****************************************************************************************/

/* Options */

#define SPECIALREQ_COOL_BUTTONS 	1

/*****************************************************************************************/

STRPTR REQ_String(STRPTR title, STRPTR stringtext, STRPTR oktext, STRPTR canceltext,
		  struct LayoutData *ld, struct AslBase_intern *AslBase);

/*****************************************************************************************/

