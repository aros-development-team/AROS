#ifndef CLIB_INTUITION_PROTOS_H
#define CLIB_INTUITION_PROTOS_H

/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for Intuition.library
    Lang: english
*/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif
#ifndef INTUITION_INTUITIONBASE_H
#   include <intuition/intuitionbase.h>
#endif
#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif

#ifndef IntuitionBase
extern struct IntuitionBase * IntuitionBase;
#endif

/* Prototypes for stubs in amiga.lib */
IPTR DoGadgetMethod (struct Gadget * gad, struct Window * win,	
		    struct Requester * req, ULONG MethodID, ...);
ULONG SetGadgetAttrs (struct Gadget * gad, struct Window * win,
		    struct Requester * req, ULONG tag1, ...);

#ifndef CLIB_BOOPSI_PROTOS_H /* Prevent name clashes */
ULONG SetAttrs (APTR obj, ULONG tag1, ...);
ULONG SetSuperAttrs (Class * cl, Object * obj, ULONG tag1, ...);
APTR NewObject (Class * classPtr, UBYTE * classID, ULONG tag1, ...);
IPTR DoMethodA (Object * obj, Msg message);
IPTR DoMethod (Object * obj, ULONG MethodID, ...);
IPTR DoSuperMethodA (Class * cl, Object * obj, Msg message);
IPTR DoSuperMethod (Class * cl, Object * obj, ULONG MethodID, ...);
IPTR CoerceMethodA (Class * cl, Object * obj, Msg message);
IPTR CoerceMethod (Class * cl, Object * obj, ULONG MethodID, ...);
IPTR CallHookA (struct Hook * hook, APTR obj, APTR param);
IPTR CallHook (struct Hook * hook, APTR obj, ...);
#endif

struct Window * OpenWindowTags (struct NewWindow * newWindow, ULONG tag1, ...);
struct Screen * OpenScreenTags (struct NewScreen * newScreen, ULONG tag1, ...);

LONG EasyRequest (struct Window * window, struct EasyStruct * easyStruct, ULONG * idcmpPtr, ...);


/* Prototypes */
AROS_LP1(void, ActivateWindow,
	AROS_LPA(struct Window *, window, A0),
	struct IntuitionBase *, IntuitionBase, 75, Intuition)

AROS_LP1(void, AddClass,
	AROS_LPA(struct IClass *, classPtr, A0),
	struct IntuitionBase *, IntuitionBase, 114, Intuition)

AROS_LP3(UWORD, AddGadget,
	AROS_LPA(struct Window *, window, A0),
	AROS_LPA(struct Gadget *, gadget, A1),
	AROS_LPA(ULONG          , position, D0),
	struct IntuitionBase *, IntuitionBase, 7, Intuition)

AROS_LP5(UWORD, AddGList,
	AROS_LPA(struct Window    *, window, A0),
	AROS_LPA(struct Gadget    *, gadget, A1),
	AROS_LPA(ULONG             , position, D0),
	AROS_LPA(LONG              , numGad, D1),
	AROS_LPA(struct Requester *, requester, A2),
	struct IntuitionBase *, IntuitionBase, 73, Intuition)

AROS_LP3(APTR, AllocRemember,
	AROS_LPA(struct Remember **, rememberKey, A0),
	AROS_LPA(ULONG             , size, D0),
	AROS_LPA(ULONG             , flags, D1),
	struct IntuitionBase *, IntuitionBase, 66, Intuition)

AROS_LP8(BOOL, AutoRequest,
	AROS_LPA(struct Window    *, window, A0),
	AROS_LPA(struct IntuiText *, body, A1),
	AROS_LPA(struct IntuiText *, posText, A2),
	AROS_LPA(struct IntuiText *, negText, A3),
	AROS_LPA(ULONG             , pFlag, D0),
	AROS_LPA(ULONG             , nFlag, D1),
	AROS_LPA(ULONG             , width, D2),
	AROS_LPA(ULONG             , height, D3),
	struct IntuitionBase *, IntuitionBase, 58, Intuition)

AROS_LP1(void, BeginRefresh,
	AROS_LPA(struct Window *, window, A0),
	struct IntuitionBase *, IntuitionBase, 59, Intuition)

AROS_LP4(struct Window *, BuildEasyRequestArgs,
	AROS_LPA(struct Window     *, RefWindow, A0),
	AROS_LPA(struct EasyStruct *, easyStruct, A1),
	AROS_LPA(ULONG              , IDCMP, D0),
	AROS_LPA(APTR               , Args, A3),
	struct IntuitionBase *, IntuitionBase, 99, Intuition)

