#ifndef COUNTRY_LOCALE_H
#define COUNTRY_LOCALE_H
/*
    Copyright © 2103, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <prefs/locale.h>

struct IntCountryPrefs {
    struct CountryPrefs country_CP;
    char        *country_Version;
    char        *country_NativeNames;
    char        *country_Flag;
};

#endif /* COUNTRY_LOCALE_H */
