/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_OPENURL_H
#define _INLINE_OPENURL_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef OPENURL_BASE_NAME
#define OPENURL_BASE_NAME OpenURLBase
#endif /* !OPENURL_BASE_NAME */

#define URL_OpenA(___url, ___tags) \
	AROS_LC2(ULONG, URL_OpenA, \
	AROS_LCA(STRPTR, (___url), A0), \
	AROS_LCA(struct TagItem *, (___tags), A1), \
	struct Library *, OPENURL_BASE_NAME, 5, Openurl)

#ifndef NO_INLINE_STDARG
#define URL_Open(___url, ___last, ...) \
	({_sfdc_vararg _tags[] = { ___last, __VA_ARGS__ }; URL_OpenA((___url), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define URL_OldGetPrefs() \
	AROS_LC0(struct URL_Prefs *, URL_OldGetPrefs, \
	struct Library *, OPENURL_BASE_NAME, 6, Openurl)

#define URL_OldFreePrefs(___up) \
	AROS_LC1NR(void, URL_OldFreePrefs, \
	AROS_LCA(struct URL_Prefs *, (___up), A0), \
	struct Library *, OPENURL_BASE_NAME, 7, Openurl)

#define URL_OldSetPrefs(___up, ___permanent) \
	AROS_LC2(ULONG, URL_OldSetPrefs, \
	AROS_LCA(struct URL_Prefs *, (___up), A0), \
	AROS_LCA(BOOL, (___permanent), D0), \
	struct Library *, OPENURL_BASE_NAME, 8, Openurl)

#define URL_OldGetDefaultPrefs() \
	AROS_LC0(struct URL_Prefs *, URL_OldGetDefaultPrefs, \
	struct Library *, OPENURL_BASE_NAME, 9, Openurl)

#define URL_OldLaunchPrefsApp() \
	AROS_LC0(ULONG, URL_OldLaunchPrefsApp, \
	struct Library *, OPENURL_BASE_NAME, 10, Openurl)

#define URL_GetPrefsA(___tags) \
	AROS_LC1(struct URL_Prefs *, URL_GetPrefsA, \
	AROS_LCA(struct TagItem *, (___tags), A0), \
	struct Library *, OPENURL_BASE_NAME, 12, Openurl)

#ifndef NO_INLINE_STDARG
#define URL_GetPrefs(___last, ...) \
	({_sfdc_vararg _tags[] = { ___last, __VA_ARGS__ }; URL_GetPrefsA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define URL_FreePrefsA(___prefs, ___tags) \
	AROS_LC2NR(void, URL_FreePrefsA, \
	AROS_LCA(struct URL_Prefs *, (___prefs), A0), \
	AROS_LCA(struct TagItem *, (___tags), A1), \
	struct Library *, OPENURL_BASE_NAME, 13, Openurl)

#ifndef NO_INLINE_STDARG
#define URL_FreePrefs(___prefs, ___last, ...) \
	({_sfdc_vararg _tags[] = { ___last, __VA_ARGS__ }; URL_FreePrefsA((___prefs), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define URL_SetPrefsA(___up, ___tags) \
	AROS_LC2(ULONG, URL_SetPrefsA, \
	AROS_LCA(struct URL_Prefs *, (___up), A0), \
	AROS_LCA(struct TagItem *, (___tags), A1), \
	struct Library *, OPENURL_BASE_NAME, 14, Openurl)

#ifndef NO_INLINE_STDARG
#define URL_SetPrefs(___up, ___last, ...) \
	({_sfdc_vararg _tags[] = { ___last, __VA_ARGS__ }; URL_SetPrefsA((___up), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define URL_LaunchPrefsAppA(___tags) \
	AROS_LC1(ULONG, URL_LaunchPrefsAppA, \
	AROS_LCA(struct TagItem *, (___tags), A0), \
	struct Library *, OPENURL_BASE_NAME, 15, Openurl)

#ifndef NO_INLINE_STDARG
#define URL_LaunchPrefsApp(___last, ...) \
	({_sfdc_vararg _tags[] = { ___last, __VA_ARGS__ }; URL_LaunchPrefsAppA((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define URL_GetAttr(___attr, ___storage) \
	AROS_LC2(ULONG, URL_GetAttr, \
	AROS_LCA(ULONG, (___attr), D0), \
	AROS_LCA(ULONG *, (___storage), A0), \
	struct Library *, OPENURL_BASE_NAME, 16, Openurl)

#endif /* !_INLINE_OPENURL_H */
