/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

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

LIBSTUB(URL_OpenA, ULONG, REG(a0, STRPTR url), REG(a1, struct TagItem *attrs))
{
  return URL_OpenA((STRPTR)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(URL_OldGetPrefs, struct URL_Prefs *)
{
  return URL_OldGetPrefs();
}

LIBSTUB(URL_OldFreePrefs, void, REG(a0, struct URL_Prefs *up))
{
  URL_OldFreePrefs((struct URL_Prefs *)REG_A0);
}

LIBSTUB(URL_OldSetPrefs, ULONG, REG(a0, struct URL_Prefs *p), REG(d0, ULONG permanent))
{
  return URL_OldSetPrefs((struct URL_Prefs *)REG_A0, (ULONG)REG_D0);
}

LIBSTUB(URL_OldGetDefaultPrefs, struct URL_Prefs *)
{
  return URL_OldGetDefaultPrefs();
}

LIBSTUB(URL_OldLaunchPrefsApp, ULONG)
{
  return URL_OldLaunchPrefsApp();
}

LIBSTUB(dispatch, LONG, REG(a0, struct RexxMsg *msg), REG(a1, STRPTR *resPtr))
{
  return dispatch((struct RexxMsg *)REG_A0, (STRPTR *)REG_A1);
}

LIBSTUB(URL_GetPrefsA, struct URL_Prefs *, REG(a0, struct TagItem *attrs))
{
  return URL_GetPrefsA((struct TagItem *)REG_A0);
}

LIBSTUB(URL_FreePrefsA, void, REG(a0, struct URL_Prefs *up), REG(a1, struct TagItem *attrs))
{
  URL_FreePrefsA((struct URL_Prefs *)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(URL_SetPrefsA, ULONG, REG(a0, struct URL_Prefs *p), REG(a1, struct TagItem *attrs))
{
  return URL_SetPrefsA((struct URL_Prefs *)REG_A0, (struct TagItem *)REG_A1);
}

LIBSTUB(URL_LaunchPrefsAppA, ULONG, REG(a0, struct TagItem *attrs))
{
  return URL_LaunchPrefsAppA((struct TagItem *)REG_A0);
}

LIBSTUB(URL_GetAttr, ULONG, REG(d0, ULONG attr), REG(a0, ULONG *storage))
{
  return URL_GetAttr((ULONG)REG_D0, (ULONG *)REG_A0);
}
