#ifndef CLIB_INTUITION_PROTOS_H
#define CLIB_INTUITION_PROTOS_H

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP1(void, ActivateWindow,
    AROS_LPA(struct Window *, window, A0),
    struct IntuitionBase *, IntuitionBase, 75, Intuition)
#define ActivateWindow(window) \
    AROS_LC1(void, ActivateWindow, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 75, Intuition)

AROS_LP1(void, AddClass,
    AROS_LPA(struct IClass *, classPtr, A0),
    struct IntuitionBase *, IntuitionBase, 114, Intuition)
#define AddClass(classPtr) \
    AROS_LC1(void, AddClass, \
    AROS_LCA(struct IClass *, classPtr, A0), \
    struct IntuitionBase *, IntuitionBase, 114, Intuition)

AROS_LP8(BOOL, AutoRequest,
    AROS_LPA(struct Window    *, window, A0),
    AROS_LPA(struct IntuiText *, body, A1),
    AROS_LPA(struct IntuiText *, posText, A2),
    AROS_LPA(struct IntuiText *, negText, A3),
    AROS_LPA(unsigned long     , pFlag, D0),
    AROS_LPA(unsigned long     , nFlag, D1),
    AROS_LPA(unsigned long     , width, D2),
    AROS_LPA(unsigned long     , height, D3),
    struct IntuitionBase *, IntuitionBase, 58, Intuition)
#define AutoRequest(window, body, posText, negText, pFlag, nFlag, width, height) \
    AROS_LC8(BOOL, AutoRequest, \
    AROS_LCA(struct Window    *, window, A0), \
    AROS_LCA(struct IntuiText *, body, A1), \
    AROS_LCA(struct IntuiText *, posText, A2), \
    AROS_LCA(struct IntuiText *, negText, A3), \
    AROS_LCA(unsigned long     , pFlag, D0), \
    AROS_LCA(unsigned long     , nFlag, D1), \
    AROS_LCA(unsigned long     , width, D2), \
    AROS_LCA(unsigned long     , height, D3), \
    struct IntuitionBase *, IntuitionBase, 58, Intuition)

AROS_LP1(void, BeginRefresh,
    AROS_LPA(struct Window *, window, A0),
    struct IntuitionBase *, IntuitionBase, 59, Intuition)
#define BeginRefresh(window) \
    AROS_LC1(void, BeginRefresh, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 59, Intuition)

AROS_LP1(void, ClearMenuStrip,
    AROS_LPA(struct Window *, window, A0),
    struct IntuitionBase *, IntuitionBase, 9, Intuition)
#define ClearMenuStrip(window) \
    AROS_LC1(void, ClearMenuStrip, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 9, Intuition)

AROS_LP1(BOOL, CloseScreen,
    AROS_LPA(struct Screen *, screen, A0),
    struct IntuitionBase *, IntuitionBase, 11, Intuition)
#define CloseScreen(screen) \
    AROS_LC1(BOOL, CloseScreen, \
    AROS_LCA(struct Screen *, screen, A0), \
    struct IntuitionBase *, IntuitionBase, 11, Intuition)

AROS_LP1(void, CloseWindow,
    AROS_LPA(struct Window *, window, A0),
    struct IntuitionBase *, IntuitionBase, 12, Intuition)
#define CloseWindow(window) \
    AROS_LC1(void, CloseWindow, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 12, Intuition)

AROS_LP1(void, DisposeObject,
    AROS_LPA(APTR, object, A0),
    struct IntuitionBase *, IntuitionBase, 107, Intuition)
#define DisposeObject(object) \
    AROS_LC1(void, DisposeObject, \
    AROS_LCA(APTR, object, A0), \
    struct IntuitionBase *, IntuitionBase, 107, Intuition)

AROS_LP4(void, DrawBorder,
    AROS_LPA(struct RastPort *, rp, A0),
    AROS_LPA(struct Border   *, border, A1),
    AROS_LPA(long             , leftOffset, D0),
    AROS_LPA(long             , topOffset, D1),
    struct IntuitionBase *, IntuitionBase, 18, Intuition)
