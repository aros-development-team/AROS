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

__AROS_LP1(void, AddClass,
    __AROS_LPA(struct IClass *, classPtr, A0),
    struct IntuitionBase *, IntuitionBase, 114, Intuition)
#define AddClass(classPtr) \
    __AROS_LC1(void, AddClass, \
    __AROS_LCA(struct IClass *, classPtr, A0), \
    struct IntuitionBase *, IntuitionBase, 114, Intuition)

__AROS_LP1(BOOL, CloseScreen,
    __AROS_LPA(struct Screen *, screen, A0),
    struct IntuitionBase *, IntuitionBase, 11, Intuition)
#define CloseScreen(screen) \
    __AROS_LC1(BOOL, CloseScreen, \
    __AROS_LCA(struct Screen *, screen, A0), \
    struct IntuitionBase *, IntuitionBase, 11, Intuition)

__AROS_LP1(void, CloseWindow,
    __AROS_LPA(struct Window *, window, A0),
    struct IntuitionBase *, IntuitionBase, 12, Intuition)
#define CloseWindow(window) \
    __AROS_LC1(void, CloseWindow, \
    __AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 12, Intuition)

__AROS_LP1(void, DisposeObject,
    __AROS_LPA(APTR, object, A0),
    struct IntuitionBase *, IntuitionBase, 107, Intuition)
#define DisposeObject(object) \
    __AROS_LC1(void, DisposeObject, \
    __AROS_LCA(APTR, object, A0), \
    struct IntuitionBase *, IntuitionBase, 107, Intuition)

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

__AROS_LP1(BOOL, FreeClass,
    __AROS_LPA(struct IClass *, classPtr, A0),
    struct IntuitionBase *, IntuitionBase, 119, Intuition)
#define FreeClass(classPtr) \
    __AROS_LC1(BOOL, FreeClass, \
    __AROS_LCA(struct IClass *, classPtr, A0), \
    struct IntuitionBase *, IntuitionBase, 119, Intuition)

__AROS_LP2(void, FreeScreenDrawInfo,
    __AROS_LPA(struct Screen   *, screen, A0),
    __AROS_LPA(struct DrawInfo *, drawInfo, A1),
    struct IntuitionBase *, IntuitionBase, 116, Intuition)
#define FreeScreenDrawInfo(screen, drawInfo) \
    __AROS_LC2(void, FreeScreenDrawInfo, \
    __AROS_LCA(struct Screen   *, screen, A0), \
    __AROS_LCA(struct DrawInfo *, drawInfo, A1), \
    struct IntuitionBase *, IntuitionBase, 116, Intuition)

__AROS_LP3(ULONG, GetAttr,
    __AROS_LPA(unsigned long, attrID, D0),
    __AROS_LPA(APTR         , object, A0),
    __AROS_LPA(ULONG       *, storagePtr, A1),
    struct IntuitionBase *, IntuitionBase, 109, Intuition)
#define GetAttr(attrID, object, storagePtr) \
    __AROS_LC3(ULONG, GetAttr, \
    __AROS_LCA(unsigned long, attrID, D0), \
    __AROS_LCA(APTR         , object, A0), \
    __AROS_LCA(ULONG       *, storagePtr, A1), \
    struct IntuitionBase *, IntuitionBase, 109, Intuition)

__AROS_LP1(void, GetDefaultPubScreen,
    __AROS_LPA(UBYTE *, nameBuffer, A0),
    struct IntuitionBase *, IntuitionBase, 97, Intuition)
#define GetDefaultPubScreen(nameBuffer) \
    __AROS_LC1(void, GetDefaultPubScreen, \
    __AROS_LCA(UBYTE *, nameBuffer, A0), \
    struct IntuitionBase *, IntuitionBase, 97, Intuition)

__AROS_LP1(struct DrawInfo *, GetScreenDrawInfo,
    __AROS_LPA(struct Screen *, screen, A0),
    struct IntuitionBase *, IntuitionBase, 115, Intuition)
