#ifndef CLIB_GADTOOLS_PROTOS_H
#define CLIB_GADTOOLS_PROTOS_H

/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Prototypes for gadtools.library
    Lang: english
*/

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef LIBRARIES_GADTOOLS_H
#   include <libraries/gadtools.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/* Prototypes for stubs in amiga.lib */
struct Gadget * CreateGadget(ULONG kind, struct Gadget * previous,
                             struct NewGadget * ng, ULONG tag1, ...);
struct Menu * CreateMenus(struct NewMenu * newmenu, ULONG tag1, ...);
void DrawBevelBox(struct RastPort *rport, WORD left, WORD top, WORD width,
                  WORD height, ULONG tag1, ...);
APTR GetVisualInfo(struct Screen * screen, ULONG tag1, ...);
LONG GT_GetGadgetAttrs(struct Gadget * gad, struct Window * win,
                       struct Requester * req, ULONG tag1, ...);
void GT_SetGadgetAttrs(struct Gadget * gad, struct Window * win,
                       struct Requester * req, ULONG tag1, ...);
BOOL LayoutMenuItems(struct MenuItem * menuitem, APTR vi, ULONG tag1, ...);
BOOL LayoutMenus(struct Menu * menu, APTR vi, ULONG tag1, ...);

AROS_LP1(struct Gadget *, CreateContext,
    AROS_LPA(struct Gadget **, glistptr, A0),
    struct Library *, GadToolsBase, 19, GadTools)

AROS_LP4(struct Gadget *, CreateGadgetA,
    AROS_LPA(ULONG, kind, D0),
    AROS_LPA(struct Gadget *, previous, A0),
    AROS_LPA(struct NewGadget *, ng, A1),
    AROS_LPA(struct TagItem *, taglist, A2),
    struct Library *, GadToolsBase, 5, GadTools)

AROS_LP2(struct Menu *, CreateMenusA,
    AROS_LPA(struct NewMenu *, newmenu, A0),
    AROS_LPA(struct TagItem *, taglist, A1),
    struct Library *, GadToolsBase, 8, GadTools)

AROS_LP6(void, DrawBevelBoxA,
    AROS_LPA(struct RastPort *, rport, A0),
    AROS_LPA(LONG, left, D0),
    AROS_LPA(LONG, top, D1),
    AROS_LPA(LONG, width, D2),
    AROS_LPA(LONG, height, D3),
    AROS_LPA(struct TagItem *, taglist, A1),
    struct Library *, GadToolsBase, 20, GadTools)

AROS_LP1(void, FreeGadgets,
    AROS_LPA(struct Gadget *, gad, A0),
    struct Library *, GadToolsBase, 6, GadTools)

AROS_LP1(void, FreeMenus,
    AROS_LPA(struct Menu *, menu, A0),
    struct Library *, GadToolsBase, 9, GadTools)

AROS_LP2(APTR, GetVisualInfoA,
    AROS_LPA(struct Screen *, screen, A0),
    AROS_LPA(struct TagItem *, taglist, A1),
    struct Library *, GadToolsBase, 21, GadTools)

AROS_LP1(void, GT_BeginRefresh,
    AROS_LPA(struct Window *, win, A0),
    struct Library *, GadToolsBase, 15, GadTools)

AROS_LP4(LONG, GT_GetGadgetAttrsA,
    AROS_LPA(struct Gadget *, gad, A0),
    AROS_LPA(struct Window *, win, A1),
    AROS_LPA(struct Requester *, req, A2),
    AROS_LPA(struct TagItem *, taglist, A3),
    struct Library *, GadToolsBase, 29, GadTools)

AROS_LP1(struct IntuiMessage *, GT_GetIMsg,
    AROS_LPA(struct MsgPort *, iport, A0),
    struct Library *, GadToolsBase, 12, GadTools)

AROS_LP2(void, GT_EndRefresh,
    AROS_LPA(struct Window *, win, A0),
    AROS_LPA(LONG, complete, D0),
    struct Library *, GadToolsBase, 16, GadTools)

AROS_LP1(struct IntuiMessage *, GT_FilterIMsg,
    AROS_LPA(struct IntuiMessage *, imsg, A1),
    struct Library *, GadToolsBase, 17, GadTools)

AROS_LP1(struct IntuiMessage *, GT_PostFilterIMsg,
    AROS_LPA(struct IntuiMessage *, imsg, A1),
    struct Library *, GadToolsBase, 18, GadTools)

AROS_LP2(void, GT_RefreshWindow,
    AROS_LPA(struct Window *, win, A0),
    AROS_LPA(struct Requester *, req, A1),
    struct Library *, GadToolsBase, 14, GadTools)

AROS_LP1(void, GT_ReplyIMsg,
    AROS_LPA(struct IntuiMessage *, imsg, A1),
    struct Library *, GadToolsBase, 13, GadTools)

AROS_LP4(void, GT_SetGadgetAttrsA,
    AROS_LPA(struct Gadget *, gad, A0),
    AROS_LPA(struct Window *, win, A1),
    AROS_LPA(struct Requester *, req, A2),
    AROS_LPA(struct TagItem *, taglist, A3),
    struct Library *, GadToolsBase, 7, GadTools)

AROS_LP1(void, FreeVisualInfo,
    AROS_LPA(APTR, vi, A0),
    struct Library *, GadToolsBase, 22, GadTools)

AROS_LP3(BOOL, LayoutMenuItemsA,
    AROS_LPA(struct MenuItem *, firstitem, A0),
    AROS_LPA(APTR, vi, A1),
    AROS_LPA(struct TagItem *, taglist, A2),
    struct Library *, GadToolsBase, 10, GadTools)

AROS_LP3(BOOL, LayoutMenusA,
    AROS_LPA(struct Menu *, firstmenu, A0),
    AROS_LPA(APTR, vi, A1),
    AROS_LPA(struct TagItem *, taglist, A2),
    struct Library *, GadToolsBase, 11, GadTools)

#endif /* CLIB_GADTOOLS_PROTOS_H */
