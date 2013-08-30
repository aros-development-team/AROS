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

#include "SDI_lib.h"

/* init.c */
ULONG freeBase(struct LibraryHeader* lib);
ULONG initBase(struct LibraryHeader* lib);

/* utils.c */
#if defined(__amigaos4__) || defined(__MORPHOS__)
  #define HAVE_ALLOCVECPOOLED 1
  #define HAVE_FREEVECPOOLED  1
#endif

#if defined(HAVE_ALLOCVECPOOLED)
#define allocVecPooled(pool,size) AllocVecPooled(pool,size)
#else
APTR allocVecPooled(APTR pool, ULONG size);
#endif
#if defined(HAVE_FREEVECPOOLED)
#define freeVecPooled(pool,mem)   FreeVecPooled(pool,mem)
#else
void freeVecPooled(APTR pool, APTR mem);
#endif
APTR reallocVecPooled(APTR pool, APTR mem, ULONG oldSize, ULONG newSize);
APTR allocArbitrateVecPooled(ULONG size);
void freeArbitrateVecPooled(APTR mem);
BOOL sendToBrowser(STRPTR URL, struct List *portlist, ULONG flags, STRPTR pubScreenName);
BOOL sendToFTP(STRPTR URL, struct List *portlist, ULONG flags, STRPTR pubScreenName);
BOOL sendToMailer(STRPTR URL, struct List *portlist, ULONG flags, STRPTR pubScreenName);
BOOL copyList(struct List *dst, struct List *src, ULONG size);
void freeList(struct List *list);
BOOL isdigits(STRPTR str);

#define SENDTOB_SHOW                   0
#define SENDTOF_SHOW                   (1<<SENDTOB_SHOW)
#define SENDTOB_TOFRONT                1
#define SENDTOF_TOFRONT                (1<<SENDTOB_TOFRONT)
#define SENDTOB_NEWWINDOW              2
#define SENDTOF_NEWWINDOW              (1<<SENDTOB_NEWWINDOW)
#define SENDTOB_LAUNCH                 3
#define SENDTOF_LAUNCH                 (1<<SENDTOB_LAUNCH)

/* api.c */
LIBPROTO(URL_OpenA, ULONG, REG(a0, STRPTR url), REG(a1, struct TagItem *attrs));
LIBPROTOVA(URL_Open, ULONG, REG(a0, STRPTR url), ...);
LIBPROTO(URL_OldGetPrefs, struct URL_Prefs *);
LIBPROTO(URL_OldFreePrefs, void, REG(a0, struct URL_Prefs *up));
LIBPROTO(URL_OldSetPrefs, ULONG, REG(a0, struct URL_Prefs *p), REG(d0, ULONG permanent));
LIBPROTO(URL_OldGetDefaultPrefs, struct URL_Prefs *);
LIBPROTO(URL_OldLaunchPrefsApp, ULONG);
LIBPROTO(dispatch, LONG, REG(a0, struct RexxMsg *msg), REG(a1, STRPTR *resPtr));
LIBPROTO(URL_GetPrefsA, struct URL_Prefs *, REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(URL_GetPrefs, struct URL_Prefs *, ...);
#else
LIBPROTOVA(URL_GetPrefs, struct URL_Prefs *, REG(a0, Tag tag1), ...);
#endif
LIBPROTO(URL_FreePrefsA, void, REG(a0, struct URL_Prefs *up), REG(a1, struct TagItem *attrs));
LIBPROTOVA(URL_FreePrefs, void, REG(a0, struct URL_Prefs *up), ...);
LIBPROTO(URL_SetPrefsA, ULONG, REG(a0, struct URL_Prefs *p), REG(a1, struct TagItem *attrs));
LIBPROTOVA(URL_SetPrefs, ULONG, REG(a0, struct URL_Prefs *p), ...);
LIBPROTO(URL_LaunchPrefsAppA, ULONG, REG(a0, struct TagItem *attrs));
#if defined(__amigaos4__)
LIBPROTOVA(URL_LaunchPrefsApp, ULONG, ...);
#else
LIBPROTOVA(URL_LaunchPrefsApp, ULONG, REG(a0, Tag tag1), ...);
#endif
LIBPROTO(URL_GetAttr, ULONG, REG(d0, ULONG attr), REG(a0, IPTR *storage));

/* handler.c */
void SAVEDS handler(void);

/* prefs.c */
struct URL_Prefs *copyPrefs(struct URL_Prefs *old);
void initPrefs(struct URL_Prefs *p);
void setDefaultPrefs(struct URL_Prefs *up);
BOOL savePrefs(CONST_STRPTR filename, struct URL_Prefs *up);
BOOL loadPrefs(struct URL_Prefs *p, ULONG mode);
struct URL_Prefs *loadPrefsNotFail(void);
