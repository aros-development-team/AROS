#ifndef _INLINE_GADTOOLS_H
#define _INLINE_GADTOOLS_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef GADTOOLS_BASE_NAME
#define GADTOOLS_BASE_NAME GadToolsBase
#endif

#define CreateContext(glistptr) \
	LP1(0x72, struct Gadget *, CreateContext, struct Gadget **, glistptr, a0, \
	, GADTOOLS_BASE_NAME)

#define CreateGadgetA(kind, gad, ng, taglist) \
	LP4(0x1e, struct Gadget *, CreateGadgetA, unsigned long, kind, d0, struct Gadget *, gad, a0, struct NewGadget *, ng, a1, struct TagItem *, taglist, a2, \
	, GADTOOLS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CreateGadget(a0, a1, a2, tags...) \
	({ULONG _tags[] = { tags }; CreateGadgetA((a0), (a1), (a2), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define CreateMenusA(newmenu, taglist) \
	LP2(0x30, struct Menu *, CreateMenusA, struct NewMenu *, newmenu, a0, struct TagItem *, taglist, a1, \
	, GADTOOLS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define CreateMenus(a0, tags...) \
	({ULONG _tags[] = { tags }; CreateMenusA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define DrawBevelBoxA(rport, left, top, width, height, taglist) \
	LP6NR(0x78, DrawBevelBoxA, struct RastPort *, rport, a0, long, left, d0, long, top, d1, long, width, d2, long, height, d3, struct TagItem *, taglist, a1, \
	, GADTOOLS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define DrawBevelBox(a0, a1, a2, a3, a4, tags...) \
	({ULONG _tags[] = { tags }; DrawBevelBoxA((a0), (a1), (a2), (a3), (a4), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define FreeGadgets(gad) \
	LP1NR(0x24, FreeGadgets, struct Gadget *, gad, a0, \
	, GADTOOLS_BASE_NAME)

#define FreeMenus(menu) \
	LP1NR(0x36, FreeMenus, struct Menu *, menu, a0, \
	, GADTOOLS_BASE_NAME)

#define FreeVisualInfo(vi) \
	LP1NR(0x84, FreeVisualInfo, APTR, vi, a0, \
	, GADTOOLS_BASE_NAME)

#define GT_BeginRefresh(win) \
	LP1NR(0x5a, GT_BeginRefresh, struct Window *, win, a0, \
	, GADTOOLS_BASE_NAME)

#define GT_EndRefresh(win, complete) \
	LP2NR(0x60, GT_EndRefresh, struct Window *, win, a0, long, complete, d0, \
	, GADTOOLS_BASE_NAME)

#define GT_FilterIMsg(imsg) \
	LP1(0x66, struct IntuiMessage *, GT_FilterIMsg, struct IntuiMessage *, imsg, a1, \
	, GADTOOLS_BASE_NAME)

#define GT_GetGadgetAttrsA(gad, win, req, taglist) \
	LP4(0xae, LONG, GT_GetGadgetAttrsA, struct Gadget *, gad, a0, struct Window *, win, a1, struct Requester *, req, a2, struct TagItem *, taglist, a3, \
	, GADTOOLS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define GT_GetGadgetAttrs(a0, a1, a2, tags...) \
	({ULONG _tags[] = { tags }; GT_GetGadgetAttrsA((a0), (a1), (a2), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define GT_GetIMsg(iport) \
	LP1(0x48, struct IntuiMessage *, GT_GetIMsg, struct MsgPort *, iport, a0, \
	, GADTOOLS_BASE_NAME)

#define GT_PostFilterIMsg(imsg) \
	LP1(0x6c, struct IntuiMessage *, GT_PostFilterIMsg, struct IntuiMessage *, imsg, a1, \
	, GADTOOLS_BASE_NAME)

#define GT_RefreshWindow(win, req) \
	LP2NR(0x54, GT_RefreshWindow, struct Window *, win, a0, struct Requester *, req, a1, \
	, GADTOOLS_BASE_NAME)

#define GT_ReplyIMsg(imsg) \
	LP1NR(0x4e, GT_ReplyIMsg, struct IntuiMessage *, imsg, a1, \
	, GADTOOLS_BASE_NAME)

#define GT_SetGadgetAttrsA(gad, win, req, taglist) \
	LP4NR(0x2a, GT_SetGadgetAttrsA, struct Gadget *, gad, a0, struct Window *, win, a1, struct Requester *, req, a2, struct TagItem *, taglist, a3, \
	, GADTOOLS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define GT_SetGadgetAttrs(a0, a1, a2, tags...) \
	({ULONG _tags[] = { tags }; GT_SetGadgetAttrsA((a0), (a1), (a2), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define GetVisualInfoA(screen, taglist) \
	LP2(0x7e, APTR, GetVisualInfoA, struct Screen *, screen, a0, struct TagItem *, taglist, a1, \
	, GADTOOLS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define GetVisualInfo(a0, tags...) \
	({ULONG _tags[] = { tags }; GetVisualInfoA((a0), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define LayoutMenuItemsA(firstitem, vi, taglist) \
	LP3(0x3c, BOOL, LayoutMenuItemsA, struct MenuItem *, firstitem, a0, APTR, vi, a1, struct TagItem *, taglist, a2, \
	, GADTOOLS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LayoutMenuItems(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; LayoutMenuItemsA((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#define LayoutMenusA(firstmenu, vi, taglist) \
	LP3(0x42, BOOL, LayoutMenusA, struct Menu *, firstmenu, a0, APTR, vi, a1, struct TagItem *, taglist, a2, \
	, GADTOOLS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define LayoutMenus(a0, a1, tags...) \
	({ULONG _tags[] = { tags }; LayoutMenusA((a0), (a1), (struct TagItem *)_tags);})
#endif /* !NO_INLINE_STDARG */

#endif /* _INLINE_GADTOOLS_H */
