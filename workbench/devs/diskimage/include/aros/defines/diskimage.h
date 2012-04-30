/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_DISKIMAGE_H
#define _INLINE_DISKIMAGE_H

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

#ifndef DISKIMAGE_BASE_NAME
#define DISKIMAGE_BASE_NAME DiskImageBase
#endif /* !DISKIMAGE_BASE_NAME */

#define MountImage(___unit_num, ___filename) \
	AROS_LC2(LONG, MountImage, \
	AROS_LCA(ULONG, (___unit_num), D0), \
	AROS_LCA(CONST_STRPTR, (___filename), A0), \
	struct Library *, DISKIMAGE_BASE_NAME, 7, Diskimage)

#define UnitInfo(___unit_num, ___filename, ___writeprotect) \
	AROS_LC3(LONG, UnitInfo, \
	AROS_LCA(ULONG, (___unit_num), D0), \
	AROS_LCA(STRPTR *, (___filename), A0), \
	AROS_LCA(BOOL *, (___writeprotect), A1), \
	struct Library *, DISKIMAGE_BASE_NAME, 8, Diskimage)

#define WriteProtect(___unit_num, ___writeprotect) \
	AROS_LC2(LONG, WriteProtect, \
	AROS_LCA(ULONG, (___unit_num), D0), \
	AROS_LCA(BOOL, (___writeprotect), D1), \
	struct Library *, DISKIMAGE_BASE_NAME, 9, Diskimage)

#define UnitControlA(___unit_num, ___tags) \
	AROS_LC2(LONG, UnitControlA, \
	AROS_LCA(ULONG, (___unit_num), D0), \
	AROS_LCA(struct TagItem *, (___tags), A0), \
	struct Library *, DISKIMAGE_BASE_NAME, 10, Diskimage)

#ifndef NO_INLINE_STDARG
#define UnitControl(___unit_num, ___tags, ...) \
	({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; UnitControlA((___unit_num), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define ReloadPlugins() \
	AROS_LC0(LONG, ReloadPlugins, \
	struct Library *, DISKIMAGE_BASE_NAME, 11, Diskimage)

#define DoHookPlugins(___hook) \
	AROS_LC1(void, DoHookPlugins, \
	AROS_LCA(struct Hook *, (___hook), A0), \
	struct Library *, DISKIMAGE_BASE_NAME, 12, Diskimage)

#define AddDiskChangeHook(___hook, ___add_or_remove) \
	AROS_LC2(void, AddDiskChangeHook, \
	AROS_LCA(struct Hook *, (___hook), A0), \
	AROS_LCA(BOOL, (___add_or_remove), D0), \
	struct Library *, DISKIMAGE_BASE_NAME, 13, Diskimage)

#define AddReloadPluginsHook(___hook, ___add_or_remove) \
	AROS_LC2(void, AddReloadPluginsHook, \
	AROS_LCA(struct Hook *, (___hook), A0), \
	AROS_LCA(BOOL, (___add_or_remove), D0), \
	struct Library *, DISKIMAGE_BASE_NAME, 14, Diskimage)

#endif /* !_INLINE_DISKIMAGE_H */
