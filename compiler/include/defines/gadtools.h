#ifndef DEFINES_GADTOOLS_H
#define DEFINES_GADTOOLS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define CreateContext(glistptr) \
    AROS_LC1(struct Gadget *, CreateContext, \
    AROS_LPA(struct Gadget **, glistptr, A0), \
    struct Library *, GadToolsBase, 19, GadTools)

#define CreateGadgetA(kind, previous, ng, taglist) \
    AROS_LC4(struct Gadget *, CreateGadgetA, \
    AROS_LPA(ULONG, kind, D0), \
    AROS_LPA(struct Gadget *, previous, A0), \
    AROS_LPA(struct NewGadget *, ng, A1), \
    AROS_LPA(struct TagItem *, taglist, A2), \
    struct Library *, GadToolsBase, 5, GadTools)

#define CreateMenusA(newmenu, taglist) \
    AROS_LC2(struct Menu *, CreateMenusA, \
    AROS_LPA(struct NewMenu *, newmenu, A0), \
    AROS_LPA(struct TagItem *, taglist, A1), \
    struct Library *, GadToolsBase, 8, GadTools)

#define DrawBevelBoxA(rport, left, top, width, height, taglist) \
    AROS_LC6(void, DrawBevelBoxA, \
    AROS_LPA(struct RastPort *, rport, A0), \
    AROS_LPA(LONG, left, D0), \
    AROS_LPA(LONG, top, D1), \
    AROS_LPA(LONG, width, D2), \
    AROS_LPA(LONG, height, D3), \
    AROS_LPA(struct TagItem *, taglist, A1), \
    struct Library *, GadToolsBase, 20, GadTools)

#define FreeGadgets(gad) \
    AROS_LC1(void, FreeGadgets, \
    AROS_LPA(struct Gadget *, gad, A0), \
    struct Library *, GadToolsBase, 6, GadTools)

#define FreeMenus(menu) \
    AROS_LC1(void, FreeMenus, \
    AROS_LPA(struct Menu *, menu, A0), \
    struct Library *, GadToolsBase, 9, GadTools)

#define FreeVisualInfo(vi) \
    AROS_LC1(void, FreeVisualInfo, \
    AROS_LPA(APTR, vi, A0), \
    struct Library *, GadToolsBase, 22, GadTools)

#define GetVisualInfoA(screen, taglist) \
    AROS_LC2(APTR, GetVisualInfoA, \
    AROS_LPA(struct Screen *, screen, A0), \
    AROS_LPA(struct TagItem *, taglist, A1), \
    struct Library *, GadToolsBase, 21, GadTools)

#define GT_BeginRefresh(win) \
    AROS_LC1(void, GT_BeginRefresh, \
    AROS_LPA(struct Window *, win, A0), \
    struct Library *, GadToolsBase, 15, GadTools)

#define GT_GetGadgetAttrsA(gad, win, req, taglist) \
    AROS_LC4(LONG, GT_GetGadgetAttrsA, \
    AROS_LPA(struct Gadget *, gad, A0), \
    AROS_LPA(struct Window *, win, A1), \
    AROS_LPA(struct Requester *, req, A2), \
    AROS_LPA(struct TagItem *, taglist, A3), \
    struct Library *, GadToolsBase, 29, GadTools)

#define GT_GetIMsg(iport) \
    AROS_LC1(struct IntuiMessage *, GT_GetIMsg, \
    AROS_LPA(struct MsgPort *, iport, A0), \
    struct Library *, GadToolsBase, 12, GadTools)

#define GT_EndRefresh(win, complete) \
    AROS_LC2(void, GT_EndRefresh, \
    AROS_LPA(struct Window *, win, A0), \
    AROS_LPA(LONG, complete, D0), \
    struct Library *, GadToolsBase, 16, GadTools)

#define GT_FilterIMsg(imsg) \
    AROS_LC1(struct IntuiMessage *, GT_FilterIMsg, \
    AROS_LPA(struct IntuiMessage *, imsg, A1), \
    struct Library *, GadToolsBase, 17, GadTools)

#define GT_PostFilterIMsg(imsg) \
    AROS_LC1(struct IntuiMessage *, GT_PostFilterIMsg, \
    AROS_LPA(struct IntuiMessage *, imsg, A1), \
    struct Library *, GadToolsBase, 18, GadTools)

#define GT_RefreshWindow(win, req) \
    AROS_LC2(void, GT_RefreshWindow, \
    AROS_LPA(struct Window *, win, A0), \
    AROS_LPA(struct Requester *, req, A1), \
    struct Library *, GadToolsBase, 14, GadTools)

#define GT_ReplyIMsg(imsg) \
    AROS_LC1(void, GT_ReplyIMsg, \
    AROS_LPA(struct IntuiMessage *, imsg, A1), \
    struct Library *, GadToolsBase, 13, GadTools)

#define GT_SetGadgetAttrsA(gad, win, req, taglist) \
    AROS_LC4(void, GT_SetGadgetAttrsA, \
    AROS_LPA(struct Gadget *, gad, A0), \
    AROS_LPA(struct Window *, win, A1), \
    AROS_LPA(struct Requester *, req, A2), \
    AROS_LPA(struct TagItem *, taglist, A3), \
    struct Library *, GadToolsBase, 7, GadTools)

#define LayoutMenuItemsA(firstitem, vi, taglist) \
    AROS_LC3(BOOL, LayoutMenuItemsA, \
    AROS_LPA(struct MenuItem *, firstitem, A0), \
    AROS_LPA(APTR, vi, A1), \
    AROS_LPA(struct TagItem *, taglist, A2), \
    struct Library *, GadToolsBase, 10, GadTools)

#define LayoutMenusA(firstmenu, vi, taglist) \
    AROS_LC3(BOOL, LayoutMenusA, \
    AROS_LPA(struct Menu *, firstmenu, A0), \
    AROS_LPA(APTR, vi, A1), \
    AROS_LPA(struct TagItem *, taglist, A2), \
    struct Library *, GadToolsBase, 11, GadTools)

#endif /* DEFINES_GADTOOLS_H */