#define GetScreenDrawInfo(screen) \
    __AROS_LC1(struct DrawInfo *, GetScreenDrawInfo, \
    __AROS_LCA(struct Screen *, screen, A0), \
    struct IntuitionBase *, IntuitionBase, 115, Intuition)

__AROS_LP1(struct Screen *, LockPubScreen,
    __AROS_LPA(UBYTE *, name, A0),
    struct IntuitionBase *, IntuitionBase, 85, Intuition)
#define LockPubScreen(name) \
    __AROS_LC1(struct Screen *, LockPubScreen, \
    __AROS_LCA(UBYTE *, name, A0), \
    struct IntuitionBase *, IntuitionBase, 85, Intuition)

__AROS_LP5(struct IClass *, MakeClass,
    __AROS_LPA(UBYTE         *, classID, A0),
    __AROS_LPA(UBYTE         *, superClassID, A1),
    __AROS_LPA(struct IClass *, superClassPtr, A2),
    __AROS_LPA(unsigned long  , instanceSize, D0),
    __AROS_LPA(unsigned long  , flags, D1),
    struct IntuitionBase *, IntuitionBase, 113, Intuition)
#define MakeClass(classID, superClassID, superClassPtr, instanceSize, flags) \
    __AROS_LC5(struct IClass *, MakeClass, \
    __AROS_LCA(UBYTE         *, classID, A0), \
    __AROS_LCA(UBYTE         *, superClassID, A1), \
    __AROS_LCA(struct IClass *, superClassPtr, A2), \
    __AROS_LCA(unsigned long  , instanceSize, D0), \
    __AROS_LCA(unsigned long  , flags, D1), \
    struct IntuitionBase *, IntuitionBase, 113, Intuition)

__AROS_LP2(BOOL, ModifyIDCMP,
    __AROS_LPA(struct Window *, window, A0),
    __AROS_LPA(unsigned long  , flags, D0),
    struct IntuitionBase *, IntuitionBase, 25, Intuition)
#define ModifyIDCMP(window, flags) \
    __AROS_LC2(BOOL, ModifyIDCMP, \
    __AROS_LCA(struct Window *, window, A0), \
    __AROS_LCA(unsigned long  , flags, D0), \
    struct IntuitionBase *, IntuitionBase, 25, Intuition)

__AROS_LP8(void, ModifyProp,
    __AROS_LPA(struct Gadget    *, gadget, A0),
    __AROS_LPA(struct Window    *, window, A1),
    __AROS_LPA(struct Requester *, requester, A2),
    __AROS_LPA(unsigned long     , flags, D0),
    __AROS_LPA(unsigned long     , horizPot, D1),
    __AROS_LPA(unsigned long     , vertPot, D2),
    __AROS_LPA(unsigned long     , horizBody, D3),
    __AROS_LPA(unsigned long     , vertBody, D4),
    struct IntuitionBase *, IntuitionBase, 26, Intuition)
#define ModifyProp(gadget, window, requester, flags, horizPot, vertPot, horizBody, vertBody) \
    __AROS_LC8(void, ModifyProp, \
    __AROS_LCA(struct Gadget    *, gadget, A0), \
    __AROS_LCA(struct Window    *, window, A1), \
    __AROS_LCA(struct Requester *, requester, A2), \
    __AROS_LCA(unsigned long     , flags, D0), \
    __AROS_LCA(unsigned long     , horizPot, D1), \
    __AROS_LCA(unsigned long     , vertPot, D2), \
    __AROS_LCA(unsigned long     , horizBody, D3), \
    __AROS_LCA(unsigned long     , vertBody, D4), \
    struct IntuitionBase *, IntuitionBase, 26, Intuition)

__AROS_LP9(void, NewModifyProp,
    __AROS_LPA(struct Gadget    *, gadget, A0),
    __AROS_LPA(struct Window    *, window, A1),
    __AROS_LPA(struct Requester *, requester, A2),
    __AROS_LPA(unsigned long     , flags, D0),
    __AROS_LPA(unsigned long     , horizPot, D1),
    __AROS_LPA(unsigned long     , vertPot, D2),
    __AROS_LPA(unsigned long     , horizBody, D3),
    __AROS_LPA(unsigned long     , vertBody, D4),
    __AROS_LPA(long              , numGad, D5),
    struct IntuitionBase *, IntuitionBase, 78, Intuition)
