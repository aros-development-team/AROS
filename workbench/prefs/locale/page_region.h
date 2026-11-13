#ifndef _LCOUNTRY_H_
#define _LCOUNTRY_H_

/*
   Copyright © 2008-2013, The AROS Development Team. All rights reserved.
   $Id$
 */

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_Region                (TAG_USER | 0x40000000)

/*** Attributes *************************************************************/
#define MUIA_Region_Regionname    (MUIB_Region | 0)

/*** Methods ****************************************************************/
#define MUIM_Region_Fill           (MUIB_Region | 0)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Region_CLASS;

/*** Macros *****************************************************************/
#define RegionObject BOOPSIOBJMACRO_START(Region_CLASS->mcc_Class)

#endif /* _LCOUNTRY_H_ */
