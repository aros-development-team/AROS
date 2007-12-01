#ifndef PRAGMAS_OPENURL_PRAGMAS_H
#define PRAGMAS_OPENURL_PRAGMAS_H

/*
**  $VER: openurl_pragmas.h 7.1 (1.12.2005)
**  Includes Release 7.2
**
**  Direct ROM interface (pragma) definitions.
**
**  openurl.library - universal URL display and browser
**  launcher library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
*/

#ifndef CLIB_OPENURL_PROTOS_H
#include <clib/openurl_protos.h>
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(OpenURLBase,0x01e,URL_OpenA(a0,a1))
#pragma amicall(OpenURLBase,0x024,URL_OldGetPrefs())
#pragma amicall(OpenURLBase,0x02a,URL_OldFreePrefs(a0))
#pragma amicall(OpenURLBase,0x030,URL_OldSetPrefs(a0,d0))
#pragma amicall(OpenURLBase,0x036,URL_OldGetDefaultPrefs())
#pragma amicall(OpenURLBase,0x03c,URL_OldLaunchPrefsApp())
#pragma amicall(OpenURLBase,0x042,DoFunction(a0))
#pragma amicall(OpenURLBase,0x048,URL_GetPrefsA(a0))
#pragma amicall(OpenURLBase,0x04e,URL_FreePrefsA(a0,a1))
#pragma amicall(OpenURLBase,0x054,URL_SetPrefsA(a0,a1))
#pragma amicall(OpenURLBase,0x05a,URL_LaunchPrefsAppA(a0))
#pragma amicall(OpenURLBase,0x060,URL_GetAttr(d0,a0))
#endif
#if defined(_DCC) || defined(__SASC)
#pragma  libcall OpenURLBase URL_OpenA              01e 9802
#pragma  libcall OpenURLBase URL_OldGetPrefs        024 00
#pragma  libcall OpenURLBase URL_OldFreePrefs       02a 801
#pragma  libcall OpenURLBase URL_OldSetPrefs        030 0802
#pragma  libcall OpenURLBase URL_OldGetDefaultPrefs 036 00
#pragma  libcall OpenURLBase URL_OldLaunchPrefsApp  03c 00
#pragma  libcall OpenURLBase DoFunction             042 801
#pragma  libcall OpenURLBase URL_GetPrefsA          048 801
#pragma  libcall OpenURLBase URL_FreePrefsA         04e 9802
#pragma  libcall OpenURLBase URL_SetPrefsA          054 9802
#pragma  libcall OpenURLBase URL_LaunchPrefsAppA    05a 801
#pragma  libcall OpenURLBase URL_GetAttr            060 8002
#endif
#ifdef __STORM__
#pragma tagcall(OpenURLBase,0x01e,URL_Open(a0,a1))
#pragma tagcall(OpenURLBase,0x048,URL_GetPrefs(a0))
#pragma tagcall(OpenURLBase,0x04e,URL_FreePrefs(a0,a1))
#pragma tagcall(OpenURLBase,0x054,URL_SetPrefs(a0,a1))
#pragma tagcall(OpenURLBase,0x05a,URL_LaunchPrefsApp(a0))
#endif
#ifdef __SASC_60
#pragma  tagcall OpenURLBase URL_Open               01e 9802
#pragma  tagcall OpenURLBase URL_GetPrefs           048 801
#pragma  tagcall OpenURLBase URL_FreePrefs          04e 9802
#pragma  tagcall OpenURLBase URL_SetPrefs           054 9802
#pragma  tagcall OpenURLBase URL_LaunchPrefsApp     05a 801
#endif

#endif  /* PRAGMAS_OPENURL_PRAGMAS_H */
