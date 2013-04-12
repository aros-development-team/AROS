/*
    Copyright © 2008-2103, The AROS Development Team. All rights reserved.
    $Id$

    Desc: makecountry code that needs access to AROS structures and types
    Lang: english
*/

#include <exec/types.h>
#include "country_locale.h"

#define EC(x)\
{\
    (x) =   (((x) & 0xFF000000) >> 24)\
	  | (((x) & 0x00FF0000) >> 8)\
	  | (((x) & 0x0000FF00) << 8)\
	  | (((x) & 0x000000FF) << 24);\
}

unsigned long getCountryPrefsSize()
{
    return sizeof(struct CountryPrefs);
}

unsigned long getCountryPrefsVers(struct IntCountryPrefs *cp)
{
    return cp->country_Version;
}

unsigned long getCountryPrefsFlag(struct IntCountryPrefs *cp)
{
    return cp->country_Flag;
}

void convertEndianness(struct CountryPrefs *cp)
{
#if (AROS_BIG_ENDIAN == 0)
    /* We have to convert the endianness of this data,
       thankfully there are only two fields which this applies
       to.
    */
    EC(cp->cp_CountryCode);
    EC(cp->cp_TelephoneCode);
#endif
}
