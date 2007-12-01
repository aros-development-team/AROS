/*
**  openurl.library - universal URL display and browser
**  launcher library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
*/


#include "lib.h"

/**************************************************************************/

ULONG LIB_URL_OpenA(void)
{
    return URL_OpenA((UBYTE *)REG_A0,(struct TagItem *)REG_A1);
}

/**************************************************************************/

struct URL_Prefs *LIB_URL_GetPrefsA(void)
{
    return URL_GetPrefsA((struct TagItem *)REG_A0);
}

/**************************************************************************/

struct URL_Prefs * LIB_URL_OldGetPrefs(void)
{
    return URL_OldGetPrefs();
}

/**************************************************************************/

void LIB_URL_FreePrefsA(void)
{
    return URL_FreePrefsA((struct URL_Prefs *)REG_A0,(struct TagItem *)REG_A1);
}

/**************************************************************************/

void LIB_URL_OldFreePrefs(void)
{
    return URL_OldFreePrefs((struct URL_Prefs *)REG_A0);
}

/**************************************************************************/

ULONG LIB_URL_SetPrefsA(void)
{
    return URL_SetPrefsA((struct URL_Prefs *)REG_A0,(struct TagItem *)REG_A1);
}

/**************************************************************************/

ULONG LIB_URL_OldSetPrefs(void)
{
    return URL_OldSetPrefs((struct URL_Prefs *)REG_A0,(ULONG)REG_D0);
}

/**************************************************************************/

struct URL_Prefs * LIB_URL_OldGetDefaultPrefs(void)
{
    return URL_OldGetDefaultPrefs();
}

/**************************************************************************/

ULONG LIB_URL_LaunchPrefsAppA(void)
{
    return URL_LaunchPrefsAppA((struct TagItem *)REG_A0);
}

/**************************************************************************/

ULONG LIB_URL_OldLaunchPrefsApp(void)
{
    return URL_OldLaunchPrefsApp();
}

/**************************************************************************/

ULONG LIB_URL_GetAttr(void)
{
    return URL_GetAttr((ULONG)REG_D0,(LONG *)REG_A0);
}

/**************************************************************************/

