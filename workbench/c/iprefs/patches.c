/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <exec/execbase.h>
#include <aros/machine.h>
#include <aros/libcall.h>

/*********************************************************************************************/

void Install_RawDoFmtPatch(void)
{
    SetFunction(&SysBase->LibNode,
    	    	-87 * LIB_VECTSIZE,
		__AROS_GETVECADDR(LocaleBase, 31));
}

/*********************************************************************************************/
