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

void Install_StrnicmpPatch(void)
{
    SetFunction(&UtilityBase->ub_LibNode,
    	    	-28 * LIB_VECTSIZE,
		__AROS_GETVECADDR(LocaleBase, 32));
}

/*********************************************************************************************/

void Install_StricmpPatch(void)
{
    SetFunction(&UtilityBase->ub_LibNode,
    	    	-27 * LIB_VECTSIZE,
		__AROS_GETVECADDR(LocaleBase, 33));
}

/*********************************************************************************************/

void Install_ToLowerPatch(void)
{
    SetFunction(&UtilityBase->ub_LibNode,
    	    	-30 * LIB_VECTSIZE,
		__AROS_GETVECADDR(LocaleBase, 34));
}

/*********************************************************************************************/

void Install_ToUpperPatch(void)
{
    SetFunction(&UtilityBase->ub_LibNode,
    	    	-29 * LIB_VECTSIZE,
		__AROS_GETVECADDR(LocaleBase, 35));
}

/*********************************************************************************************/
