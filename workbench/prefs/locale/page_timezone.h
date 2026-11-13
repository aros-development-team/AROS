#ifndef _TIMEZONE_H_
#define _TIMEZONE_H_

/*
   Copyright © 2004-2010, The AROS Development Team. All rights reserved.
   $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_Timezone               (TAG_USER | 0x20000000)

/*** Attributes *************************************************************/
#define MUIA_Timezone_Timeoffset    (MUIB_Timezone | 0)
#define MUIA_Timezone_GMTClock      (MUIB_Timezone | 1)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Timezone_CLASS;

/*** Macros *****************************************************************/
#define TimezoneObject BOOPSIOBJMACRO_START(Timezone_CLASS->mcc_Class)

#endif
