#ifndef _MUI_CLASSES_WINDOW_H
#define _MUI_CLASSES_WINDOW_H

#ifndef _DRAGNDROP_H
#include "dragndrop.h"
#endif

/* this is for the cycle list */
struct ObjNode
{
    struct MinNode node;
    Object *obj;
};

struct MUI_WindowData
{
    struct MUI_RenderInfo wd_RenderInfo;
    struct MUI_MinMax     wd_MinMax;
    struct MsgPort       *wd_UserPort; /* IDCMP port */

    struct IBox    wd_AltDim;       /* zoomed dimensions */
    struct MinList wd_CycleChain;   /* objects activated with tab */
    struct MinList wd_EHList;       /* event handlers */
    struct MinList wd_CCList;       /* control chars */
    ULONG          wd_Events;       /* events received */
    ULONG          wd_CrtFlags;     /* window creation flags, see below */
    struct ObjNode *wd_ActiveObject; /* the active object embeded in the cyclechain */
    APTR           wd_DefaultObject;
    ULONG          wd_ID;
    STRPTR         wd_Title;
    LONG           wd_Height;       /* Current dimensions */
    LONG           wd_Width;
    LONG           wd_X;
    LONG           wd_Y;
    LONG           wd_ReqHeight;    /* given by programmer */
    LONG           wd_ReqWidth;
    APTR           wd_RootObject;   /* unique child */
    ULONG          wd_Flags;        /* various status flags */
    UWORD          wd_innerLeft;
    UWORD          wd_innerRight;
    UWORD          wd_innerTop;
    UWORD          wd_innerBottom;

    Object *       wd_DragObject; /* the object which is being dragged */
    Object *       wd_DropObject; /* the destination object */
    struct DragNDrop *wd_dnd;
    struct MUI_DragImage *wd_DragImage;
};

#ifndef WFLG_SIZEGADGET

#define WFLG_CLOSEGADGET (1<<0) /* has close gadget */
#define WFLG_SIZEGADGET  (1<<1) /* has size gadget */
#define WFLG_BACKDROP    (1<<2) /* is backdrop window */
#define WFLG_BORDERLESS  (1<<3) /* has no borders */
#define WFLG_DEPTHGADGET (1<<4) /* has depth gadget */
#define WFLG_DRAGBAR     (1<<5) /* is draggable */
#define WFLG_SIZEBRIGHT  (1<<6) /* size gadget is in right border */

#endif

/* wd_Flags */
#define MUIWF_OPENED    (1<<0) /* window currently opened */
#define MUIWF_ICONIFIED (1<<1) /* window currently iconified */
#define MUIWF_ACTIVE    (1<<2) /* window currently active */
#define MUIWF_CLOSEREQUESTED (1<<3) /* when user hits close gadget */
#define MUIWF_RESIZING (1<<4) /* window currently resizing, for simple refresh */

struct __dummyXFC3__
{
	struct MUI_NotifyData mnd;
	struct MUI_WindowData mwd;
};

#define muiWindowData(obj)   (&(((struct __dummyXFC3__ *)(obj))->mwd))

#ifdef _DCC
extern char MUIC_Window[];
#else
#define MUIC_Window "Window.mui"
#endif

/* Methods */

#define MUIM_Window_AddEventHandler         0x804203b7 /* V16 */
#ifdef MUI_OBSOLETE
#define MUIM_Window_GetMenuCheck            0x80420414 /* V4  */
#endif /* MUI_OBSOLETE */
#ifdef MUI_OBSOLETE
#define MUIM_Window_GetMenuState            0x80420d2f /* V4  */
#endif /* MUI_OBSOLETE */
#define MUIM_Window_RemEventHandler         0x8042679e /* V16 */
#define MUIM_Window_ScreenToBack            0x8042913d /* V4  */
#define MUIM_Window_ScreenToFront           0x804227a4 /* V4  */
#ifdef MUI_OBSOLETE
#define MUIM_Window_SetCycleChain           0x80426510 /* V4  */
#endif /* MUI_OBSOLETE */
#ifdef MUI_OBSOLETE
#define MUIM_Window_SetMenuCheck            0x80422243 /* V4  */
#endif /* MUI_OBSOLETE */
#ifdef MUI_OBSOLETE
#define MUIM_Window_SetMenuState            0x80422b5e /* V4  */
#endif /* MUI_OBSOLETE */
#define MUIM_Window_Snapshot                0x8042945e /* V11 */
#define MUIM_Window_ToBack                  0x8042152e /* V4  */
#define MUIM_Window_ToFront                 0x8042554f /* V4  */
struct  MUIP_Window_AddEventHandler         { ULONG MethodID; struct MUI_EventHandlerNode *ehnode; };
struct  MUIP_Window_GetMenuCheck            { ULONG MethodID; ULONG MenuID; };
struct  MUIP_Window_GetMenuState            { ULONG MethodID; ULONG MenuID; };
struct  MUIP_Window_RemEventHandler         { ULONG MethodID; struct MUI_EventHandlerNode *ehnode; };
struct  MUIP_Window_ScreenToBack            { ULONG MethodID; };
struct  MUIP_Window_ScreenToFront           { ULONG MethodID; };
struct  MUIP_Window_SetCycleChain           { ULONG MethodID; Object *obj[1]; };
struct  MUIP_Window_SetMenuCheck            { ULONG MethodID; ULONG MenuID; LONG stat; };
struct  MUIP_Window_SetMenuState            { ULONG MethodID; ULONG MenuID; LONG stat; };
struct  MUIP_Window_Snapshot                { ULONG MethodID; LONG flags; };
struct  MUIP_Window_ToBack                  { ULONG MethodID; };
struct  MUIP_Window_ToFront                 { ULONG MethodID; };

