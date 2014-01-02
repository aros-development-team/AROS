#ifndef _VBCCINLINE_OPENURL_H
#define _VBCCINLINE_OPENURL_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

ULONG __URL_OpenA(__reg("a6") struct Library *, __reg("a0") STRPTR url, __reg("a1") struct TagItem * tags)="\tjsr\t-30(a6)";
#define URL_OpenA(url, tags) __URL_OpenA(OpenURLBase, (url), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
ULONG __URL_Open(__reg("a6") struct Library *, __reg("a0") STRPTR url, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-30(a6)\n\tmovea.l\t(a7)+,a1";
#define URL_Open(...) __URL_Open(OpenURLBase, __VA_ARGS__)
#endif

struct URL_Prefs * __URL_OldGetPrefs(__reg("a6") struct Library *)="\tjsr\t-36(a6)";
#define URL_OldGetPrefs() __URL_OldGetPrefs(OpenURLBase)

void __URL_OldFreePrefs(__reg("a6") struct Library *, __reg("a0") struct URL_Prefs * up)="\tjsr\t-42(a6)";
#define URL_OldFreePrefs(up) __URL_OldFreePrefs(OpenURLBase, (up))

ULONG __URL_OldSetPrefs(__reg("a6") struct Library *, __reg("a0") struct URL_Prefs * up, __reg("d0") BOOL permanent)="\tjsr\t-48(a6)";
#define URL_OldSetPrefs(up, permanent) __URL_OldSetPrefs(OpenURLBase, (up), (permanent))

struct URL_Prefs * __URL_OldGetDefaultPrefs(__reg("a6") struct Library *)="\tjsr\t-54(a6)";
#define URL_OldGetDefaultPrefs() __URL_OldGetDefaultPrefs(OpenURLBase)

ULONG __URL_OldLaunchPrefsApp(__reg("a6") struct Library *)="\tjsr\t-60(a6)";
#define URL_OldLaunchPrefsApp() __URL_OldLaunchPrefsApp(OpenURLBase)

struct URL_Prefs * __URL_GetPrefsA(__reg("a6") struct Library *, __reg("a0") struct TagItem * tags)="\tjsr\t-72(a6)";
#define URL_GetPrefsA(tags) __URL_GetPrefsA(OpenURLBase, (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct URL_Prefs * __URL_GetPrefs(__reg("a6") struct Library *, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-72(a6)\n\tmovea.l\t(a7)+,a0";
#define URL_GetPrefs(...) __URL_GetPrefs(OpenURLBase, __VA_ARGS__)
#endif

void __URL_FreePrefsA(__reg("a6") struct Library *, __reg("a0") struct URL_Prefs * prefs, __reg("a1") struct TagItem * tags)="\tjsr\t-78(a6)";
#define URL_FreePrefsA(prefs, tags) __URL_FreePrefsA(OpenURLBase, (prefs), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
void __URL_FreePrefs(__reg("a6") struct Library *, __reg("a0") struct URL_Prefs * prefs, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-78(a6)\n\tmovea.l\t(a7)+,a1";
#define URL_FreePrefs(...) __URL_FreePrefs(OpenURLBase, __VA_ARGS__)
#endif

ULONG __URL_SetPrefsA(__reg("a6") struct Library *, __reg("a0") struct URL_Prefs * up, __reg("a1") struct TagItem * tags)="\tjsr\t-84(a6)";
#define URL_SetPrefsA(up, tags) __URL_SetPrefsA(OpenURLBase, (up), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
ULONG __URL_SetPrefs(__reg("a6") struct Library *, __reg("a0") struct URL_Prefs * up, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-84(a6)\n\tmovea.l\t(a7)+,a1";
#define URL_SetPrefs(...) __URL_SetPrefs(OpenURLBase, __VA_ARGS__)
#endif

ULONG __URL_LaunchPrefsAppA(__reg("a6") struct Library *, __reg("a0") struct TagItem * tags)="\tjsr\t-90(a6)";
#define URL_LaunchPrefsAppA(tags) __URL_LaunchPrefsAppA(OpenURLBase, (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
ULONG __URL_LaunchPrefsApp(__reg("a6") struct Library *, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-90(a6)\n\tmovea.l\t(a7)+,a0";
#define URL_LaunchPrefsApp(...) __URL_LaunchPrefsApp(OpenURLBase, __VA_ARGS__)
#endif

ULONG __URL_GetAttr(__reg("a6") struct Library *, __reg("d0") ULONG attr, __reg("a0") ULONG * storage)="\tjsr\t-96(a6)";
#define URL_GetAttr(attr, storage) __URL_GetAttr(OpenURLBase, (attr), (storage))

#endif /*  _VBCCINLINE_OPENURL_H  */
