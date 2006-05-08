#ifndef _MUI_CLASSES_WINDOW_H
#define _MUI_CLASSES_WINDOW_H

/*
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Window                 "Window.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Window                 (MUIB_ZUNE | 0x00003600)

/*** Methods ****************************************************************/
#define MUIM_Window_ActionIconify   (MUIB_MUI|0x00422cc0) /* MUI: V18 undoc*/
#define MUIM_Window_AddEventHandler (MUIB_MUI|0x004203b7) /* MUI: V16 */
#define MUIM_Window_Cleanup         (MUIB_MUI|0x0042ab26) /* MUI: V18 undoc */ /* For custom classes only */
#define MUIM_Window_RemEventHandler (MUIB_MUI|0x0042679e) /* MUI: V16 */
#define MUIM_Window_ScreenToBack    (MUIB_MUI|0x0042913d) /* MUI: V4  */
#define MUIM_Window_ScreenToFront   (MUIB_MUI|0x004227a4) /* MUI: V4  */
#define MUIM_Window_Setup           (MUIB_MUI|0x0042c34c) /* MUI: V18 undoc */ /* For custom Classes only */
#define MUIM_Window_Snapshot        (MUIB_MUI|0x0042945e) /* MUI: V11 */
#define MUIM_Window_ToBack          (MUIB_MUI|0x0042152e) /* MUI: V4  */
#define MUIM_Window_ToFront         (MUIB_MUI|0x0042554f) /* MUI: V4  */
struct  MUIP_Window_ActionIconify   {ULONG MethodID;};
struct  MUIP_Window_AddEventHandler {ULONG MethodID; struct MUI_EventHandlerNode *ehnode;};
struct  MUIP_Window_Cleanup         {ULONG MethodID;};
struct  MUIP_Window_RemEventHandler {ULONG MethodID; struct MUI_EventHandlerNode *ehnode;};
struct  MUIP_Window_ScreenToBack    {ULONG MethodID;};
struct  MUIP_Window_ScreenToFront   {ULONG MethodID;};
struct  MUIP_Window_Setup           {ULONG MethodID;};
struct  MUIP_Window_Snapshot        {ULONG MethodID; LONG flags;};
struct  MUIP_Window_ToBack          {ULONG MethodID;};
struct  MUIP_Window_ToFront         {ULONG MethodID;};

#define MUIM_Window_AddControlCharHandler  (MUIB_Window | 0x00000000) /* Zune: V1, PRIV don't use it! */
#define MUIM_Window_AllocGadgetID          (MUIB_Window | 0x00000001) /* Zune: V1 - allocate a GadgetID for BOOPSI gadgets */
#define MUIM_Window_DrawBackground         (MUIB_Window | 0x00000002) /* Zune: V1 - like MUIM_DrawBackground but PRIV */
#define MUIM_Window_DragObject             (MUIB_Window | 0x00000003) /* Zune: V1, PRIV don't use it! */
#define MUIM_Window_FreeGadgetID           (MUIB_Window | 0x00000004) /* Zune: V1 - free the GadgetID for BOOPSI gadgets */
#define MUIM_Window_RecalcDisplay          (MUIB_Window | 0x00000005) /* Zune: V1, PRIV don't use it! */
#define MUIM_Window_RemControlCharHandler  (MUIB_Window | 0x00000006) /* Zune: V1, PRIV don't use it! */
struct  MUIP_Window_AddControlCharHandler  { ULONG MethodID; struct MUI_EventHandlerNode *ccnode; };
struct  MUIP_Window_AllocGadgetID          { ULONG MethodID; }; /* Custom Class - returns the Gadget ID */
struct  MUIP_Window_DrawBackground         { ULONG MethodID; LONG left; LONG top; LONG width; LONG height; LONG xoffset; LONG yoffset; LONG flags;};
struct  MUIP_Window_DragObject             { ULONG MethodID; Object *obj; LONG touchx; LONG touchy; ULONG flags; };
struct  MUIP_Window_FreeGadgetID           { ULONG MethodID; LONG gadgetid; }; /* Custom Class */
struct  MUIP_Window_RecalcDisplay          { ULONG MethodID; Object *originator; };
struct  MUIP_Window_RemControlCharHandler  { ULONG MethodID; struct MUI_EventHandlerNode *ccnode; };

