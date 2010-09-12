#ifndef _LCOUNTRY_H_
#define _LCOUNTRY_H_

/*
   Copyright © 2008-2010, The AROS Development Team. All rights reserved.
   $Id$
 */

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_Country                (TAG_USER | 0x40000000)

/*** Attributes *************************************************************/
#define MUIA_Country_Countryname    (MUIB_Country | 0)

/*** Methods ****************************************************************/
#define MUIM_Country_Fill           (MUIB_Country | 0)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Country_CLASS;

/*** Macros *****************************************************************/
#define CountryObject BOOPSIOBJMACRO_START(Country_CLASS->mcc_Class)

#endif /* _LCOUNTRY_H_ */