/* Attributes */

#define MUIA_Window_Activate                0x80428d2f /* V4  isg BOOL              */
#define MUIA_Window_ActiveObject            0x80427925 /* V4  .sg Object *          */
#define MUIA_Window_AltHeight               0x8042cce3 /* V4  i.g LONG              */
#define MUIA_Window_AltLeftEdge             0x80422d65 /* V4  i.g LONG              */
#define MUIA_Window_AltTopEdge              0x8042e99b /* V4  i.g LONG              */
#define MUIA_Window_AltWidth                0x804260f4 /* V4  i.g LONG              */
#define MUIA_Window_AppWindow               0x804280cf /* V5  i.. BOOL              */
#define MUIA_Window_Backdrop                0x8042c0bb /* V4  i.. BOOL              */
#define MUIA_Window_Borderless              0x80429b79 /* V4  i.. BOOL              */
#define MUIA_Window_CloseGadget             0x8042a110 /* V4  i.. BOOL              */
#define MUIA_Window_CloseRequest            0x8042e86e /* V4  ..g BOOL              */
#define MUIA_Window_DefaultObject           0x804294d7 /* V4  isg Object *          */
#define MUIA_Window_DepthGadget             0x80421923 /* V4  i.. BOOL              */
#define MUIA_Window_DragBar                 0x8042045d /* V4  i.. BOOL              */
#define MUIA_Window_FancyDrawing            0x8042bd0e /* V8  isg BOOL              */
#define MUIA_Window_Height                  0x80425846 /* V4  i.g LONG              */
#define MUIA_Window_ID                      0x804201bd /* V4  isg ULONG             */
#define MUIA_Window_InputEvent              0x804247d8 /* V4  ..g struct InputEvent * */
#define MUIA_Window_IsSubWindow             0x8042b5aa /* V4  isg BOOL              */
#define MUIA_Window_LeftEdge                0x80426c65 /* V4  i.g LONG              */
#ifdef MUI_OBSOLETE
#define MUIA_Window_Menu                    0x8042db94 /* V4  i.. struct NewMenu *  */
#endif /* MUI_OBSOLETE */
#define MUIA_Window_MenuAction              0x80427521 /* V8  isg ULONG             */
#define MUIA_Window_Menustrip               0x8042855e /* V8  i.g Object *          */
#define MUIA_Window_MouseObject             0x8042bf9b /* V10 ..g Object *          */
#define MUIA_Window_NeedsMouseObject        0x8042372a /* V10 i.. BOOL              */
#define MUIA_Window_NoMenus                 0x80429df5 /* V4  is. BOOL              */
#define MUIA_Window_Open                    0x80428aa0 /* V4  .sg BOOL              */
#define MUIA_Window_PublicScreen            0x804278e4 /* V6  isg STRPTR            */
#define MUIA_Window_RefWindow               0x804201f4 /* V4  is. Object *          */
#define MUIA_Window_RootObject              0x8042cba5 /* V4  isg Object *          */
#define MUIA_Window_Screen                  0x8042df4f /* V4  isg struct Screen *   */
#define MUIA_Window_ScreenTitle             0x804234b0 /* V5  isg STRPTR            */
#define MUIA_Window_SizeGadget              0x8042e33d /* V4  i.. BOOL              */
#define MUIA_Window_SizeRight               0x80424780 /* V4  i.. BOOL              */
#define MUIA_Window_Sleep                   0x8042e7db /* V4  .sg BOOL              */
#define MUIA_Window_Title                   0x8042ad3d /* V4  isg STRPTR            */
#define MUIA_Window_TopEdge                 0x80427c66 /* V4  i.g LONG              */
#define MUIA_Window_UseBottomBorderScroller 0x80424e79 /* V13 isg BOOL              */
#define MUIA_Window_UseLeftBorderScroller   0x8042433e /* V13 isg BOOL              */
#define MUIA_Window_UseRightBorderScroller  0x8042c05e /* V13 isg BOOL              */
#define MUIA_Window_Width                   0x8042dcae /* V4  i.g LONG              */
#define MUIA_Window_Window                  0x80426a42 /* V4  ..g struct Window *   */