AROS_LP5(void, ChangeWindowBox,
	AROS_LPA(struct Window *, window, A0),
	AROS_LPA(LONG           , left, D0),
	AROS_LPA(LONG           , top, D1),
	AROS_LPA(LONG           , width, D2),
	AROS_LPA(LONG           , height, D3),
	struct IntuitionBase *, IntuitionBase, 81, Intuition)

AROS_LP1(void, ClearMenuStrip,
	AROS_LPA(struct Window *, window, A0),
	struct IntuitionBase *, IntuitionBase, 9, Intuition)

AROS_LP1(BOOL, CloseScreen,
	AROS_LPA(struct Screen *, screen, A0),
	struct IntuitionBase *, IntuitionBase, 11, Intuition)

AROS_LP1(void, CloseWindow,
	AROS_LPA(struct Window *, window, A0),
	struct IntuitionBase *, IntuitionBase, 12, Intuition)

AROS_LP2(void, CurrentTime,
	AROS_LPA(ULONG *, seconds, A0),
	AROS_LPA(ULONG *, micros, A1),
	struct IntuitionBase *, IntuitionBase, 14, Intuition)

AROS_LP1(void, DisposeObject,
	AROS_LPA(APTR, object, A0),
	struct IntuitionBase *, IntuitionBase, 107, Intuition)

AROS_LP4(IPTR, DoGadgetMethodA,
	AROS_LPA(struct Gadget    *, gad, A0),
	AROS_LPA(struct Window    *, win, A1),
	AROS_LPA(struct Requester *, req, A2),
	AROS_LPA(Msg               , msg, A3),
	struct IntuitionBase *, IntuitionBase, 135, Intuition)

AROS_LP4(BOOL, DoubleClick,
	AROS_LPA(ULONG, sSeconds, D0),
	AROS_LPA(ULONG, sMicros, D1),
	AROS_LPA(ULONG, cSeconds, D2),
	AROS_LPA(ULONG, cMicros, D3),
	struct IntuitionBase *, IntuitionBase, 17, Intuition)

AROS_LP4(void, DrawBorder,
	AROS_LPA(struct RastPort *, rp, A0),
	AROS_LPA(struct Border   *, border, A1),
	AROS_LPA(LONG             , leftOffset, D0),
	AROS_LPA(LONG             , topOffset, D1),
	struct IntuitionBase *, IntuitionBase, 18, Intuition)

AROS_LP4(void, DrawImage,
	AROS_LPA(struct RastPort *, rp, A0),
	AROS_LPA(struct Image    *, image, A1),
	AROS_LPA(LONG             , leftOffset, D0),
	AROS_LPA(LONG             , topOffset, D1),
	struct IntuitionBase *, IntuitionBase, 19, Intuition)

AROS_LP6(void, DrawImageState,
	AROS_LPA(struct RastPort *, rp,         A0),
	AROS_LPA(struct Image    *, image,      A1),
	AROS_LPA(LONG             , leftOffset, D0),
	AROS_LPA(LONG             , topOffset,  D1),
	AROS_LPA(ULONG            , state,      D2),
	AROS_LPA(struct DrawInfo *, drawInfo,   A2),
	struct IntuitionBase *, IntuitionBase, 103, Intuition)

AROS_LP4(LONG, EasyRequestArgs,
	AROS_LPA(struct Window     *, window, A0),
	AROS_LPA(struct EasyStruct *, easyStruct, A1),
	AROS_LPA(ULONG             *, IDCMP_ptr, A2),
	AROS_LPA(APTR               , argList, A3),
	struct IntuitionBase *, IntuitionBase, 98, Intuition)

AROS_LP2(void, EndRefresh,
	AROS_LPA(struct Window *, window, A0),
	AROS_LPA(BOOL           , complete, D0),
	struct IntuitionBase *, IntuitionBase, 61, Intuition)

AROS_LP4(void, EraseImage,
	AROS_LPA(struct RastPort *, rp, A0),
	AROS_LPA(struct Image    *, image, A1),
	AROS_LPA(LONG             , leftOffset, D0),
	AROS_LPA(LONG             , topOffset, D1),
	struct IntuitionBase *, IntuitionBase, 105, Intuition)

AROS_LP1(BOOL, FreeClass,
	AROS_LPA(struct IClass *, classPtr, A0),
	struct IntuitionBase *, IntuitionBase, 119, Intuition)

