#ifndef DEFINES_INTUITION_H
#define DEFINES_INTUITION_H

#ifndef  EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define ActivateWindow(window) \
    AROS_LC1(void, ActivateWindow, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 75, Intuition)

#define AddClass(classPtr) \
    AROS_LC1(void, AddClass, \
    AROS_LCA(struct IClass *, classPtr, A0), \
    struct IntuitionBase *, IntuitionBase, 114, Intuition)

#define AddGadget(window, gadget, position) \
    AROS_LC3(UWORD, AddGadget, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(struct Gadget *, gadget, A1), \
    AROS_LCA(ULONG          , position, D0), \
    struct IntuitionBase *, IntuitionBase, 7, Intuition)

#define AddGList(window, gadget, position, numGad, requester) \
    AROS_LC5(UWORD, AddGList, \
    AROS_LCA(struct Window    *, window, A0), \
    AROS_LCA(struct Gadget    *, gadget, A1), \
    AROS_LCA(ULONG             , position, D0), \
    AROS_LCA(LONG              , numGad, D1), \
    AROS_LCA(struct Requester *, requester, A2), \
    struct IntuitionBase *, IntuitionBase, 73, Intuition)

#define AutoRequest(window, body, posText, negText, pFlag, nFlag, width, height) \
    AROS_LC8(BOOL, AutoRequest, \
    AROS_LCA(struct Window    *, window, A0), \
    AROS_LCA(struct IntuiText *, body, A1), \
    AROS_LCA(struct IntuiText *, posText, A2), \
    AROS_LCA(struct IntuiText *, negText, A3), \
    AROS_LCA(ULONG             , pFlag, D0), \
    AROS_LCA(ULONG             , nFlag, D1), \
    AROS_LCA(ULONG             , width, D2), \
    AROS_LCA(ULONG             , height, D3), \
    struct IntuitionBase *, IntuitionBase, 58, Intuition)

#define BeginRefresh(window) \
    AROS_LC1(void, BeginRefresh, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 59, Intuition)

#define ChangeWindowBox(window, left, top, width, height) \
    AROS_LC5(void, ChangeWindowBox, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(LONG           , left, D0), \
    AROS_LCA(LONG           , top, D1), \
    AROS_LCA(LONG           , width, D2), \
    AROS_LCA(LONG           , height, D3), \
    struct IntuitionBase *, IntuitionBase, 81, Intuition)

#define ClearMenuStrip(window) \
    AROS_LC1(void, ClearMenuStrip, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 9, Intuition)

#define CloseScreen(screen) \
    AROS_LC1(BOOL, CloseScreen, \
    AROS_LCA(struct Screen *, screen, A0), \
    struct IntuitionBase *, IntuitionBase, 11, Intuition)

#define CloseWindow(window) \
    AROS_LC1(void, CloseWindow, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 12, Intuition)

#define DisposeObject(object) \
    AROS_LC1(void, DisposeObject, \
    AROS_LCA(APTR, object, A0), \
    struct IntuitionBase *, IntuitionBase, 107, Intuition)

#define DoGadgetMethodA(gad, win, req, msg) \
    AROS_LC4(IPTR, DoGadgetMethodA, \
    AROS_LCA(struct Gadget    *, gad, A0), \
    AROS_LCA(struct Window    *, win, A1), \
    AROS_LCA(struct Requester *, req, A2), \
    AROS_LCA(Msg               , msg, A3), \
    struct IntuitionBase *, IntuitionBase, 135, Intuition)

#define DoubleClick(sSeconds, sMicros, cSeconds, cMicros) \
    AROS_LC4(BOOL, DoubleClick, \
    AROS_LCA(ULONG, sSeconds, D0), \
    AROS_LCA(ULONG, sMicros, D1), \
    AROS_LCA(ULONG, cSeconds, D2), \
    AROS_LCA(ULONG, cMicros, D3), \
    struct IntuitionBase *, IntuitionBase, 17, Intuition)

