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
  return URL_OpenA(url, attrs);
}

LIBSTUBVA(URL_Open, ULONG, REG(a0, STRPTR url), ...)
{
  ULONG res;
  VA_LIST args;

  VA_START(args, url);
  res = URL_OpenA(url, VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(URL_OldGetPrefs, struct URL_Prefs *)
{
  return URL_OldGetPrefs();
}

LIBSTUB(URL_OldFreePrefs, void, REG(a0, struct URL_Prefs *up))
{
  URL_OldFreePrefs( up );
}

LIBSTUB(URL_OldSetPrefs, ULONG, REG(a0, struct URL_Prefs *p), REG(d0, ULONG permanent))
{
  return URL_OldSetPrefs(p, permanent);
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
  return dispatch(msg, resPtr);
}

LIBSTUB(URL_GetPrefsA, struct URL_Prefs *, REG(a0, struct TagItem *attrs))
{
  return URL_GetPrefsA(attrs);
}

LIBSTUBVA(URL_GetPrefs, struct URL_Prefs *, ...)
{
  struct URL_Prefs *res;
  VA_LIST args;

  VA_START(args, self);
  res = URL_GetPrefsA(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(URL_FreePrefsA, void, REG(a0, struct URL_Prefs *up), REG(a1, struct TagItem *attrs))
{
  URL_FreePrefsA(up, attrs);
}

LIBSTUBVA(URL_FreePrefs, void, REG(a0, struct URL_Prefs *up), ...)
{
  VA_LIST args;

  VA_START(args, up);
  URL_FreePrefsA(up, VA_ARG(args, struct TagItem *));
  VA_END(args);
}

LIBSTUB(URL_SetPrefsA, ULONG, REG(a0, struct URL_Prefs *p), REG(a1, struct TagItem *attrs))
{
  return URL_SetPrefsA(p, attrs);
}

LIBSTUBVA(URL_SetPrefs, ULONG, REG(a0, struct URL_Prefs *p), ...)
{
  ULONG res;
  VA_LIST args;

  VA_START(args, p);
  res = URL_SetPrefsA(p, VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(URL_LaunchPrefsAppA, ULONG, REG(a0, struct TagItem *attrs))
{
  return URL_LaunchPrefsAppA(attrs);
}

LIBSTUBVA(URL_LaunchPrefsApp, ULONG, ...)
{
  ULONG res;
  VA_LIST args;

  VA_START(args, self);
  res = URL_LaunchPrefsAppA(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}

LIBSTUB(URL_GetAttr, ULONG, REG(d0, ULONG attr), REG(a0, ULONG *storage))
{
  return URL_GetAttr(attr, storage);
}
