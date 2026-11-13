/* Automatically generated header! Do not edit! */

#ifndef _PPCINLINE_OPENURL_H
#define _PPCINLINE_OPENURL_H

#ifndef __PPCINLINE_MACROS_H
#include <ppcinline/macros.h>
#endif /* !__PPCINLINE_MACROS_H */

#ifndef OPENURL_BASE_NAME
#define OPENURL_BASE_NAME OpenURLBase
#endif /* !OPENURL_BASE_NAME */

#define URL_FreePrefsA(___prefs, ___tags) \
	LP2NR(0x4e, URL_FreePrefsA, struct URL_Prefs *, ___prefs, a0, struct TagItem *, ___tags, a1, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#ifndef NO_INLINE_STDARG
#define URL_FreePrefs(a0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; URL_FreePrefsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define URL_GetAttr(___attr, ___storage) \
	LP2(0x60, ULONG, URL_GetAttr, ULONG, ___attr, d0, ULONG *, ___storage, a0, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#define URL_GetPrefsA(___tags) \
	LP1(0x48, struct URL_Prefs *, URL_GetPrefsA, struct TagItem *, ___tags, a0, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#ifndef NO_INLINE_STDARG
#define URL_GetPrefs(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; URL_GetPrefsA((struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define URL_LaunchPrefsAppA(___tags) \
	LP1(0x5a, ULONG, URL_LaunchPrefsAppA, struct TagItem *, ___tags, a0, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#ifndef NO_INLINE_STDARG
#define URL_LaunchPrefsApp(...) \
	({ULONG _tags[] = { __VA_ARGS__ }; URL_LaunchPrefsAppA((struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define URL_OldFreePrefs(___up) \
	LP1NR(0x2a, URL_OldFreePrefs, struct URL_Prefs *, ___up, a0, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#define URL_OldGetDefaultPrefs() \
	LP0(0x36, struct URL_Prefs *, URL_OldGetDefaultPrefs, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#define URL_OldGetPrefs() \
	LP0(0x24, struct URL_Prefs *, URL_OldGetPrefs, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#define URL_OldLaunchPrefsApp() \
	LP0(0x3c, ULONG, URL_OldLaunchPrefsApp, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#define URL_OldSetPrefs(___up, ___permanent) \
	LP2(0x30, ULONG, URL_OldSetPrefs, struct URL_Prefs *, ___up, a0, BOOL, ___permanent, d0, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#define URL_OpenA(___url, ___tags) \
	LP2(0x1e, ULONG, URL_OpenA, STRPTR, ___url, a0, struct TagItem *, ___tags, a1, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#ifndef NO_INLINE_STDARG
#define URL_Open(a0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; URL_OpenA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define URL_SetPrefsA(___up, ___tags) \
	LP2(0x54, ULONG, URL_SetPrefsA, struct URL_Prefs *, ___up, a0, struct TagItem *, ___tags, a1, \
	, OPENURL_BASE_NAME, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)

#ifndef NO_INLINE_STDARG
#define URL_SetPrefs(a0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; URL_SetPrefsA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#endif /* !_INLINE_OPENURL_H */
