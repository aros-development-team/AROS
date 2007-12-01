#ifndef CLIB_OPENURL_PROTOS_H
#define CLIB_OPENURL_PROTOS_H

/*
**  $VER: openurl_protos.h 7.2 (1.12.2005)
**
**  C prototypes. For use with 32 bit integers only.
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

#ifndef LIBRARIES_OPENURL_H
# include <libraries/openurl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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
ULONG URL_GetAttr(ULONG attr,ULONG *storage);

#if defined(_DCC) || defined(__SASC) || defined (__STORM__)
ULONG URL_Open(STRPTR, ...);
struct URL_Prefs *URL_GetPrefs(...);
void URL_FreePrefs(struct URL_Prefs *,...);
ULONG URL_SetPrefs(struct URL_Prefs *,...);
ULONG URL_LaunchPrefsApp(...);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* CLIB_OPENURL_PROTOS_H */
