#ifndef CLIB_INTUITION_PROTOS_H
#define CLIB_INTUITION_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
__AROS_LP1(void, ActivateWindow,
    __AROS_LPA(struct Window *, window, A0),
    struct IntuitionBase *, IntuitionBase, 75, Intuition)
#define ActivateWindow(window) \
    __AROS_LC1(void, ActivateWindow, \
    __AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 75, Intuition)

__AROS_LP1(void, CloseWindow,
    __AROS_LPA(struct Window *, window, A0),
    struct IntuitionBase *, IntuitionBase, 12, Intuition)
#define CloseWindow(window) \
    __AROS_LC1(void, CloseWindow, \
    __AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 12, Intuition)

__AROS_LP4(void, DrawBorder,
    __AROS_LPA(struct RastPort *, rp, A0),
    __AROS_LPA(struct Border   *, border, A1),
    __AROS_LPA(long             , leftOffset, D0),
    __AROS_LPA(long             , topOffset, D1),
    struct IntuitionBase *, IntuitionBase, 18, Intuition)
#define DrawBorder(rp, border, leftOffset, topOffset) \
    __AROS_LC4(void, DrawBorder, \
    __AROS_LCA(struct RastPort *, rp, A0), \
    __AROS_LCA(struct Border   *, border, A1), \
    __AROS_LCA(long             , leftOffset, D0), \
    __AROS_LCA(long             , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 18, Intuition)

__AROS_LP4(void, DrawImage,
    __AROS_LPA(struct RastPort *, rp, A0),
    __AROS_LPA(struct Image    *, image, A1),
    __AROS_LPA(long             , leftOffset, D0),
    __AROS_LPA(long             , topOffset, D1),
    struct IntuitionBase *, IntuitionBase, 19, Intuition)
#define DrawImage(rp, image, leftOffset, topOffset) \
    __AROS_LC4(void, DrawImage, \
    __AROS_LCA(struct RastPort *, rp, A0), \
    __AROS_LCA(struct Image    *, image, A1), \
    __AROS_LCA(long             , leftOffset, D0), \
    __AROS_LCA(long             , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 19, Intuition)

__AROS_LP2(BOOL, ModifyIDCMP,
    __AROS_LPA(struct Window *, window, A0),
    __AROS_LPA(unsigned long  , flags, D0),
    struct IntuitionBase *, IntuitionBase, 25, Intuition)
#define ModifyIDCMP(window, flags) \
    __AROS_LC2(BOOL, ModifyIDCMP, \
    __AROS_LCA(struct Window *, window, A0), \
    __AROS_LCA(unsigned long  , flags, D0), \
    struct IntuitionBase *, IntuitionBase, 25, Intuition)

__AROS_LP1(struct Window *, OpenWindow,
    __AROS_LPA(struct NewWindow *, newWindow, A0),
    struct IntuitionBase *, IntuitionBase, 34, Intuition)
#define OpenWindow(newWindow) \
    __AROS_LC1(struct Window *, OpenWindow, \
    __AROS_LCA(struct NewWindow *, newWindow, A0), \
    struct IntuitionBase *, IntuitionBase, 34, Intuition)

__AROS_LP4(void, PrintIText,
    __AROS_LPA(struct RastPort  *, rp, A0),
    __AROS_LPA(struct IntuiText *, iText, A1),
    __AROS_LPA(long              , left, D0),
    __AROS_LPA(long              , top, D1),
    struct IntuitionBase *, IntuitionBase, 36, Intuition)
#define PrintIText(rp, iText, left, top) \
    __AROS_LC4(void, PrintIText, \
    __AROS_LCA(struct RastPort  *, rp, A0), \
    __AROS_LCA(struct IntuiText *, iText, A1), \
    __AROS_LCA(long              , left, D0), \
    __AROS_LCA(long              , top, D1), \
    struct IntuitionBase *, IntuitionBase, 36, Intuition)

__AROS_LP3(void, SetWindowTitles,
    __AROS_LPA(struct Window *, window, A0),
    __AROS_LPA(UBYTE         *, windowTitle, A1),
    __AROS_LPA(UBYTE         *, screenTitle, A2),
    struct IntuitionBase *, IntuitionBase, 46, Intuition)
#define SetWindowTitles(window, windowTitle, screenTitle) \
    __AROS_LC3(void, SetWindowTitles, \
    __AROS_LCA(struct Window *, window, A0), \
    __AROS_LCA(UBYTE         *, windowTitle, A1), \
    __AROS_LCA(UBYTE         *, screenTitle, A2), \
    struct IntuitionBase *, IntuitionBase, 46, Intuition)

__AROS_LP3(void, SizeWindow,
    __AROS_LPA(struct Window *, window, A0),
    __AROS_LPA(long           , dx, D0),
    __AROS_LPA(long           , dy, D1),
    struct IntuitionBase *, IntuitionBase, 48, Intuition)
#define SizeWindow(window, dx, dy) \
    __AROS_LC3(void, SizeWindow, \
    __AROS_LCA(struct Window *, window, A0), \
    __AROS_LCA(long           , dx, D0), \
    __AROS_LCA(long           , dy, D1), \
    struct IntuitionBase *, IntuitionBase, 48, Intuition)


#endif /* CLIB_INTUITION_PROTOS_H */