#ifdef MUI_OBSOLETE
#define MUIM_Window_GetMenuCheck    (MUIB_MUI|0x00420414) /* MUI: V4  */
#define MUIM_Window_GetMenuState    (MUIB_MUI|0x00420d2f) /* MUI: V4  */
#define MUIM_Window_SetCycleChain   (MUIB_MUI|0x00426510) /* MUI: V4  */
#define MUIM_Window_SetMenuCheck    (MUIB_MUI|0x00422243) /* MUI: V4  */
#define MUIM_Window_SetMenuState    (MUIB_MUI|0x00422b5e) /* MUI: V4  */
struct  MUIP_Window_GetMenuCheck    {ULONG MethodID; ULONG MenuID;};
struct  MUIP_Window_GetMenuState    {ULONG MethodID; ULONG MenuID;};
struct  MUIP_Window_SetCycleChain   {ULONG MethodID; Object *obj[1];};
struct  MUIP_Window_SetMenuCheck    {ULONG MethodID; ULONG MenuID; LONG stat;};
struct  MUIP_Window_SetMenuState    {ULONG MethodID; ULONG MenuID; LONG stat;};
#endif /* MUI_OBSOLETE */

/*** Attributes *************************************************************/
#define MUIA_Window_Activate                (MUIB_MUI|0x00428d2f) /* MUI: V4  isg BOOL                */
#define MUIA_Window_ActiveObject            (MUIB_MUI|0x00427925) /* MUI: V4  .sg Object *            */
#define MUIA_Window_AltHeight               (MUIB_MUI|0x0042cce3) /* MUI: V4  i.g LONG                */
#define MUIA_Window_AltLeftEdge             (MUIB_MUI|0x00422d65) /* MUI: V4  i.g LONG                */
#define MUIA_Window_AltTopEdge              (MUIB_MUI|0x0042e99b) /* MUI: V4  i.g LONG                */
#define MUIA_Window_AltWidth                (MUIB_MUI|0x004260f4) /* MUI: V4  i.g LONG                */
#define MUIA_Window_AppWindow               (MUIB_MUI|0x004280cf) /* MUI: V5  i.. BOOL                */
#define MUIA_Window_Backdrop                (MUIB_MUI|0x0042c0bb) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_Borderless              (MUIB_MUI|0x00429b79) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_CloseGadget             (MUIB_MUI|0x0042a110) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_CloseRequest            (MUIB_MUI|0x0042e86e) /* MUI: V4  ..g BOOL                */
#define MUIA_Window_DefaultObject           (MUIB_MUI|0x004294d7) /* MUI: V4  isg Object *            */
#define MUIA_Window_DepthGadget             (MUIB_MUI|0x00421923) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_DisableKeys             (MUIB_MUI|0x00424c36) /* MUI: V15 isg ULONG               */ /* undoc */
#define MUIA_Window_DragBar                 (MUIB_MUI|0x0042045d) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_FancyDrawing            (MUIB_MUI|0x0042bd0e) /* MUI: V8  isg BOOL                */
#define MUIA_Window_Height                  (MUIB_MUI|0x00425846) /* MUI: V4  i.g LONG                */
#define MUIA_Window_ID                      (MUIB_MUI|0x004201bd) /* MUI: V4  isg ULONG               */
#define MUIA_Window_InputEvent              (MUIB_MUI|0x004247d8) /* MUI: V4  ..g struct InputEvent * */
#define MUIA_Window_IsSubWindow             (MUIB_MUI|0x0042b5aa) /* MUI: V4  isg BOOL                */
#define MUIA_Window_LeftEdge                (MUIB_MUI|0x00426c65) /* MUI: V4  i.g LONG                */
#define MUIA_Window_MenuAction              (MUIB_MUI|0x00427521) /* MUI: V8  isg ULONG               */
#define MUIA_Window_Menustrip               (MUIB_MUI|0x0042855e) /* MUI: V8  i.g Object *            */
#define MUIA_Window_MouseObject             (MUIB_MUI|0x0042bf9b) /* MUI: V10 ..g Object *            */
#define MUIA_Window_NeedsMouseObject        (MUIB_MUI|0x0042372a) /* MUI: V10 i.. BOOL                */
#define MUIA_Window_NoMenus                 (MUIB_MUI|0x00429df5) /* MUI: V4  is. BOOL                */
#define MUIA_Window_Open                    (MUIB_MUI|0x00428aa0) /* MUI: V4  .sg BOOL                */
#define MUIA_Window_PublicScreen            (MUIB_MUI|0x004278e4) /* MUI: V6  isg STRPTR              */
#define MUIA_Window_RefWindow               (MUIB_MUI|0x004201f4) /* MUI: V4  is. Object *            */
#define MUIA_Window_RootObject              (MUIB_MUI|0x0042cba5) /* MUI: V4  isg Object *            */
#define MUIA_Window_Screen                  (MUIB_MUI|0x0042df4f) /* MUI: V4  isg struct Screen *     */
#define MUIA_Window_ScreenTitle             (MUIB_MUI|0x004234b0) /* MUI: V5  isg STRPTR              */
#define MUIA_Window_SizeGadget              (MUIB_MUI|0x0042e33d) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_SizeRight               (MUIB_MUI|0x00424780) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_Sleep                   (MUIB_MUI|0x0042e7db) /* MUI: V4  .sg BOOL                */
#define MUIA_Window_Title                   (MUIB_MUI|0x0042ad3d) /* MUI: V4  isg STRPTR              */
#define MUIA_Window_TopEdge                 (MUIB_MUI|0x00427c66) /* MUI: V4  i.g LONG                */
#define MUIA_Window_UseBottomBorderScroller (MUIB_MUI|0x00424e79) /* MUI: V13 isg BOOL                */
#define MUIA_Window_UseLeftBorderScroller   (MUIB_MUI|0x0042433e) /* MUI: V13 isg BOOL                */
#define MUIA_Window_UseRightBorderScroller  (MUIB_MUI|0x0042c05e) /* MUI: V13 isg BOOL                */
#define MUIA_Window_Width                   (MUIB_MUI|0x0042dcae) /* MUI: V4  i.g LONG                */
#define MUIA_Window_Window                  (MUIB_MUI|0x00426a42) /* MUI: V4  ..g struct Window *     */

