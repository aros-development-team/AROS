#ifndef _INLINE_WORKBENCH_H
#define _INLINE_WORKBENCH_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef WORKBENCH_BASE_NAME
#define WORKBENCH_BASE_NAME WorkbenchBase
#endif

#define AddAppIconA(id, userdata, text, msgport, lock, diskobj, taglist) \
	LP7A4(0x3c, struct AppIcon *, AddAppIconA, unsigned long, id, d0, unsigned long, userdata, d1, UBYTE *, text, a0, struct MsgPort *, msgport, a1, struct FileLock *, lock, a2, struct DiskObject *, diskobj, a3, struct TagItem *, taglist, d7, \
	, WORKBENCH_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AddAppIcon(a0, a1, a2, a3, a4, a5, tags...) \
	({ULONG _tags[] = { tags }; AddAppIconA((a0), (a1), (a2), (a3), (a4), (a5), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define AddAppMenuItemA(id, userdata, text, msgport, taglist) \
	LP5(0x48, struct AppMenuItem *, AddAppMenuItemA, unsigned long, id, d0, unsigned long, userdata, d1, UBYTE *, text, a0, struct MsgPort *, msgport, a1, struct TagItem *, taglist, a2, \
	, WORKBENCH_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AddAppMenuItem(a0, a1, a2, a3, tags...) \
	({ULONG _tags[] = { tags }; AddAppMenuItemA((a0), (a1), (a2), (a3), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define AddAppWindowA(id, userdata, window, msgport, taglist) \
	LP5(0x30, struct AppWindow *, AddAppWindowA, unsigned long, id, d0, unsigned long, userdata, d1, struct Window *, window, a0, struct MsgPort *, msgport, a1, struct TagItem *, taglist, a2, \
	, WORKBENCH_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AddAppWindow(a0, a1, a2, a3, tags...) \
	({ULONG _tags[] = { tags }; AddAppWindowA((a0), (a1), (a2), (a3), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define RemoveAppIcon(appIcon) \
	LP1(0x42, BOOL, RemoveAppIcon, struct AppIcon *, appIcon, a0, \
	, WORKBENCH_BASE_NAME)

#define RemoveAppMenuItem(appMenuItem) \
	LP1(0x4e, BOOL, RemoveAppMenuItem, struct AppMenuItem *, appMenuItem, a0, \
	, WORKBENCH_BASE_NAME)

#define RemoveAppWindow(appWindow) \
	LP1(0x36, BOOL, RemoveAppWindow, struct AppWindow *, appWindow, a0, \
	, WORKBENCH_BASE_NAME)

#define WBInfo(lock, name, screen) \
	LP3NR(0x5a, WBInfo, BPTR, lock, a0, STRPTR, name, a1, struct Screen *, screen, a2, \
	, WORKBENCH_BASE_NAME)

#endif /* _INLINE_WORKBENCH_H */