#define DrawBorder(rp, border, leftOffset, topOffset) \
    AROS_LC4(void, DrawBorder, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(struct Border   *, border, A1), \
    AROS_LCA(LONG             , leftOffset, D0), \
    AROS_LCA(LONG             , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 18, Intuition)

#define DrawImage(rp, image, leftOffset, topOffset) \
    AROS_LC4(void, DrawImage, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(struct Image    *, image, A1), \
    AROS_LCA(LONG             , leftOffset, D0), \
    AROS_LCA(LONG             , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 19, Intuition)

#define DrawImageState(rp, image, leftOffset, topOffset, state, drawInfo) \
    AROS_LC6(void, DrawImageState, \
    AROS_LCA(struct RastPort *, rp,         A0), \
    AROS_LCA(struct Image    *, image,      A1), \
    AROS_LCA(LONG             , leftOffset, D0), \
    AROS_LCA(LONG             , topOffset,  D1), \
    AROS_LCA(ULONG            , state,      D2), \
    AROS_LCA(struct DrawInfo *, drawInfo,   A2), \
    struct IntuitionBase *, IntuitionBase, 103, Intuition)

#define EasyRequestArgs(window, easyStruct, idcmpPtr, args) \
    AROS_LC4(LONG, EasyRequestArgs, \
    AROS_LCA(struct Window     *, window, A0), \
    AROS_LCA(struct EasyStruct *, easyStruct, A1), \
    AROS_LCA(ULONG             *, idcmpPtr, A2), \
    AROS_LCA(APTR               , args, A3), \
    struct IntuitionBase *, IntuitionBase, 98, Intuition)

#define EndRefresh(window, complete) \
    AROS_LC2(void, EndRefresh, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(BOOL           , complete, D0), \
    struct IntuitionBase *, IntuitionBase, 61, Intuition)

#define EraseImage(rp, image, leftOffset, topOffset) \
    AROS_LC4(void, EraseImage, \
    AROS_LCA(struct RastPort *, rp, A0), \
    AROS_LCA(struct Image    *, image, A1), \
    AROS_LCA(LONG             , leftOffset, D0), \
    AROS_LCA(LONG             , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 105, Intuition)

#define FreeClass(classPtr) \
    AROS_LC1(BOOL, FreeClass, \
    AROS_LCA(struct IClass *, classPtr, A0), \
    struct IntuitionBase *, IntuitionBase, 119, Intuition)

#define FreeScreenDrawInfo(screen, drawInfo) \
    AROS_LC2(void, FreeScreenDrawInfo, \
    AROS_LCA(struct Screen   *, screen, A0), \
    AROS_LCA(struct DrawInfo *, drawInfo, A1), \
    struct IntuitionBase *, IntuitionBase, 116, Intuition)

#define GetAttr(attrID, object, storagePtr) \
    AROS_LC3(ULONG, GetAttr, \
    AROS_LCA(ULONG   , attrID, D0), \
    AROS_LCA(Object *, object, A0), \
    AROS_LCA(IPTR *  , storagePtr, A1), \
    struct IntuitionBase *, IntuitionBase, 109, Intuition)

#define GetDefaultPubScreen(nameBuffer) \
    AROS_LC1(void, GetDefaultPubScreen, \
    AROS_LCA(UBYTE *, nameBuffer, A0), \
    struct IntuitionBase *, IntuitionBase, 97, Intuition)

#define GetScreenData(buffer, size, type, screen) \
    AROS_LC4(LONG, GetScreenData, \
    AROS_LCA(APTR           , buffer, A0), \
    AROS_LCA(ULONG          , size, D0), \
    AROS_LCA(ULONG          , type, D1), \
    AROS_LCA(struct Screen *, screen, A1), \
    struct IntuitionBase *, IntuitionBase, 71, Intuition)

#define GetScreenDrawInfo(screen) \
    AROS_LC1(struct DrawInfo *, GetScreenDrawInfo, \
    AROS_LCA(struct Screen *, screen, A0), \
    struct IntuitionBase *, IntuitionBase, 115, Intuition)

#define IntuiTextLength(iText) \
    AROS_LC1(LONG, IntuiTextLength, \
    AROS_LCA(struct IntuiText *, iText, A0), \
    struct IntuitionBase *, IntuitionBase, 55, Intuition)

#define LockIBase(What) \
    AROS_LC1(ULONG, LockIBase, \
    AROS_LCA(ULONG, What, D0), \
    struct IntuitionBase *, IntuitionBase, 69, Intuition)

#define LockPubScreen(name) \
    AROS_LC1(struct Screen *, LockPubScreen, \
    AROS_LCA(UBYTE *, name, A0), \
    struct IntuitionBase *, IntuitionBase, 85, Intuition)

#define MakeClass(classID, superClassID, superClassPtr, instanceSize, flags) \
    AROS_LC5(struct IClass *, MakeClass, \
    AROS_LCA(UBYTE         *, classID, A0), \
    AROS_LCA(UBYTE         *, superClassID, A1), \
    AROS_LCA(struct IClass *, superClassPtr, A2), \
    AROS_LCA(ULONG          , instanceSize, D0), \
    AROS_LCA(ULONG          , flags, D1), \
    struct IntuitionBase *, IntuitionBase, 113, Intuition)

#define ModifyIDCMP(window, flags) \
    AROS_LC2(BOOL, ModifyIDCMP, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(ULONG          , flags, D0), \
    struct IntuitionBase *, IntuitionBase, 25, Intuition)

#define ModifyProp(gadget, window, requester, flags, horizPot, vertPot, horizBody, vertBody) \
    AROS_LC8(void, ModifyProp, \
    AROS_LCA(struct Gadget    *, gadget, A0), \
    AROS_LCA(struct Window    *, window, A1), \
    AROS_LCA(struct Requester *, requester, A2), \
    AROS_LCA(ULONG             , flags, D0), \
    AROS_LCA(ULONG             , horizPot, D1), \
    AROS_LCA(ULONG             , vertPot, D2), \
    AROS_LCA(ULONG             , horizBody, D3), \
    AROS_LCA(ULONG             , vertBody, D4), \
    struct IntuitionBase *, IntuitionBase, 26, Intuition)

#define MoveScreen(screen, dx, dy) \
    AROS_LC3(void, MoveScreen, \
    AROS_LCA(struct Screen *, screen, A0), \
    AROS_LCA(LONG           , dx, D0), \
    AROS_LCA(LONG           , dy, D1), \
    struct IntuitionBase *, IntuitionBase, 27, Intuition)

#define MoveWindow(window, dx, dy) \
    AROS_LC3(void, MoveWindow, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(LONG           , dx, D0), \
    AROS_LCA(LONG           , dy, D1), \
    struct IntuitionBase *, IntuitionBase, 28, Intuition)

#define NewModifyProp(gadget, window, requester, flags, horizPot, vertPot, horizBody, vertBody, numGad) \
    AROS_LC9(void, NewModifyProp, \
    AROS_LCA(struct Gadget    *, gadget, A0), \
    AROS_LCA(struct Window    *, window, A1), \
    AROS_LCA(struct Requester *, requester, A2), \
    AROS_LCA(ULONG             , flags, D0), \
    AROS_LCA(ULONG             , horizPot, D1), \
    AROS_LCA(ULONG             , vertPot, D2), \
    AROS_LCA(ULONG             , horizBody, D3), \
    AROS_LCA(ULONG             , vertBody, D4), \
    AROS_LCA(LONG              , numGad, D5), \
    struct IntuitionBase *, IntuitionBase, 78, Intuition)

#define NewObjectA(classPtr, classID, tagList) \
    AROS_LC3(APTR, NewObjectA, \
    AROS_LCA(struct IClass  *, classPtr, A0), \
    AROS_LCA(UBYTE          *, classID, A1), \
    AROS_LCA(struct TagItem *, tagList, A2), \
    struct IntuitionBase *, IntuitionBase, 106, Intuition)

#define ObtainGIRPort(gInfo) \
    AROS_LC1(struct RastPort *, ObtainGIRPort, \
    AROS_LCA(struct GadgetInfo *, gInfo, A0), \
    struct IntuitionBase *, IntuitionBase, 93, Intuition)

#define OpenScreen(newScreen) \
    AROS_LC1(struct Screen *, OpenScreen, \
    AROS_LCA(struct NewScreen *, newScreen, A0), \
    struct IntuitionBase *, IntuitionBase, 33, Intuition)

#define OpenScreenTagList(newScreen, tagList) \
    AROS_LC2(struct Screen *, OpenScreenTagList, \
    AROS_LCA(struct NewScreen *, newScreen, A0), \
    AROS_LCA(struct TagItem   *, tagList, A1), \
    struct IntuitionBase *, IntuitionBase, 102, Intuition)

#define OpenWindow(newWindow) \
    AROS_LC1(struct Window *, OpenWindow, \
    AROS_LCA(struct NewWindow *, newWindow, A0), \
    struct IntuitionBase *, IntuitionBase, 34, Intuition)

#define OpenWindowTagList(newWindow, tagList) \
    AROS_LC2(struct Window *, OpenWindowTagList, \
    AROS_LCA(struct NewWindow *, newWindow, A0), \
    AROS_LCA(struct TagItem   *, tagList, A1), \
    struct IntuitionBase *, IntuitionBase, 101, Intuition)

#define PointInImage(point, image) \
    AROS_LC2(BOOL, PointInImage, \
    AROS_LCA(ULONG,          point, D0), \
    AROS_LCA(struct Image *, image, A0), \
    struct IntuitionBase *, IntuitionBase, 104, Intuition)

#define PrintIText(rp, iText, leftOffset, topOffset) \
    AROS_LC4(void, PrintIText, \
    AROS_LCA(struct RastPort  *, rp, A0), \
    AROS_LCA(struct IntuiText *, iText, A1), \
    AROS_LCA(LONG              , leftOffset, D0), \
    AROS_LCA(LONG              , topOffset, D1), \
    struct IntuitionBase *, IntuitionBase, 36, Intuition)

#define RefreshGadgets(gadgets, window, requester) \
    AROS_LC3(void, RefreshGadgets, \
    AROS_LCA(struct Gadget    *, gadgets, A0), \
    AROS_LCA(struct Window    *, window, A1), \
    AROS_LCA(struct Requester *, requester, A2), \
    struct IntuitionBase *, IntuitionBase, 37, Intuition)

#define RefreshGList(gadgets, window, requester, numGad) \
    AROS_LC4(void, RefreshGList, \
    AROS_LCA(struct Gadget    *, gadgets, A0), \
    AROS_LCA(struct Window    *, window, A1), \
    AROS_LCA(struct Requester *, requester, A2), \
    AROS_LCA(LONG              , numGad, D0), \
    struct IntuitionBase *, IntuitionBase, 72, Intuition)

#define RefreshWindowFrame(window) \
    AROS_LC1(void, RefreshWindowFrame, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 76, Intuition)

#define ReleaseGIRPort(rp) \
    AROS_LC1(void, ReleaseGIRPort, \
    AROS_LCA(struct RastPort *, rp, A0), \
    struct IntuitionBase *, IntuitionBase, 94, Intuition)

#define RemoveClass(classPtr) \
    AROS_LC1(void, RemoveClass, \
    AROS_LCA(struct IClass *, classPtr, A0), \
    struct IntuitionBase *, IntuitionBase, 118, Intuition)

#define RemoveGadget(window, gadget) \
    AROS_LC2(UWORD, RemoveGadget, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(struct Gadget *, gadget, A1), \
    struct IntuitionBase *, IntuitionBase, 38, Intuition)

#define RemoveGList(remPtr, gadget, numGad) \
    AROS_LC3(UWORD, RemoveGList, \
    AROS_LCA(struct Window *, remPtr, A0), \
    AROS_LCA(struct Gadget *, gadget, A1), \
    AROS_LCA(LONG           , numGad, D0), \
    struct IntuitionBase *, IntuitionBase, 74, Intuition)

#define ReportMouse(flag, window) \
    AROS_LC2(void, ReportMouse, \
    AROS_LCA(LONG           , flag, D0), \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 39, Intuition)

#define ScreenToFront(screen) \
    AROS_LC1(void, ScreenToFront, \
    AROS_LCA(struct Screen *, screen, A0), \
    struct IntuitionBase *, IntuitionBase, 42, Intuition)

#define SetAttrsA(object, tagList) \
    AROS_LC2(ULONG, SetAttrsA, \
    AROS_LCA(APTR            , object, A0), \
    AROS_LCA(struct TagItem *, tagList, A1), \
    struct IntuitionBase *, IntuitionBase, 108, Intuition)

#define SetDefaultPubScreen(name) \
    AROS_LC1(void, SetDefaultPubScreen, \
    AROS_LCA(UBYTE *, name, A0), \
    struct IntuitionBase *, IntuitionBase, 90, Intuition)

#define SetPointer(window, pointer, height, width, xOffset, yOffset) \
    AROS_LC6(void, SetPointer, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(UWORD         *, pointer, A1), \
    AROS_LCA(LONG           , height, D0), \
    AROS_LCA(LONG           , width, D1), \
    AROS_LCA(LONG           , xOffset, D2), \
    AROS_LCA(LONG           , yOffset, D3), \
    struct IntuitionBase *, IntuitionBase, 45, Intuition)

#define SetWindowTitles(window, windowTitle, screenTitle) \
    AROS_LC3(void, SetWindowTitles, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(UBYTE         *, windowTitle, A1), \
    AROS_LCA(UBYTE         *, screenTitle, A2), \
    struct IntuitionBase *, IntuitionBase, 46, Intuition)

#define SizeWindow(window, dx, dy) \
    AROS_LC3(void, SizeWindow, \
    AROS_LCA(struct Window *, window, A0), \
    AROS_LCA(LONG           , dx, D0), \
    AROS_LCA(LONG           , dy, D1), \
    struct IntuitionBase *, IntuitionBase, 48, Intuition)

#define UnlockIBase(ibLock) \
    AROS_LC1(void, UnlockIBase, \
    AROS_LCA(ULONG, ibLock, A0), \
    struct IntuitionBase *, IntuitionBase, 70, Intuition)

#define UnlockPubScreen(name, screen) \
    AROS_LC2(void, UnlockPubScreen, \
    AROS_LCA(UBYTE         *, name, A0), \
    AROS_LCA(struct Screen *, screen, A1), \
    struct IntuitionBase *, IntuitionBase, 86, Intuition)

#define WindowToBack(window) \
    AROS_LC1(void, WindowToBack, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 51, Intuition)

#define WindowToFront(window) \
    AROS_LC1(void, WindowToFront, \
    AROS_LCA(struct Window *, window, A0), \
    struct IntuitionBase *, IntuitionBase, 52, Intuition)


#endif /* DEFINES_INTUITION_H */