#define MUIV_Window_ActiveObject_None 0
#define MUIV_Window_ActiveObject_Next -1
#define MUIV_Window_ActiveObject_Prev -2
#define MUIV_Window_AltHeight_MinMax(p) (0-(p))
#define MUIV_Window_AltHeight_Visible(p) (-100-(p))
#define MUIV_Window_AltHeight_Screen(p) (-200-(p))
#define MUIV_Window_AltHeight_Scaled -1000
#define MUIV_Window_AltLeftEdge_Centered -1
#define MUIV_Window_AltLeftEdge_Moused -2
#define MUIV_Window_AltLeftEdge_NoChange -1000
#define MUIV_Window_AltTopEdge_Centered -1
#define MUIV_Window_AltTopEdge_Moused -2
#define MUIV_Window_AltTopEdge_Delta(p) (-3-(p))
#define MUIV_Window_AltTopEdge_NoChange -1000
#define MUIV_Window_AltWidth_MinMax(p) (0-(p))
#define MUIV_Window_AltWidth_Visible(p) (-100-(p))
#define MUIV_Window_AltWidth_Screen(p) (-200-(p))
#define MUIV_Window_AltWidth_Scaled -1000
#define MUIV_Window_Height_MinMax(p) (0-(p))
#define MUIV_Window_Height_Visible(p) (-100-(p))
#define MUIV_Window_Height_Screen(p) (-200-(p))
#define MUIV_Window_Height_Scaled -1000
#define MUIV_Window_Height_Default -1001
#define MUIV_Window_LeftEdge_Centered -1
#define MUIV_Window_LeftEdge_Moused -2
#ifdef MUI_OBSOLETE
#define MUIV_Window_Menu_NoMenu -1
#endif /* MUI_OBSOLETE */
#define MUIV_Window_TopEdge_Centered -1
#define MUIV_Window_TopEdge_Moused -2
#define MUIV_Window_TopEdge_Delta(p) (-3-(p))
#define MUIV_Window_Width_MinMax(p) (0-(p))
#define MUIV_Window_Width_Visible(p) (-100-(p))
#define MUIV_Window_Width_Screen(p) (-200-(p))
#define MUIV_Window_Width_Scaled -1000
#define MUIV_Window_Width_Default -1001

/* Methods */

#define MUIM_Window_ActionIconify 0x80422cc0 /* V18 */
#define MUIM_Window_Cleanup       0x8042ab26 /* Custom Class */ /* V18 */
#define MUIM_Window_Setup         0x8042c34c /* Custom Class */ /* V18 */

#define MUIM_Window_AddControlCharHandler 0x8042c34d
#define MUIM_Window_RemControlCharHandler 0x8042c34e
#define MUIM_Window_DragObject            0x8042c34f /* ZV1 */

struct  MUIP_Window_Cleanup       { ULONG MethodID; }; /* Custom Class */
struct  MUIP_Window_Setup         { ULONG MethodID; }; /* Custom Class */

struct  MUIP_Window_AddControlCharHandler   { ULONG MethodID; struct MUI_EventHandlerNode *ccnode; }; /* Custom Class */
struct  MUIP_Window_RemControlCharHandler   { ULONG MethodID; struct MUI_EventHandlerNode *ccnode; }; /* Custom Class */
struct  MUIP_Window_DragObject { ULONG MethodID; Object *obj; LONG touchx; LONG touchy; ULONG flags; };

/* Attributes */

#define MUIA_Window_DisableKeys   0x80424c36 /* V15 isg ULONG */ 

#define MUIM_Window_RecalcDisplay     0x80429abc
struct  MUIP_Window_RecalcDisplay  { ULONG MethodID; };


extern const struct __MUIBuiltinClass _MUI_Window_desc; /* PRIV */

#endif