#define DrawBorder(rp, border, leftOffset, topOffset) \
    AROS_LC4(void, DrawBorder, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(struct Border   *, border, A1), \
    AROS_LCA(long             , leftOffset, D0), \
    AROS_LCA(long             , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 18, Intuition)

AROS_LP4(void, DrawImage,
    AROS_LPA(struct RastPort *, rp, A0),
    AROS_LPA(struct Image    *, image, A1),
    AROS_LPA(long             , leftOffset, D0),
    AROS_LPA(long             , topOffset, D1),
    struct IntuitionBase *, IntuitionBase, 19, Intuition)
#define DrawImage(rp, image, leftOffset, topOffset) \
    AROS_LC4(void, DrawImage, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(struct Image    *, image, A1), \
    AROS_LCA(long             , leftOffset, D0), \
    AROS_LCA(long             , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 19, Intuition)

AROS_LP6(void, DrawImageState,
    AROS_LPA(struct RastPort *, rp,         A0),
    AROS_LPA(struct Image    *, image,      A1),
    AROS_LPA(long             , leftOffset, D0),
    AROS_LPA(long             , topOffset,  D1),
    AROS_LPA(ULONG            , state,      D2),
    AROS_LPA(struct DrawInfo *, drawInfo,   A2),
    struct IntuitionBase *, IntuitionBase, 103, Intuition)
#define DrawImageState(rp, image, leftOffset, topOffset, state, drawInfo) \
    AROS_LC6(void, DrawImageState, \
    AROS_LCA(struct RastPort *, rp,         A0), \
    AROS_LCA(struct Image    *, image,      A1), \
    AROS_LCA(long             , leftOffset, D0), \
    AROS_LCA(long             , topOffset,  D1), \
    AROS_LCA(ULONG            , state,      D2), \
    AROS_LCA(struct DrawInfo *, drawInfo,   A2), \
    struct IntuitionBase *, IntuitionBase, 103, Intuition)

AROS_LP2(void, EndRefresh,
    AROS_LPA(struct Window *, window, A0),
    AROS_LPA(BOOL           , complete, D0),
    struct IntuitionBase *, IntuitionBase, 61, Intuition)
#define EndRefresh(window, complete) \
    AROS_LC2(void, EndRefresh, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(BOOL           , complete, D0), \
    struct IntuitionBase *, IntuitionBase, 61, Intuition)

AROS_LP4(void, EraseImage,
    AROS_LPA(struct RastPort *, rp, A0),
    AROS_LPA(struct Image    *, image, A1),
    AROS_LPA(long             , leftOffset, D0),
    AROS_LPA(long             , topOffset, D1),
    struct IntuitionBase *, IntuitionBase, 105, Intuition)
#define EraseImage(rp, image, leftOffset, topOffset) \
    AROS_LC4(void, EraseImage, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(struct Image    *, image, A1), \
    AROS_LCA(long             , leftOffset, D0), \
    AROS_LCA(long             , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 105, Intuition)

AROS_LP1(BOOL, FreeClass,
    AROS_LPA(struct IClass *, classPtr, A0),
    struct IntuitionBase *, IntuitionBase, 119, Intuition)
#define FreeClass(classPtr) \
    AROS_LC1(BOOL, FreeClass, \
    AROS_LCA(struct IClass *, classPtr, A0), \
    struct IntuitionBase *, IntuitionBase, 119, Intuition)

AROS_LP2(void, FreeScreenDrawInfo,
    AROS_LPA(struct Screen   *, screen, A0),
    AROS_LPA(struct DrawInfo *, drawInfo, A1),
    struct IntuitionBase *, IntuitionBase, 116, Intuition)
#define FreeScreenDrawInfo(screen, drawInfo) \
    AROS_LC2(void, FreeScreenDrawInfo, \
    AROS_LCA(struct Screen   *, screen, A0), \
    AROS_LCA(struct DrawInfo *, drawInfo, A1), \
    struct IntuitionBase *, IntuitionBase, 116, Intuition)

AROS_LP3(ULONG, GetAttr,
    AROS_LPA(unsigned long, attrID, D0),
    AROS_LPA(APTR         , object, A0),
    AROS_LPA(ULONG       *, storagePtr, A1),
    struct IntuitionBase *, IntuitionBase, 109, Intuition)
#define GetAttr(attrID, object, storagePtr) \
    AROS_LC3(ULONG, GetAttr, \
    AROS_LCA(unsigned long, attrID, D0), \
    AROS_LCA(APTR         , object, A0), \
    AROS_LCA(ULONG       *, storagePtr, A1), \
    struct IntuitionBase *, IntuitionBase, 109, Intuition)

AROS_LP1(void, GetDefaultPubScreen,
    AROS_LPA(UBYTE *, nameBuffer, A0),
    struct IntuitionBase *, IntuitionBase, 97, Intuition)
#define GetDefaultPubScreen(nameBuffer) \
    AROS_LC1(void, GetDefaultPubScreen, \
    AROS_LCA(UBYTE *, nameBuffer, A0), \
    struct IntuitionBase *, IntuitionBase, 97, Intuition)

AROS_LP4(LONG, GetScreenData,
    AROS_LPA(APTR           , buffer, A0),
    AROS_LPA(unsigned long  , size, D0),
    AROS_LPA(unsigned long  , type, D1),
    AROS_LPA(struct Screen *, screen, A1),
    struct IntuitionBase *, IntuitionBase, 71, Intuition)
#define GetScreenData(buffer, size, type, screen) \
    AROS_LC4(LONG, GetScreenData, \
    AROS_LCA(APTR           , buffer, A0), \
    AROS_LCA(unsigned long  , size, D0), \
    AROS_LCA(unsigned long  , type, D1), \
    AROS_LCA(struct Screen *, screen, A1), \
    struct IntuitionBase *, IntuitionBase, 71, Intuition)

AROS_LP1(struct DrawInfo *, GetScreenDrawInfo,
    AROS_LPA(struct Screen *, screen, A0),
    struct IntuitionBase *, IntuitionBase, 115, Intuition)
#define GetScreenDrawInfo(screen) \
    AROS_LC1(struct DrawInfo *, GetScreenDrawInfo, \
    AROS_LCA(struct Screen *, screen, A0), \
    struct IntuitionBase *, IntuitionBase, 115, Intuition)

AROS_LP1(LONG, IntuiTextLength,
    AROS_LPA(struct IntuiText *, iText, A0),
    struct IntuitionBase *, IntuitionBase, 55, Intuition)
#define IntuiTextLength(iText) \
    AROS_LC1(LONG, IntuiTextLength, \
    AROS_LCA(struct IntuiText *, iText, A0), \
    struct IntuitionBase *, IntuitionBase, 55, Intuition)

AROS_LP1(ULONG, LockIBase,
    AROS_LPA(ULONG, What, D0),
    struct IntuitionBase *, IntuitionBase, 69, Intuition)
#define LockIBase(What) \
    AROS_LC1(ULONG, LockIBase, \
    AROS_LCA(ULONG, What, D0), \
    struct IntuitionBase *, IntuitionBase, 69, Intuition)

AROS_LP1(struct Screen *, LockPubScreen,
    AROS_LPA(UBYTE *, name, A0),
    struct IntuitionBase *, IntuitionBase, 85, Intuition)
#define LockPubScreen(name) \
    AROS_LC1(struct Screen *, LockPubScreen, \
    AROS_LCA(UBYTE *, name, A0), \
    struct IntuitionBase *, IntuitionBase, 85, Intuition)

AROS_LP5(struct IClass *, MakeClass,
    AROS_LPA(UBYTE         *, classID, A0),
    AROS_LPA(UBYTE         *, superClassID, A1),
    AROS_LPA(struct IClass *, superClassPtr, A2),
    AROS_LPA(unsigned long  , instanceSize, D0),
    AROS_LPA(unsigned long  , flags, D1),
    struct IntuitionBase *, IntuitionBase, 113, Intuition)
#define MakeClass(classID, superClassID, superClassPtr, instanceSize, flags) \
    AROS_LC5(struct IClass *, MakeClass, \
    AROS_LCA(UBYTE         *, classID, A0), \
    AROS_LCA(UBYTE         *, superClassID, A1), \
    AROS_LCA(struct IClass *, superClassPtr, A2), \
    AROS_LCA(unsigned long  , instanceSize, D0), \
    AROS_LCA(unsigned long  , flags, D1), \
    struct IntuitionBase *, IntuitionBase, 113, Intuition)

AROS_LP2(BOOL, ModifyIDCMP,
    AROS_LPA(struct Window *, window, A0),
    AROS_LPA(unsigned long  , flags, D0),
    struct IntuitionBase *, IntuitionBase, 25, Intuition)
#define ModifyIDCMP(window, flags) \
    AROS_LC2(BOOL, ModifyIDCMP, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(unsigned long  , flags, D0), \
    struct IntuitionBase *, IntuitionBase, 25, Intuition)

AROS_LP8(void, ModifyProp,
    AROS_LPA(struct Gadget    *, gadget, A0),
    AROS_LPA(struct Window    *, window, A1),
    AROS_LPA(struct Requester *, requester, A2),
    AROS_LPA(unsigned long     , flags, D0),
    AROS_LPA(unsigned long     , horizPot, D1),
    AROS_LPA(unsigned long     , vertPot, D2),
    AROS_LPA(unsigned long     , horizBody, D3),
    AROS_LPA(unsigned long     , vertBody, D4),
    struct IntuitionBase *, IntuitionBase, 26, Intuition)
#define ModifyProp(gadget, window, requester, flags, horizPot, vertPot, horizBody, vertBody) \
    AROS_LC8(void, ModifyProp, \
    AROS_LCA(struct Gadget    *, gadget, A0), \
    AROS_LCA(struct Window    *, window, A1), \
    AROS_LCA(struct Requester *, requester, A2), \
    AROS_LCA(unsigned long     , flags, D0), \
    AROS_LCA(unsigned long     , horizPot, D1), \
    AROS_LCA(unsigned long     , vertPot, D2), \
    AROS_LCA(unsigned long     , horizBody, D3), \
    AROS_LCA(unsigned long     , vertBody, D4), \
    struct IntuitionBase *, IntuitionBase, 26, Intuition)

AROS_LP9(void, NewModifyProp,
    AROS_LPA(struct Gadget    *, gadget, A0),
    AROS_LPA(struct Window    *, window, A1),
    AROS_LPA(struct Requester *, requester, A2),
    AROS_LPA(unsigned long     , flags, D0),
    AROS_LPA(unsigned long     , horizPot, D1),
    AROS_LPA(unsigned long     , vertPot, D2),
    AROS_LPA(unsigned long     , horizBody, D3),
    AROS_LPA(unsigned long     , vertBody, D4),
    AROS_LPA(long              , numGad, D5),
    struct IntuitionBase *, IntuitionBase, 78, Intuition)
#define NewModifyProp(gadget, window, requester, flags, horizPot, vertPot, horizBody, vertBody, numGad) \
    AROS_LC9(void, NewModifyProp, \
    AROS_LCA(struct Gadget    *, gadget, A0), \
    AROS_LCA(struct Window    *, window, A1), \
    AROS_LCA(struct Requester *, requester, A2), \
    AROS_LCA(unsigned long     , flags, D0), \
    AROS_LCA(unsigned long     , horizPot, D1), \
    AROS_LCA(unsigned long     , vertPot, D2), \
    AROS_LCA(unsigned long     , horizBody, D3), \
    AROS_LCA(unsigned long     , vertBody, D4), \
    AROS_LCA(long              , numGad, D5), \
    struct IntuitionBase *, IntuitionBase, 78, Intuition)

AROS_LP3(APTR, NewObjectA,
    AROS_LPA(struct IClass  *, classPtr, A0),
    AROS_LPA(UBYTE          *, classID, A1),
    AROS_LPA(struct TagItem *, tagList, A2),
    struct IntuitionBase *, IntuitionBase, 106, Intuition)
#define NewObjectA(classPtr, classID, tagList) \
    AROS_LC3(APTR, NewObjectA, \
    AROS_LCA(struct IClass  *, classPtr, A0), \
    AROS_LCA(UBYTE          *, classID, A1), \
    AROS_LCA(struct TagItem *, tagList, A2), \
    struct IntuitionBase *, IntuitionBase, 106, Intuition)

AROS_LP1(struct Screen *, OpenScreen,
    AROS_LPA(struct NewScreen *, newScreen, A0),
    struct IntuitionBase *, IntuitionBase, 33, Intuition)
#define OpenScreen(newScreen) \
    AROS_LC1(struct Screen *, OpenScreen, \
    AROS_LCA(struct NewScreen *, newScreen, A0), \
    struct IntuitionBase *, IntuitionBase, 33, Intuition)

AROS_LP2(struct Screen *, OpenScreenTagList,
    AROS_LPA(struct NewScreen *, newScreen, A0),
    AROS_LPA(struct TagItem   *, tagList, A1),
    struct IntuitionBase *, IntuitionBase, 102, Intuition)
#define OpenScreenTagList(newScreen, tagList) \
    AROS_LC2(struct Screen *, OpenScreenTagList, \
    AROS_LCA(struct NewScreen *, newScreen, A0), \
    AROS_LCA(struct TagItem   *, tagList, A1), \
    struct IntuitionBase *, IntuitionBase, 102, Intuition)

AROS_LP1(struct Window *, OpenWindow,
    AROS_LPA(struct NewWindow *, newWindow, A0),
    struct IntuitionBase *, IntuitionBase, 34, Intuition)
#define OpenWindow(newWindow) \
    AROS_LC1(struct Window *, OpenWindow, \
    AROS_LCA(struct NewWindow *, newWindow, A0), \
    struct IntuitionBase *, IntuitionBase, 34, Intuition)

AROS_LP2(struct Window *, OpenWindowTagList,
    AROS_LPA(struct NewWindow *, newWindow, A0),
    AROS_LPA(struct TagItem   *, tagList, A1),
    struct IntuitionBase *, IntuitionBase, 101, Intuition)
#define OpenWindowTagList(newWindow, tagList) \
    AROS_LC2(struct Window *, OpenWindowTagList, \
    AROS_LCA(struct NewWindow *, newWindow, A0), \
    AROS_LCA(struct TagItem   *, tagList, A1), \
    struct IntuitionBase *, IntuitionBase, 101, Intuition)

AROS_LP2(BOOL, PointInImage,
    AROS_LPA(ULONG,          point, D0),
    AROS_LPA(struct Image *, image, A0),
    struct IntuitionBase *, IntuitionBase, 104, Intuition)
#define PointInImage(point, image) \
    AROS_LC2(BOOL, PointInImage, \
    AROS_LCA(ULONG,          point, D0), \
    AROS_LCA(struct Image *, image, A0), \
    struct IntuitionBase *, IntuitionBase, 104, Intuition)

AROS_LP4(void, PrintIText,
    AROS_LPA(struct RastPort  *, rp, A0),
    AROS_LPA(struct IntuiText *, iText, A1),
    AROS_LPA(long              , leftOffset, D0),
    AROS_LPA(long              , topOffset, D1),
    struct IntuitionBase *, IntuitionBase, 36, Intuition)
#define PrintIText(rp, iText, leftOffset, topOffset) \
    AROS_LC4(void, PrintIText, \
    AROS_LCA(struct RastPort  *, rp, A0), \
    AROS_LCA(struct IntuiText *, iText, A1), \
    AROS_LCA(long              , leftOffset, D0), \
    AROS_LCA(long              , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 36, Intuition)

AROS_LP3(void, RefreshGadgets,
    AROS_LPA(struct Gadget    *, gadgets, A0),
    AROS_LPA(struct Window    *, window, A1),
    AROS_LPA(struct Requester *, requester, A2),
    struct IntuitionBase *, IntuitionBase, 37, Intuition)
#define RefreshGadgets(gadgets, window, requester) \
    AROS_LC3(void, RefreshGadgets, \
    AROS_LCA(struct Gadget    *, gadgets, A0), \
    AROS_LCA(struct Window    *, window, A1), \
    AROS_LCA(struct Requester *, requester, A2), \
    struct IntuitionBase *, IntuitionBase, 37, Intuition)

AROS_LP4(void, RefreshGList,
    AROS_LPA(struct Gadget    *, gadgets, A0),
    AROS_LPA(struct Window    *, window, A1),
    AROS_LPA(struct Requester *, requester, A2),
    AROS_LPA(long              , numGad, D0),
    struct IntuitionBase *, IntuitionBase, 72, Intuition)
#define RefreshGList(gadgets, window, requester, numGad) \
    AROS_LC4(void, RefreshGList, \
    AROS_LCA(struct Gadget    *, gadgets, A0), \
    AROS_LCA(struct Window    *, window, A1), \
    AROS_LCA(struct Requester *, requester, A2), \
    AROS_LCA(long              , numGad, D0), \
    struct IntuitionBase *, IntuitionBase, 72, Intuition)

AROS_LP1(void, RefreshWindowFrame,
    AROS_LPA(struct Window *, window, A0),
    struct IntuitionBase *, IntuitionBase, 76, Intuition)
#define RefreshWindowFrame(window) \
    AROS_LC1(void, RefreshWindowFrame, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 76, Intuition)

AROS_LP1(void, RemoveClass,
    AROS_LPA(struct IClass *, classPtr, A0),
    struct IntuitionBase *, IntuitionBase, 118, Intuition)
#define RemoveClass(classPtr) \
    AROS_LC1(void, RemoveClass, \
    AROS_LCA(struct IClass *, classPtr, A0), \
    struct IntuitionBase *, IntuitionBase, 118, Intuition)

AROS_LP2(ULONG, SetAttrsA,
    AROS_LPA(APTR            , object, A0),
    AROS_LPA(struct TagItem *, tagList, A1),
    struct IntuitionBase *, IntuitionBase, 108, Intuition)
#define SetAttrsA(object, tagList) \
    AROS_LC2(ULONG, SetAttrsA, \
    AROS_LCA(APTR            , object, A0), \
    AROS_LCA(struct TagItem *, tagList, A1), \
    struct IntuitionBase *, IntuitionBase, 108, Intuition)

AROS_LP1(void, SetDefaultPubScreen,
    AROS_LPA(UBYTE *, name, A0),
    struct IntuitionBase *, IntuitionBase, 90, Intuition)
#define SetDefaultPubScreen(name) \
    AROS_LC1(void, SetDefaultPubScreen, \
    AROS_LCA(UBYTE *, name, A0), \
    struct IntuitionBase *, IntuitionBase, 90, Intuition)

AROS_LP3(void, SetWindowTitles,
    AROS_LPA(struct Window *, window, A0),
    AROS_LPA(UBYTE         *, windowTitle, A1),
    AROS_LPA(UBYTE         *, screenTitle, A2),
    struct IntuitionBase *, IntuitionBase, 46, Intuition)
#define SetWindowTitles(window, windowTitle, screenTitle) \
    AROS_LC3(void, SetWindowTitles, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(UBYTE         *, windowTitle, A1), \
    AROS_LCA(UBYTE         *, screenTitle, A2), \
    struct IntuitionBase *, IntuitionBase, 46, Intuition)

AROS_LP3(void, SizeWindow,
    AROS_LPA(struct Window *, window, A0),
    AROS_LPA(long           , dx, D0),
    AROS_LPA(long           , dy, D1),
    struct IntuitionBase *, IntuitionBase, 48, Intuition)
#define SizeWindow(window, dx, dy) \
    AROS_LC3(void, SizeWindow, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(long           , dx, D0), \
    AROS_LCA(long           , dy, D1), \
    struct IntuitionBase *, IntuitionBase, 48, Intuition)

AROS_LP1(void, UnlockIBase,
    AROS_LPA(ULONG, ibLock, A0),
    struct IntuitionBase *, IntuitionBase, 70, Intuition)
#define UnlockIBase(ibLock) \
    AROS_LC1(void, UnlockIBase, \
    AROS_LCA(ULONG, ibLock, A0), \
    struct IntuitionBase *, IntuitionBase, 70, Intuition)

AROS_LP2(void, UnlockPubScreen,
    AROS_LPA(UBYTE         *, name, A0),
    AROS_LPA(struct Screen *, screen, A1),
    struct IntuitionBase *, IntuitionBase, 86, Intuition)
#define UnlockPubScreen(name, screen) \
    AROS_LC2(void, UnlockPubScreen, \
    AROS_LCA(UBYTE         *, name, A0), \
    AROS_LCA(struct Screen *, screen, A1), \
    struct IntuitionBase *, IntuitionBase, 86, Intuition)

AROS_LP1(void, WindowToBack,
    AROS_LPA(struct Window *, window, A0),
    struct IntuitionBase *, IntuitionBase, 51, Intuition)
#define WindowToBack(window) \
    AROS_LC1(void, WindowToBack, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 51, Intuition)

AROS_LP1(void, WindowToFront,
    AROS_LPA(struct Window *, window, A0),
    struct IntuitionBase *, IntuitionBase, 52, Intuition)
#define WindowToFront(window) \
    AROS_LC1(void, WindowToFront, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 52, Intuition)


#endif /* CLIB_INTUITION_PROTOS_H */