AROS_LP2(void, FreeRemember,
	AROS_LPA(struct Remember **, rememberKey, A0),
	AROS_LPA(LONG              , reallyForget, D0),
	struct IntuitionBase *, IntuitionBase, 68, Intuition)

AROS_LP2(void, FreeScreenDrawInfo,
	AROS_LPA(struct Screen   *, screen, A0),
	AROS_LPA(struct DrawInfo *, drawInfo, A1),
	struct IntuitionBase *, IntuitionBase, 116, Intuition)

AROS_LP1(void, FreeSysRequest,
	AROS_LPA(struct Window *, window, A0),
	struct IntuitionBase *, IntuitionBase, 62, Intuition)

AROS_LP3(ULONG, GetAttr,
	AROS_LPA(ULONG   , attrID, D0),
	AROS_LPA(Object *, object, A0),
	AROS_LPA(IPTR *  , storagePtr, A1),
	struct IntuitionBase *, IntuitionBase, 109, Intuition)

AROS_LP1(void, GetDefaultPubScreen,
	AROS_LPA(UBYTE *, nameBuffer, A0),
	struct IntuitionBase *, IntuitionBase, 97, Intuition)

AROS_LP4(LONG, GetScreenData,
	AROS_LPA(APTR           , buffer, A0),
	AROS_LPA(ULONG          , size, D0),
	AROS_LPA(ULONG          , type, D1),
	AROS_LPA(struct Screen *, screen, A1),
	struct IntuitionBase *, IntuitionBase, 71, Intuition)

AROS_LP1(struct DrawInfo *, GetScreenDrawInfo,
	AROS_LPA(struct Screen *, screen, A0),
	struct IntuitionBase *, IntuitionBase, 115, Intuition)

AROS_LP1(ULONG, LockIBase,
	AROS_LPA(ULONG, What, D0),
	struct IntuitionBase *, IntuitionBase, 69, Intuition)

AROS_LP1(struct Screen *, LockPubScreen,
	AROS_LPA(UBYTE *, name, A0),
	struct IntuitionBase *, IntuitionBase, 85, Intuition)

AROS_LP5(struct IClass *, MakeClass,
	AROS_LPA(UBYTE         *, classID, A0),
	AROS_LPA(UBYTE         *, superClassID, A1),
	AROS_LPA(struct IClass *, superClassPtr, A2),
	AROS_LPA(ULONG          , instanceSize, D0),
	AROS_LPA(ULONG          , flags, D1),
	struct IntuitionBase *, IntuitionBase, 113, Intuition)

AROS_LP2(BOOL, ModifyIDCMP,
	AROS_LPA(struct Window *, window, A0),
	AROS_LPA(ULONG          , flags, D0),
	struct IntuitionBase *, IntuitionBase, 25, Intuition)

AROS_LP8(void, ModifyProp,
	AROS_LPA(struct Gadget    *, gadget, A0),
	AROS_LPA(struct Window    *, window, A1),
	AROS_LPA(struct Requester *, requester, A2),
	AROS_LPA(ULONG             , flags, D0),
	AROS_LPA(ULONG             , horizPot, D1),
	AROS_LPA(ULONG             , vertPot, D2),
	AROS_LPA(ULONG             , horizBody, D3),
	AROS_LPA(ULONG             , vertBody, D4),
	struct IntuitionBase *, IntuitionBase, 26, Intuition)

AROS_LP3(void, MoveScreen,
	AROS_LPA(struct Screen *, screen, A0),
	AROS_LPA(LONG           , dx, D0),
	AROS_LPA(LONG           , dy, D1),
	struct IntuitionBase *, IntuitionBase, 27, Intuition)

AROS_LP3(void, MoveWindow,
	AROS_LPA(struct Window *, window, A0),
	AROS_LPA(LONG           , dx, D0),
	AROS_LPA(LONG           , dy, D1),
	struct IntuitionBase *, IntuitionBase, 28, Intuition)

AROS_LP1(APTR, NextObject,
	AROS_LPA(APTR, objectPtrPtr, A0),
	struct IntuitionBase *, IntuitionBase, 111, Intuition)

AROS_LP9(void, NewModifyProp,
	AROS_LPA(struct Gadget    *, gadget, A0),
	AROS_LPA(struct Window    *, window, A1),
	AROS_LPA(struct Requester *, requester, A2),
	AROS_LPA(ULONG             , flags, D0),
	AROS_LPA(ULONG             , horizPot, D1),
	AROS_LPA(ULONG             , vertPot, D2),
	AROS_LPA(ULONG             , horizBody, D3),
	AROS_LPA(ULONG             , vertBody, D4),
	AROS_LPA(LONG              , numGad, D5),
	struct IntuitionBase *, IntuitionBase, 78, Intuition)

