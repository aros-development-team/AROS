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
    struct Library *, GadtoolsBase, 19, Gadtools)

#define CreateGadgetA(kind, previous, ng, taglist) \
    AROS_LC4(struct Gadget *, CreateGadgetA, \
    AROS_LPA(ULONG, kind, D0), \
    AROS_LPA(struct Gadget *, previous, A0), \
    AROS_LPA(struct NewGadget *, ng, A1), \
    AROS_LPA(struct TagItem *, taglist, A2), \
    struct Library *, GadtoolsBase, 5, Gadtools)

#define CreateMenusA(newmenu, taglist) \
    AROS_LC2(struct Menu *, CreateMenusA, \
    AROS_LPA(struct NewMenu *, newmenu, A0), \
    AROS_LPA(struct TagItem *, taglist, A1), \
    struct Library *, GadtoolsBase, 8, Gadtools)

#define FreeGadgets(gad) \
    AROS_LC1(void, FreeGadgets, \
    AROS_LPA(struct Gadget *, gad, A0), \
    struct Library *, GadtoolsBase, 6, Gadtools)

#define FreeMenus(menu) \
    AROS_LC1(void, FreeMenus, \
    AROS_LPA(struct Menu *, menu, A0), \
    struct Library *, GadtoolsBase, 9, Gadtools)

#define DrawBevelBoxA(rport, left, top, width, height, taglist) \
    AROS_LC6(void, DrawBevelBoxA, \
    AROS_LPA(struct RastPort *, rport, A0), \
    AROS_LPA(LONG, left, D0), \
    AROS_LPA(LONG, top, D1), \
    AROS_LPA(LONG, width, D2), \
    AROS_LPA(LONG, height, D3), \
    AROS_LPA(struct TagItem *, taglist, A1), \
    struct Library *, GadtoolsBase, 20, Gadtools)

#define GetVisualInfoA(screen, taglist) \
    AROS_LC2(APTR, GetVisualInfoA, \
    AROS_LPA(struct Screen *, screen, A0), \
    AROS_LPA(struct TagItem *, taglist, A1), \
    struct Library *, GadtoolsBase, 21, Gadtools)

#define GT_BeginRefresh(win) \
    AROS_LC1(void, GT_BeginRefresh, \
    AROS_LPA(struct Window *, win, A0), \
    struct Library *, GadtoolsBase, 15, Gadtools)

#define GT_GetGadgetAttrsA(gad, win, req, taglist) \
    AROS_LC4(LONG, GT_GetGadgetAttrsA, \
    AROS_LPA(struct Gadget *, gad, A0), \
    AROS_LPA(struct Window *, win, A1), \
    AROS_LPA(struct Requester *, req, A2), \
    AROS_LPA(struct TagItem *, taglist, A3), \
    struct Library *, GadtoolsBase, 29, Gadtools)

#define GT_GetIMsg(iport) \
    AROS_LC1(struct IntuiMessage *, GT_GetIMsg, \
    AROS_LPA(struct MsgPort *, iport, A0), \
    struct Library *, GadtoolsBase, 12, Gadtools)

#define GT_EndRefresh(win, complete) \
    AROS_LC2(void, GT_EndRefresh, \
    AROS_LPA(struct Window *, win, A0), \
    AROS_LPA(LONG, complete, D0), \
    struct Library *, GadtoolsBase, 16, Gadtools)

#define GT_FilterIMsg(imsg) \
    AROS_LC1(struct IntuiMessage *, GT_FilterIMsg, \
    AROS_LPA(struct IntuiMessage *, imsg, A1), \
    struct Library *, GadtoolsBase, 17, Gadtools)

#define GT_PostFilterIMsg(imsg) \
    AROS_LC1(struct IntuiMessage *, GT_PostFilterIMsg, \
    AROS_LPA(struct IntuiMessage *, imsg, A1), \
    struct Library *, GadtoolsBase, 18, Gadtools)

#define GT_RefreshWindow(win, req) \
    AROS_LC2(void, GT_RefreshWindow, \
    AROS_LPA(struct Window *, win, A0), \
    AROS_LPA(struct Requester *, req, A1), \
    struct Library *, GadtoolsBase, 14, Gadtools)

#define GT_ReplyIMsg(imsg) \
    AROS_LC1(void, GT_ReplyIMsg, \
    AROS_LPA(struct IntuiMessage *, imsg, A1), \
    struct Library *, GadtoolsBase, 13, Gadtools)

#define GT_SetGadgetAttrsA(gad, win, req, taglist) \
    AROS_LC4(void, GT_SetGadgetAttrsA, \
    AROS_LPA(struct Gadget *, gad, A0), \
    AROS_LPA(struct Window *, win, A1), \
    AROS_LPA(struct Requester *, req, A2), \
    AROS_LPA(struct TagItem *, taglist, A3), \
    struct Library *, GadtoolsBase, 7, Gadtools)

#define FreeVisualInfo(vi) \
    AROS_LC1(void, FreeVisualInfo, \
    AROS_LPA(APTR, vi, A0), \
    struct Library *, GadtoolsBase, 22, Gadtools)

#define LayoutMenuItemsA(firstitem, vi, taglist) \
    AROS_LC3(BOOL, LayoutMenuItemsA, \
    AROS_LPA(struct MenuItem *, firstitem, A0), \
    AROS_LPA(APTR, vi, A1), \
    AROS_LPA(struct TagItem *, taglist, A2), \
    struct Library *, GadtoolsBase, 10, Gadtools)

#define LayoutMenusA(firstmenu, vi, taglist) \
    AROS_LC3(BOOL, LayoutMenusA, \
    AROS_LPA(struct Menu *, firstmenu, A0), \
    AROS_LPA(APTR, vi, A1), \
    AROS_LPA(struct TagItem *, taglist, A2), \
    struct Library *, GadtoolsBase, 11, Gadtools)

#endif /* DEFINES_GADTOOLS_H */
