#ifndef CLIB_GADTOOLS_PROTOS_H
#define CLIB_GADTOOLS_PROTOS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for gadtools.library
    Lang: english
*/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

AROS_LP1(struct Gadget *, CreateContext,
    AROS_LPA(struct Gadget **, glistptr, A0),
    struct Library *, GadtoolsBase, 19, Gadtools)

AROS_LP4(struct Gadget *, CreateGadgetA,
    AROS_LPA(ULONG, kind, D0),
    AROS_LPA(struct Gadget *, previous, A0),
    AROS_LPA(struct NewGadget *, ng, A1),
    AROS_LPA(struct TagItem *, taglist, A2),
    struct Library *, GadtoolsBase, 5, Gadtools)

AROS_LP2(struct Menu *, CreateMenusA,
    AROS_LPA(struct NewMenu *, newmenu, A0),
    AROS_LPA(struct TagItem *, taglist, A1),
    struct Library *, GadtoolsBase, 8, Gadtools)

AROS_LP6(void, DrawBevelBoxA,
    AROS_LPA(struct RastPort *, rport, A0),
    AROS_LPA(LONG, left, D0),
    AROS_LPA(LONG, top, D1),
    AROS_LPA(LONG, width, D2),
    AROS_LPA(LONG, height, D3),
    AROS_LPA(struct TagItem *, taglist, A1),
    struct Library *, GadtoolsBase, 20, Gadtools)

AROS_LP1(void, FreeGadgets,
    AROS_LPA(struct Gadget *, gad, A0),
    struct Library *, GadtoolsBase, 6, Gadtools)

AROS_LP1(void, FreeMenus,
    AROS_LPA(struct Menu *, menu, A0),
    struct Library *, GadtoolsBase, 9, Gadtools)

AROS_LP2(APTR, GetVisualInfoA,
    AROS_LPA(struct Screen *, screen, A0),
    AROS_LPA(struct TagItem *, taglist, A1),
    struct Library *, GadtoolsBase, 21, Gadtools)

AROS_LP1(void, GT_BeginRefresh,
    AROS_LPA(struct Window *, win, A0),
    struct Library *, GadtoolsBase, 15, Gadtools)

AROS_LP4(LONG, GT_GetGadgetAttrsA,
    AROS_LPA(struct Gadget *, gad, A0),
    AROS_LPA(struct Window *, win, A1),
    AROS_LPA(struct Requester *, req, A2),
    AROS_LPA(struct TagItem *, taglist, A3),
    struct Library *, GadtoolsBase, 29, Gadtools)

AROS_LP1(struct IntuiMessage *, GT_GetIMsg,
    AROS_LPA(struct MsgPort *, iport, A0),
    struct Library *, GadtoolsBase, 12, Gadtools)

AROS_LP2(void, GT_EndRefresh,
    AROS_LPA(struct Window *, win, A0),
    AROS_LPA(LONG, complete, D0),
    struct Library *, GadtoolsBase, 16, Gadtools)

AROS_LP1(struct IntuiMessage *, GT_FilterIMsg,
    AROS_LPA(struct IntuiMessage *, imsg, A1),
    struct Library *, GadtoolsBase, 17, Gadtools)

AROS_LP1(struct IntuiMessage *, GT_PostFilterIMsg,
    AROS_LPA(struct IntuiMessage *, imsg, A1),
    struct Library *, GadtoolsBase, 18, Gadtools)

AROS_LP2(void, GT_RefreshWindow,
    AROS_LPA(struct Window *, win, A0),
    AROS_LPA(struct Requester *, req, A1),
    struct Library *, GadtoolsBase, 14, Gadtools)

AROS_LP1(void, GT_ReplyIMsg,
    AROS_LPA(struct IntuiMessage *, imsg, A1),
    struct Library *, GadtoolsBase, 13, Gadtools)

AROS_LP4(void, GT_SetGadgetAttrsA,
    AROS_LPA(struct Gadget *, gad, A0),
    AROS_LPA(struct Window *, win, A1),
    AROS_LPA(struct Requester *, req, A2),
    AROS_LPA(struct TagItem *, taglist, A3),
    struct Library *, GadtoolsBase, 7, Gadtools)

AROS_LP1(void, FreeVisualInfo,
    AROS_LPA(APTR, vi, A0),
    struct Library *, GadtoolsBase, 22, Gadtools)

AROS_LP3(BOOL, LayoutMenuItemsA,
    AROS_LPA(struct MenuItem *, firstitem, A0),
    AROS_LPA(APTR, vi, A1),
    AROS_LPA(struct TagItem *, taglist, A2),
    struct Library *, GadtoolsBase, 10, Gadtools)

AROS_LP3(BOOL, LayoutMenusA,
    AROS_LPA(struct Menu *, firstmenu, A0),
    AROS_LPA(APTR, vi, A1),
    AROS_LPA(struct TagItem *, taglist, A2),
    struct Library *, GadtoolsBase, 11, Gadtools)

#endif /* CLIB_GADTOOLS_PROTOS_H */