#define MUIA_Window_EraseArea               (MUIB_Window | 0x00000000) /* Zune only i.. BOOL (default: TRUE) */
#define MUIA_Window_WandererBackdrop	    (MUIB_Window | 0x00000001) /* ZUNE .s. BOOL */
#define MUIA_Window_ZoomGadget              (MUIB_Window | 0x00000002)

#define MUIV_Window_ActiveObject_None       0
#define MUIV_Window_ActiveObject_Next       (-1)
#define MUIV_Window_ActiveObject_Prev       (-2)
#define MUIV_Window_AltHeight_MinMax(p)     (0-(p))
#define MUIV_Window_AltHeight_Visible(p)    (-100-(p))
#define MUIV_Window_AltHeight_Screen(p)     (-200-(p))
#define MUIV_Window_AltHeight_Scaled        (-1000)
#define MUIV_Window_AltLeftEdge_Centered    (-1)
#define MUIV_Window_AltLeftEdge_Moused      (-2)
#define MUIV_Window_AltLeftEdge_NoChange    (-1000)
#define MUIV_Window_AltTopEdge_Centered     (-1)
#define MUIV_Window_AltTopEdge_Moused       (-2)
#define MUIV_Window_AltTopEdge_Delta(p)     (-3-(p))
#define MUIV_Window_AltTopEdge_NoChange     (-1000)
#define MUIV_Window_AltWidth_MinMax(p)      (0-(p))
#define MUIV_Window_AltWidth_Visible(p)     (-100-(p))
#define MUIV_Window_AltWidth_Screen(p)      (-200-(p))
#define MUIV_Window_AltWidth_Scaled         (-1000)
#define MUIV_Window_Height_MinMax(p)        (0-(p))
#define MUIV_Window_Height_Visible(p)       (-100-(p))
#define MUIV_Window_Height_Screen(p)        (-200-(p))
#define MUIV_Window_Height_Scaled           (-1000)
#define MUIV_Window_Height_Default          (-1001)
#define MUIV_Window_LeftEdge_Centered       (-1)
#define MUIV_Window_LeftEdge_Moused         (-2)
#define MUIV_Window_TopEdge_Centered        (-1)
#define MUIV_Window_TopEdge_Moused          (-2)
#define MUIV_Window_TopEdge_Delta(p)        (-3-(p))
#define MUIV_Window_Width_MinMax(p)         (0-(p))
#define MUIV_Window_Width_Visible(p)        (-100-(p))
#define MUIV_Window_Width_Screen(p)         (-200-(p))
#define MUIV_Window_Width_Scaled            (-1000)
#define MUIV_Window_Width_Default           (-1001)

#ifdef MUI_OBSOLETE
#define MUIA_Window_Menu            (MUIB_MUI|0x0042db94) /* MUI: V4  i.. struct NewMenu * */
#define MUIV_Window_Menu_NoMenu     (-1)
#endif /* MUI_OBSOLETE */

/**************************************************************************
 Info about the display environment on which all Area Objects have a
 reference to it.
**************************************************************************/

#define MRI_RARRAY_SIZE 20