#define NewModifyProp(gadget, window, requester, flags, horizPot, vertPot, horizBody, vertBody, numGad) \
    __AROS_LC9(void, NewModifyProp, \
    __AROS_LCA(struct Gadget    *, gadget, A0), \
    __AROS_LCA(struct Window    *, window, A1), \
    __AROS_LCA(struct Requester *, requester, A2), \
    __AROS_LCA(unsigned long     , flags, D0), \
    __AROS_LCA(unsigned long     , horizPot, D1), \
    __AROS_LCA(unsigned long     , vertPot, D2), \
    __AROS_LCA(unsigned long     , horizBody, D3), \
    __AROS_LCA(unsigned long     , vertBody, D4), \
    __AROS_LCA(long              , numGad, D5), \
    struct IntuitionBase *, IntuitionBase, 78, Intuition)

__AROS_LP3(APTR, NewObjectA,
    __AROS_LPA(struct IClass  *, classPtr, A0),
    __AROS_LPA(UBYTE          *, classID, A1),
    __AROS_LPA(struct TagItem *, tagList, A2),
    struct IntuitionBase *, IntuitionBase, 106, Intuition)
#define NewObjectA(classPtr, classID, tagList) \
    __AROS_LC3(APTR, NewObjectA, \
    __AROS_LCA(struct IClass  *, classPtr, A0), \
    __AROS_LCA(UBYTE          *, classID, A1), \
    __AROS_LCA(struct TagItem *, tagList, A2), \
    struct IntuitionBase *, IntuitionBase, 106, Intuition)

__AROS_LP1(struct Screen *, OpenScreen,
    __AROS_LPA(struct NewScreen *, newScreen, A0),
    struct IntuitionBase *, IntuitionBase, 33, Intuition)
#define OpenScreen(newScreen) \
    __AROS_LC1(struct Screen *, OpenScreen, \
    __AROS_LCA(struct NewScreen *, newScreen, A0), \
    struct IntuitionBase *, IntuitionBase, 33, Intuition)

__AROS_LP2(struct Screen *, OpenScreenTagList,
    __AROS_LPA(struct NewScreen *, newScreen, A0),
    __AROS_LPA(struct TagItem   *, tagList, A1),
    struct IntuitionBase *, IntuitionBase, 102, Intuition)
#define OpenScreenTagList(newScreen, tagList) \
    __AROS_LC2(struct Screen *, OpenScreenTagList, \
    __AROS_LCA(struct NewScreen *, newScreen, A0), \
    __AROS_LCA(struct TagItem   *, tagList, A1), \
    struct IntuitionBase *, IntuitionBase, 102, Intuition)

__AROS_LP1(struct Window *, OpenWindow,
    __AROS_LPA(struct NewWindow *, newWindow, A0),
    struct IntuitionBase *, IntuitionBase, 34, Intuition)
#define OpenWindow(newWindow) \
    __AROS_LC1(struct Window *, OpenWindow, \
    __AROS_LCA(struct NewWindow *, newWindow, A0), \
    struct IntuitionBase *, IntuitionBase, 34, Intuition)

__AROS_LP2(struct Window *, OpenWindowTagList,
    __AROS_LPA(struct NewWindow *, newWindow, A0),
    __AROS_LPA(struct TagItem   *, tagList, A1),
    struct IntuitionBase *, IntuitionBase, 101, Intuition)
#define OpenWindowTagList(newWindow, tagList) \
    __AROS_LC2(struct Window *, OpenWindowTagList, \
    __AROS_LCA(struct NewWindow *, newWindow, A0), \
    __AROS_LCA(struct TagItem   *, tagList, A1), \
    struct IntuitionBase *, IntuitionBase, 101, Intuition)

__AROS_LP4(void, PrintIText,
    __AROS_LPA(struct RastPort  *, rp, A0),
    __AROS_LPA(struct IntuiText *, iText, A1),
    __AROS_LPA(long              , leftOffset, D0),
    __AROS_LPA(long              , topOffset, D1),
    struct IntuitionBase *, IntuitionBase, 36, Intuition)
