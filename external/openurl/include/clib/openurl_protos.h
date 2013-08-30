#ifndef CLIB_OPENURL_PROTOS_H
#define CLIB_OPENURL_PROTOS_H

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

#ifndef LIBRARIES_OPENURL_H
# include <libraries/openurl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if !defined(__AROS__) && !defined(IPTR)
  #define IPTR ULONG
#endif

/* Obsolete, don't use! */
struct URL_Prefs *URL_OldGetPrefs(void);
void URL_OldFreePrefs(struct URL_Prefs *);
ULONG URL_OldSetPrefs(struct URL_Prefs *, BOOL);
struct URL_Prefs *URL_OldGetDefaultPrefs(void);
ULONG URL_OldLaunchPrefsApp(void);

/* Reach URL */
ULONG URL_OpenA(STRPTR, struct TagItem *);

/* Preferences */
struct URL_Prefs *URL_GetPrefsA(struct TagItem *);
void URL_FreePrefsA(struct URL_Prefs *,struct TagItem *);
ULONG URL_SetPrefsA(struct URL_Prefs *,struct TagItem *);

/* Prefs application */
ULONG URL_LaunchPrefsAppA(struct TagItem *);

/* Information */
ULONG URL_GetAttr(ULONG attr,IPTR *storage);

#if defined(_DCC) || defined(__SASC) || defined (__STORM__) || defined(__GNUC__)
ULONG URL_Open(STRPTR, Tag tag1, ...);
struct URL_Prefs *URL_GetPrefs(Tag tag1, ...);
void URL_FreePrefs(struct URL_Prefs *, Tag tag1, ...);
ULONG URL_SetPrefs(struct URL_Prefs *, Tag tag1, ...);
ULONG URL_LaunchPrefsApp(Tag tag1, ...);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* CLIB_OPENURL_PROTOS_H */