struct MUI_RenderInfo
{
    Object          *mri_WindowObject;  /* accessable inbetween MUIM_Setup/MUIM_Cleanup */
    struct Screen   *mri_Screen;        /* accessable inbetween MUIM_Setup/MUIM_Cleanup */
    struct DrawInfo *mri_DrawInfo;      /* accessable inbetween MUIM_Setup/MUIM_Cleanup */
    UWORD           *mri_Pens;          /* accessable inbetween MUIM_Setup/MUIM_Cleanup */
    struct Window   *mri_Window;        /* accessable inbetween MUIM_Show/MUIM_Hide */
    struct RastPort *mri_RastPort;      /* accessable inbetween MUIM_Show/MUIM_Hide */
    ULONG            mri_Flags;         /* accessable inbetween MUIM_Setup/MUIM_Cleanup */

    /* the following stuff is private */
    struct ColorMap *mri_Colormap;
    UWORD            mri_ScreenWidth;
    UWORD            mri_ScreenHeight;
    UWORD            mri_PensStorage[MPEN_COUNT]; /* storage for pens, mri_Pens point to here */

    struct TextFont *mri_Fonts[-MUIV_Font_NegCount]; /* Opened text fonts, done by zune_get_font() */

    /* this is for AddClipping/AddClipRegion */
    struct Region   *mri_rArray[MRI_RARRAY_SIZE];
    int              mri_rCount;

    struct Rectangle mri_ClipRect;

    UWORD            mri_BorderTop;     /* The height of the windows top border (the title) */
    UWORD            mri_BorderBottom;  /* The height of the windows bottom bodder */
    UWORD            mri_BorderLeft;    /* The width of the windows left border */
    UWORD            mri_BorderRight;   /* The width of the windows right border */

    /* Stuff for Borderscrollers */
    Object *mri_LeftImage; /* Valid between MUIM_Setup/MUIM_Cleanup */
    Object *mri_RightImage;
    Object *mri_UpImage;
    Object *mri_DownImage;
    Object *mri_SizeImage;

    Object *mri_VertProp; /* Valid between MUIM_Show/MUIM_Hide */
    Object *mri_HorizProp;

    /* buffering */
    struct RastPort mri_BufferRP;
    struct BitMap  *mri_BufferBM;
};

#define MUIMRI_RECTFILL (1<<0)
#define MUIMRI_TRUECOLOR (1<<1)
#define MUIMRI_THINFRAMES (1<<2)
#define MUIMRI_REFRESHMODE (1<<3)

/**************************************************************************
 MUI_EventHandlerNode as used by
 MUIM_Window_AddEventHandler/RemoveEventHandler
**************************************************************************/
#ifdef __AROS__
/* AROS Nodes are not necessarily binary compatible with AOS ones, so
 * the (MUI_EventHandlerNode *) -> (Node *) cast hack doesnt work.
 */
struct MUI_EventHandlerNode
{
    struct Node    ehn_Node;     /* embedded node structure, private! */
    UWORD          ehn_Flags;    /* some flags, see below */
    Object        *ehn_Object;   /* object which should receive MUIM_HandleEvent. */
    struct IClass *ehn_Class;    /* Class for CoerceMethod(). If NULL DoMethod() is used */
    ULONG          ehn_Events;   /* the IDCMP flags the handler should be invoked. */
    BYTE           ehn_Priority; /* sorted by priority. */
};
#else
struct MUI_EventHandlerNode
{
    struct MinNode ehn_Node;     /* embedded node structure, private! */
    BYTE           ehn_Reserved; /* private! */
    BYTE           ehn_Priority; /* sorted by priority. */
    UWORD          ehn_Flags;    /* some flags, see below */
    Object        *ehn_Object;   /* object which should receive MUIM_HandleEvent. */
    struct IClass *ehn_Class;    /* Class for CoerceMethod(). If NULL DoMethod() is used */
    ULONG          ehn_Events;   /* the IDCMP flags the handler should be invoked. */
};
#endif

/* here are the flags for ehn_Flags */
#define MUI_EHF_ALWAYSKEYS  (1<<0)
#define MUI_EHF_GUIMODE     (1<<1)  /* handler will not be called if object is not visible or disabled */
#define MUI_EHF_HANDLEINPUT (1<<15) /* ZUNEPRIV: Send MUIM_HandleInput instead of MUIM_HandleEvent */

/* MUIM_HandleEvent must return a bitmask where following bit's can be set (all other must be 0) */
#define MUI_EventHandlerRC_Eat (1<<0) /* do not invoke more handlers ers */


extern const struct __MUIBuiltinClass _MUI_Window_desc; /* PRIV */

#endif /* _MUI_CLASSES_WINDOW_H */