#define PrintIText(rp, iText, leftOffset, topOffset) \
    __AROS_LC4(void, PrintIText, \
    __AROS_LCA(struct RastPort  *, rp, A0), \
    __AROS_LCA(struct IntuiText *, iText, A1), \
    __AROS_LCA(long              , leftOffset, D0), \
    __AROS_LCA(long              , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 36, Intuition)

__AROS_LP3(void, RefreshGadgets,
    __AROS_LPA(struct Gadget    *, gadgets, A0),
    __AROS_LPA(struct Window    *, window, A1),
    __AROS_LPA(struct Requester *, requester, A2),
    struct IntuitionBase *, IntuitionBase, 37, Intuition)
#define RefreshGadgets(gadgets, window, requester) \
    __AROS_LC3(void, RefreshGadgets, \
    __AROS_LCA(struct Gadget    *, gadgets, A0), \
    __AROS_LCA(struct Window    *, window, A1), \
    __AROS_LCA(struct Requester *, requester, A2), \
    struct IntuitionBase *, IntuitionBase, 37, Intuition)

__AROS_LP4(void, RefreshGList,
    __AROS_LPA(struct Gadget    *, gadgets, A0),
    __AROS_LPA(struct Window    *, window, A1),
    __AROS_LPA(struct Requester *, requester, A2),
    __AROS_LPA(long              , numGad, D0),
    struct IntuitionBase *, IntuitionBase, 72, Intuition)
#define RefreshGList(gadgets, window, requester, numGad) \
    __AROS_LC4(void, RefreshGList, \
    __AROS_LCA(struct Gadget    *, gadgets, A0), \
    __AROS_LCA(struct Window    *, window, A1), \
    __AROS_LCA(struct Requester *, requester, A2), \
    __AROS_LCA(long              , numGad, D0), \
    struct IntuitionBase *, IntuitionBase, 72, Intuition)

__AROS_LP1(void, RemoveClass,
    __AROS_LPA(struct IClass *, classPtr, A0),
    struct IntuitionBase *, IntuitionBase, 118, Intuition)
#define RemoveClass(classPtr) \
    __AROS_LC1(void, RemoveClass, \
    __AROS_LCA(struct IClass *, classPtr, A0), \
    struct IntuitionBase *, IntuitionBase, 118, Intuition)

__AROS_LP2(ULONG, SetAttrsA,
    __AROS_LPA(APTR            , object, A0),
    __AROS_LPA(struct TagItem *, tagList, A1),
    struct IntuitionBase *, IntuitionBase, 108, Intuition)
#define SetAttrsA(object, tagList) \
    __AROS_LC2(ULONG, SetAttrsA, \
    __AROS_LCA(APTR            , object, A0), \
    __AROS_LCA(struct TagItem *, tagList, A1), \
    struct IntuitionBase *, IntuitionBase, 108, Intuition)

__AROS_LP1(void, SetDefaultPubScreen,
    __AROS_LPA(UBYTE *, name, A0),
    struct IntuitionBase *, IntuitionBase, 90, Intuition)
#define SetDefaultPubScreen(name) \
    __AROS_LC1(void, SetDefaultPubScreen, \
    __AROS_LCA(UBYTE *, name, A0), \
    struct IntuitionBase *, IntuitionBase, 90, Intuition)

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

__AROS_LP2(void, UnlockPubScreen,
    __AROS_LPA(UBYTE         *, name, A0),
    __AROS_LPA(struct Screen *, screen, A1),
    struct IntuitionBase *, IntuitionBase, 86, Intuition)
#define UnlockPubScreen(name, screen) \
    __AROS_LC2(void, UnlockPubScreen, \
    __AROS_LCA(UBYTE         *, name, A0), \
    __AROS_LCA(struct Screen *, screen, A1), \
    struct IntuitionBase *, IntuitionBase, 86, Intuition)


#endif /* CLIB_INTUITION_PROTOS_H */