AROS_LP3(APTR, NewObjectA,
	AROS_LPA(struct IClass  *, classPtr, A0),
	AROS_LPA(UBYTE          *, classID, A1),
	AROS_LPA(struct TagItem *, tagList, A2),
	struct IntuitionBase *, IntuitionBase, 106, Intuition)

AROS_LP1(struct RastPort *, ObtainGIRPort,
	AROS_LPA(struct GadgetInfo *, gInfo, A0),
	struct IntuitionBase *, IntuitionBase, 93, Intuition)

AROS_LP3(void, OffGadget,
	AROS_LPA(struct Gadget    *, gadget, A0),
	AROS_LPA(struct Window    *, window, A1),
	AROS_LPA(struct Requester *, requester, A2),
	struct IntuitionBase *, IntuitionBase, 29, Intuition)

AROS_LP3(void, OnGadget,
	AROS_LPA(struct Gadget    *, gadget, A0),
	AROS_LPA(struct Window    *, window, A1),
	AROS_LPA(struct Requester *, requester, A2),
	struct IntuitionBase *, IntuitionBase, 31, Intuition)

AROS_LP1(struct Screen *, OpenScreen,
	AROS_LPA(struct NewScreen *, newScreen, A0),
	struct IntuitionBase *, IntuitionBase, 33, Intuition)

AROS_LP2(struct Screen *, OpenScreenTagList,
	AROS_LPA(struct NewScreen *, newScreen, A0),
	AROS_LPA(struct TagItem   *, tagList, A1),
	struct IntuitionBase *, IntuitionBase, 102, Intuition)

AROS_LP1(struct Window *, OpenWindow,
	AROS_LPA(struct NewWindow *, newWindow, A0),
	struct IntuitionBase *, IntuitionBase, 34, Intuition)

AROS_LP2(struct Window *, OpenWindowTagList,
	AROS_LPA(struct NewWindow *, newWindow, A0),
	AROS_LPA(struct TagItem   *, tagList, A1),
	struct IntuitionBase *, IntuitionBase, 101, Intuition)

AROS_LP2(BOOL, PointInImage,
	AROS_LPA(ULONG,          point, D0),
	AROS_LPA(struct Image *, image, A0),
	struct IntuitionBase *, IntuitionBase, 104, Intuition)

AROS_LP4(void, PrintIText,
	AROS_LPA(struct RastPort  *, rp, A0),
	AROS_LPA(struct IntuiText *, iText, A1),
	AROS_LPA(LONG              , leftOffset, D0),
	AROS_LPA(LONG              , topOffset, D1),
	struct IntuitionBase *, IntuitionBase, 36, Intuition)

AROS_LP3(void, RefreshGadgets,
	AROS_LPA(struct Gadget    *, gadgets, A0),
	AROS_LPA(struct Window    *, window, A1),
	AROS_LPA(struct Requester *, requester, A2),
	struct IntuitionBase *, IntuitionBase, 37, Intuition)

AROS_LP4(void, RefreshGList,
	AROS_LPA(struct Gadget    *, gadgets, A0),
	AROS_LPA(struct Window    *, window, A1),
	AROS_LPA(struct Requester *, requester, A2),
	AROS_LPA(LONG              , numGad, D0),
	struct IntuitionBase *, IntuitionBase, 72, Intuition)

AROS_LP1(void, RefreshWindowFrame,
	AROS_LPA(struct Window *, window, A0),
	struct IntuitionBase *, IntuitionBase, 76, Intuition)

AROS_LP1(void, ReleaseGIRPort,
	AROS_LPA(struct RastPort *, rp, A0),
	struct IntuitionBase *, IntuitionBase, 94, Intuition)

AROS_LP1(void, RemoveClass,
	AROS_LPA(struct IClass *, classPtr, A0),
	struct IntuitionBase *, IntuitionBase, 118, Intuition)

AROS_LP2(UWORD, RemoveGadget,
	AROS_LPA(struct Window *, window, A0),
	AROS_LPA(struct Gadget *, gadget, A1),
	struct IntuitionBase *, IntuitionBase, 38, Intuition)

AROS_LP3(UWORD, RemoveGList,
	AROS_LPA(struct Window *, remPtr, A0),
	AROS_LPA(struct Gadget *, gadget, A1),
	AROS_LPA(LONG           , numGad, D0),
	struct IntuitionBase *, IntuitionBase, 74, Intuition)

