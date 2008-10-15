#ifndef _LCOUNTRY_H_
#define _LCOUNTRY_H_

/*
    Copyright  2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_Country                  (TAG_USER | 0x10000000)

enum
{
  COUNTRY_FILL=MUIB_Country
};

/*** Variables **************************************************************/
extern struct MUI_CustomClass *Country_CLASS;

/*** Macros *****************************************************************/
#define CountryObject BOOPSIOBJMACRO_START(Country_CLASS->mcc_Class)

void InitCountry();
void CleanCountry();

#endif /* _LCOUNTRY_H_ */
