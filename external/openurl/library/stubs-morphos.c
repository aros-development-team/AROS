/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include "lib.h"

#include "SDI_lib.h"
#include "SDI_stdarg.h"

// only the return type must be defined for the MorphOS stub functions
// all parameters are taken from the emulated 68k registers

LIBSTUB(URL_OpenA, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(URL_OpenA, (STRPTR)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(URL_OldGetPrefs, struct URL_Prefs *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC_NP(URL_OldGetPrefs);
}

LIBSTUB(URL_OldFreePrefs, void)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  CALL_LFUNC(URL_OldFreePrefs, (struct URL_Prefs *)REG_A0);
}

LIBSTUB(URL_OldSetPrefs, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(URL_OldSetPrefs, (struct URL_Prefs *)REG_A0, (ULONG)REG_D0);
}

LIBSTUB(URL_OldGetDefaultPrefs, struct URL_Prefs *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC_NP(URL_OldGetDefaultPrefs);
}

LIBSTUB(URL_OldLaunchPrefsApp, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC_NP(URL_OldLaunchPrefsApp);
}

LIBSTUB(dispatch, LONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(dispatch, (struct RexxMsg *)REG_A0, (STRPTR *)REG_A1);
}

LIBSTUB(URL_GetPrefsA, struct URL_Prefs *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(URL_GetPrefsA, (struct TagItem *)REG_A0);
}

LIBSTUB(URL_FreePrefsA, void)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  CALL_LFUNC(URL_FreePrefsA, (struct URL_Prefs *)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(URL_SetPrefsA, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(URL_SetPrefsA, (struct URL_Prefs *)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(URL_LaunchPrefsAppA, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(URL_LaunchPrefsAppA, (struct TagItem *)REG_A0);
}

LIBSTUB(URL_GetAttr, ULONG)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(URL_GetAttr, (ULONG)REG_D0, (ULONG *)REG_A0);
}