AROS_LP2(void, ReportMouse,
	AROS_LPA(LONG           , flag, D0),
	AROS_LPA(struct Window *, window, A0),
	struct IntuitionBase *, IntuitionBase, 39, Intuition)

AROS_LP1(void, ScreenToBack,
	AROS_LPA(struct Screen *, screen, A0),
	struct IntuitionBase *, IntuitionBase, 41, Intuition)

AROS_LP1(void, ScreenToFront,
	AROS_LPA(struct Screen *, screen, A0),
	struct IntuitionBase *, IntuitionBase, 42, Intuition)

AROS_LP2(ULONG, SetAttrsA,
	AROS_LPA(APTR            , object, A0),
	AROS_LPA(struct TagItem *, tagList, A1),
	struct IntuitionBase *, IntuitionBase, 108, Intuition)

AROS_LP1(void, SetDefaultPubScreen,
	AROS_LPA(UBYTE *, name, A0),
	struct IntuitionBase *, IntuitionBase, 90, Intuition)

AROS_LP1(struct Hook *, SetEditHook,
	AROS_LPA(struct Hook *, hook, A0),
	struct IntuitionBase *, IntuitionBase, 82, Intuition)

AROS_LP4(IPTR, SetGadgetAttrsA,
	AROS_LPA(struct Gadget *,    gadget,    A0),
	AROS_LPA(struct Window *,    window,    A1),
	AROS_LPA(struct Requester *, requester, A2),
	AROS_LPA(struct TagItem *,   tagList,   A3),
	struct IntuitionBase *, IntuitionBase, 110, Intuition)

AROS_LP6(void, SetPointer,
	AROS_LPA(struct Window *, window, A0),
	AROS_LPA(UWORD         *, pointer, A1),
	AROS_LPA(LONG           , height, D0),
	AROS_LPA(LONG           , width, D1),
	AROS_LPA(LONG           , xOffset, D2),
	AROS_LPA(LONG           , yOffset, D3),
	struct IntuitionBase *, IntuitionBase, 45, Intuition)

AROS_LP3(void, SetWindowTitles,
	AROS_LPA(struct Window *, window, A0),
	AROS_LPA(UBYTE         *, windowTitle, A1),
	AROS_LPA(UBYTE         *, screenTitle, A2),
	struct IntuitionBase *, IntuitionBase, 46, Intuition)

AROS_LP3(void, SizeWindow,
	AROS_LPA(struct Window *, window, A0),
	AROS_LPA(LONG           , dx, D0),
	AROS_LPA(LONG           , dy, D1),
	struct IntuitionBase *, IntuitionBase, 48, Intuition)

AROS_LP3(LONG, SysReqHandler,
	AROS_LPA(struct Window  *, window, A0),
	AROS_LPA(ULONG   *,        IDCMPFlagsPtr, A1),
	AROS_LPA(BOOL            , WaitInput, D0),
	struct IntuitionBase *, IntuitionBase, 100, Intuition)

AROS_LP1(void, UnlockIBase,
	AROS_LPA(ULONG, ibLock, A0),
	struct IntuitionBase *, IntuitionBase, 70, Intuition)

AROS_LP2(void, UnlockPubScreen,
	AROS_LPA(UBYTE         *, name, A0),
	AROS_LPA(struct Screen *, screen, A1),
	struct IntuitionBase *, IntuitionBase, 86, Intuition)

AROS_LP0(struct View *, ViewAddress,
	struct IntuitionBase *, IntuitionBase, 49, Intuition)

AROS_LP1(struct ViewPort *, ViewPortAddress,
	AROS_LPA(struct Window *, Window, D0),
	struct IntuitionBase *, IntuitionBase, 50, Intuition)

AROS_LP5(BOOL, WindowLimits,
	AROS_LPA(struct Window *, window, A0),
	AROS_LPA(WORD,            MinWidth, D0),
	AROS_LPA(WORD,            MinHeight, D1),
	AROS_LPA(UWORD,           MaxWidth, D2),
	AROS_LPA(UWORD,           MaxHeight, D3),
	struct IntuitionBase *, IntuitionBase, 53, Intuition)

AROS_LP1(void, WindowToBack,
	AROS_LPA(struct Window *, window, A0),
	struct IntuitionBase *, IntuitionBase, 51, Intuition)

AROS_LP1(void, WindowToFront,
	AROS_LPA(struct Window *, window, A0),
	struct IntuitionBase *, IntuitionBase, 52, Intuition)

#endif /* CLIB_INTUITION_PROTOS_H */
