/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_CLASSES_AREA_H
#define _MUI_CLASSES_AREA_H

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef GRAPHICS_GRAPHICS_H
#include <graphics/gfx.h>
#endif

#ifndef _MUI_CLASSES_WINDOW_H
#include "classes/window.h" /* for MUI_EventHandlerNode, will be gone if this MUI_AreaData is moved to area.c */
#endif

/* Classname */
#define MUIC_Area "Area.mui"

#ifndef METHOD_USER
#define METHOD_USER TAG_USER
#endif

/* Area methods */
#define MUIM_AskMinMax              (METHOD_USER|0x00423874) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Cleanup                (METHOD_USER|0x0042d985) /* MUI: V4  */ /* For Custom Classes only */
#define MUIM_ContextMenuBuild       (METHOD_USER|0x00429d2e) /* MUI: V11 */
#define MUIM_ContextMenuChoice      (METHOD_USER|0x00420f0e) /* MUI: V11 */
#define MUIM_CreateBubble	    (METHOD_USER|0x00421c41) /* MUI: V18 */
#define MUIM_CreateDragImage	    (METHOD_USER|0x0042eb6f) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */
#define MUIM_CreateShortHelp        (METHOD_USER|0x00428e93) /* MUI: V11 */
#define MUIM_CustomBackfill	    (METHOD_USER|0x00428d73) /* Undoc */
#define MUIM_DeleteBubble	    (METHOD_USER|0x004211af) /* MUI: V18 */
#define MUIM_DeleteDragImage	    (METHOD_USER|0x00423037) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */
#define MUIM_DeleteShortHelp        (METHOD_USER|0x0042d35a) /* MUI: V11 */
#define MUIM_DoDrag		    (METHOD_USER|0x004216bb) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */
#define MUIM_DragBegin	            (METHOD_USER|0x0042c03a) /* MUI: V11 */
#define MUIM_DragDrop	            (METHOD_USER|0x0042c555) /* MUI: V11 */
#define MUIM_DragFinish	    (METHOD_USER|0x004251f0) /* MUI: V11 */
#define MUIM_DragQuery	            (METHOD_USER|0x00420261) /* MUI: V11 */
#define MUIM_DragReport	    (METHOD_USER|0x0042edad) /* MUI: V11 */
#define MUIM_Draw		    (METHOD_USER|0x00426f3f) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_DrawBackground	    (METHOD_USER|0x004238ca) /* MUI: V11 */
#define MUIM_GoActive		    (METHOD_USER|0x0042491a) /* Undoc */
#define MUIM_GoInactive	    (METHOD_USER|0x00422c0c) /* Undoc */
#define MUIM_HandleEvent	    (METHOD_USER|0x00426d66) /* MUI: V16 */ /* For Custom Classes only */ 
#define MUIM_HandleInput	    (METHOD_USER|0x00422a1a) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Hide		    (METHOD_USER|0x0042f20f) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Setup		    (METHOD_USER|0x00428354) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Show		    (METHOD_USER|0x0042cc84) /* MUI: V4  */ /* For Custom Classes only */ 
struct MUIP_AskMinMax		    {ULONG MethodID; struct MUI_MinMax *MinMaxInfo;};
struct MUIP_Cleanup		    {ULONG MethodID;};
struct MUIP_ContextMenuBuild	    {ULONG MethodID; LONG mx; LONG my;};
struct MUIP_ContextMenuChoice	    {ULONG MethodID; Object *item;};
struct MUIP_CreateBubble	    {ULONG MethodID; LONG x; LONG y; char *txt; ULONG flags;};
struct MUIP_CreateDragImage	    {ULONG MethodID; LONG touchx; LONG touchy; ULONG flags;};
struct MUIP_CreateShortHelp	    {ULONG MethodID; LONG mx; LONG my;};
struct MUIP_CustomBackfill 	    {ULONG MethodID; LONG left; LONG top; LONG right; LONG bottom; LONG xoffset; LONG yoffset;};
struct MUIP_DeleteBubble	    {ULONG MethodID; APTR bubble;};
struct MUIP_DeleteDragImage	    {ULONG MethodID; struct MUI_DragImage *di;};
struct MUIP_DeleteShortHelp	    {ULONG MethodID; STRPTR help; };
struct MUIP_DoDrag          	    {ULONG MethodID; LONG touchx; LONG touchy; ULONG flags;};
struct MUIP_DragBegin               {ULONG MethodID; Object *obj;};
struct MUIP_DragDrop                {ULONG MethodID; Object *obj; LONG x; LONG y;};
struct MUIP_DragFinish              {ULONG MethodID; Object *obj;};
struct MUIP_DragQuery               {ULONG MethodID; Object *obj;};
struct MUIP_DragReport              {ULONG MethodID; Object *obj; LONG x; LONG y; LONG update;};
struct MUIP_Draw                    {ULONG MethodID; ULONG flags;};
struct MUIP_DrawBackground          {ULONG MethodID; LONG left; LONG top; LONG width; LONG height; LONG xoffset; LONG yoffset; LONG flags;};
struct MUIP_GoActive                {ULONG MethodID;};
struct MUIP_GoInacrive              {ULONG MethodID;};
struct MUIP_HandleEvent             {ULONG MethodID; struct IntuiMessage *imsg; LONG muikey;};
struct MUIP_HandleInput             {ULONG MethodID; struct IntuiMessage *imsg; LONG muikey;};
struct MUIP_Hide                    {ULONG MethodID;};
struct MUIP_Setup                   {ULONG MethodID; struct MUI_RenderInfo *RenderInfo;};
struct MUIP_Show                    {ULONG MethodID;};

struct MUI_DragImage
{
    struct BitMap *bm;
    WORD width;  /* exact width and height of bitmap */
    WORD height;
    WORD touchx; /* position of pointer click relative to bitmap */
    WORD touchy;
    ULONG flags; /* must be set to 0 */
};

/* Area attributes */
#define MUIA_Background		(TAG_USER|0x0042545b) /* MUI: V4  is. LONG              */
#define MUIA_BottomEdge		(TAG_USER|0x0042e552) /* MUI: V4  ..g LONG              */
#define MUIA_ContextMenu		(TAG_USER|0x0042b704) /* MUI: V11 isg Object *          */
#define MUIA_ContextMenuTrigger	(TAG_USER|0x0042a2c1) /* MUI: V11 ..g Object *          */
#define MUIA_ControlChar        	(TAG_USER|0x0042120b) /* MUI: V4  isg char              */
#define MUIA_CustomBackfill		(TAG_USER|0x00420a63) /* undoc    i..                   */
#define MUIA_CycleChain         	(TAG_USER|0x00421ce7) /* MUI: V11 isg LONG              */
#define MUIA_Disabled           	(TAG_USER|0x00423661) /* MUI: V4  isg BOOL              */
#define MUIA_Draggable          	(TAG_USER|0x00420b6e) /* MUI: V11 isg BOOL              */
#define MUIA_Dropable           	(TAG_USER|0x0042fbce) /* MUI: V11 isg BOOL              */
#define MUIA_FillArea           	(TAG_USER|0x004294a3) /* MUI: V4  is. BOOL              */
#define MUIA_FixHeight          	(TAG_USER|0x0042a92b) /* MUI: V4  i.. LONG              */
#define MUIA_FixHeightTxt       	(TAG_USER|0x004276f2) /* MUI: V4  i.. STRPTR            */
#define MUIA_FixWidth           	(TAG_USER|0x0042a3f1) /* MUI: V4  i.. LONG              */
#define MUIA_FixWidthTxt        	(TAG_USER|0x0042d044) /* MUI: V4  i.. STRPTR            */
#define MUIA_Font               	(TAG_USER|0x0042be50) /* MUI: V4  i.g struct TextFont * */
#define MUIA_Frame              	(TAG_USER|0x0042ac64) /* MUI: V4  i.. LONG              */
#define MUIA_FramePhantomHoriz  	(TAG_USER|0x0042ed76) /* MUI: V4  i.. BOOL              */
#define MUIA_FrameTitle         	(TAG_USER|0x0042d1c7) /* MUI: V4  i.. STRPTR            */
#define MUIA_Height             	(TAG_USER|0x00423237) /* MUI: V4  ..g LONG              */
#define MUIA_HorizDisappear     	(TAG_USER|0x00429615) /* MUI: V11 isg LONG              */
#define MUIA_HorizWeight        	(TAG_USER|0x00426db9) /* MUI: V4  isg WORD              */
#define MUIA_InnerBottom        	(TAG_USER|0x0042f2c0) /* MUI: V4  i.g LONG              */
#define MUIA_InnerLeft          	(TAG_USER|0x004228f8) /* MUI: V4  i.g LONG              */
#define MUIA_InnerRight         	(TAG_USER|0x004297ff) /* MUI: V4  i.g LONG              */
#define MUIA_InnerTop           	(TAG_USER|0x00421eb6) /* MUI: V4  i.g LONG              */
#define MUIA_InputMode          	(TAG_USER|0x0042fb04) /* MUI: V4  i.. LONG              */
#define MUIA_LeftEdge           	(TAG_USER|0x0042bec6) /* MUI: V4  ..g LONG              */
#define MUIA_MaxHeight          	(TAG_USER|0x004293e4) /* MUI: V11 i.. LONG              */
#define MUIA_MaxWidth           	(TAG_USER|0x0042f112) /* MUI: V11 i.. LONG              */
#define MUIA_Pressed            	(TAG_USER|0x00423535) /* MUI: V4  ..g BOOL              */
#define MUIA_RightEdge          	(TAG_USER|0x0042ba82) /* MUI: V4  ..g LONG              */
#define MUIA_Selected           	(TAG_USER|0x0042654b) /* MUI: V4  isg BOOL              */
#define MUIA_ShortHelp          	(TAG_USER|0x00428fe3) /* MUI: V11 isg STRPTR            */
#define MUIA_ShowMe             	(TAG_USER|0x00429ba8) /* MUI: V4  isg BOOL              */
#define MUIA_ShowSelState       	(TAG_USER|0x0042caac) /* MUI: V4  i.. BOOL              */
#define MUIA_Timer              	(TAG_USER|0x00426435) /* MUI: V4  ..g LONG              */
#define MUIA_TopEdge            	(TAG_USER|0x0042509b) /* MUI: V4  ..g LONG              */
#define MUIA_VertDisappear      	(TAG_USER|0x0042d12f) /* MUI: V11 isg LONG              */
#define MUIA_VertWeight         	(TAG_USER|0x004298d0) /* MUI: V4  isg WORD              */
#define MUIA_Weight             	(TAG_USER|0x00421d1f) /* MUI: V4  i.. WORD              */
#define MUIA_Width              	(TAG_USER|0x0042b59c) /* MUI: V4  ..g LONG              */
#define MUIA_Window             	(TAG_USER|0x00421591) /* MUI: V4  ..g struct Window *   */
#define MUIA_WindowObject       	(TAG_USER|0x0042669e) /* MUI: V4  ..g Object *          */

#ifdef MUI_OBSOLETE	 		
#define MUIA_ExportID (TAG_USER|0x0042d76e) /* V4  isg ULONG */
#endif /* MUI_OBSOLETE */		


struct MUI_AreaData
{
    struct MUI_RenderInfo *mad_RenderInfo; /* RenderInfo for this object */
    LONG               mad_FontPreset;     /* MUI font wanted - private */
    struct TextFont   *mad_Font;           /* Font which is used to draw */
    struct MUI_MinMax  mad_MinMax;         /* min/max/default dimensions */
    struct IBox        mad_Box;            /* coordinates and dim of this object after layouted */
    BYTE               mad_addleft;        /* left offset (frame & innerspacing) */
    BYTE               mad_addtop;         /* top offset (frame & innerspacing) */
    BYTE               mad_subwidth;       /* additional width (frame & innerspacing) */
    BYTE               mad_subheight;      /* additional height (frame & innerspacing) */
    ULONG              mad_Flags;          /* some flags; see below */

    /* The following data is private */
    /* START PRIV */
    WORD               mad_InputMode;      /* how to react to events */
    WORD               mad_Frame;          /* frame setting -- private */
    STRPTR             mad_FrameTitle;     /* for groups. Req. mad_Frame > 0 */
    WORD               mad_HardHeight;     /* if harcoded dim (see flags)  */
    WORD               mad_HardWidth;      /* if harcoded dim (see flags)  */
    struct MUI_ImageSpec *mad_Background;  /* bg setting */
    struct MUI_ImageSpec *mad_SelBack;     /* selected state background */
    WORD               mad_HardILeft;      /* hardcoded inner values */
    WORD               mad_HardIRight;
    WORD               mad_HardITop;
    WORD               mad_HardIBottom;
    UWORD              mad_HorizWeight;    /* weight values for layout */
    UWORD              mad_VertWeight;
    STRPTR             mad_ShortHelp;      /* bubble help */
    struct MUI_EventHandlerNode mad_ehn;
    struct MUI_InputHandlerNode mad_Timer; /* MUIA_Timer */
    ULONG              mad_Timeval;       /* just to trigger notifications */
    struct MUI_EventHandlerNode mad_ccn;  /* gross hack for control char */
    Object            *mad_ContextMenu;   /* menu strip */
    struct ZText      *mad_TitleText;     /* frame title */
    TEXT               mad_ControlChar;   /* key shortcut */
    LONG               mad_ClickX;        /* x position of the initial SELECTDOWN click */
    LONG               mad_ClickY;        /* y position of the intiial SELECTDOWN click */
    struct ZMenu      *mad_ContextZMenu;
    struct MUI_EventHandlerNode mad_hiehn; /* Eventhandler to simulate MUIM_HandleInput */
    /* EBD PRIV */
};


/* Flags during MUIM_Draw */
#define MADF_DRAWOBJECT        (1<< 0) /* draw object completely */
#define MADF_DRAWUPDATE        (1<< 1) /* update object */

#define MADF_DRAWALL           (1<< 19)


/* mad_Flags, private one */
#define MADF_FIXHEIGHT         (1<< 2) /* PRIV */
#define MADF_FIXWIDTH          (1<< 3) /* PRIV */
#define MADF_MAXHEIGHT         (1<< 4) /* PRIV */
#define MADF_MAXWIDTH          (1<< 5) /* PRIV */
#define MADF_FILLAREA          (1<< 6) /* PRIV */
#define MADF_INNERLEFT         (1<< 7) /* PRIV */
#define MADF_INNERRIGHT        (1<< 8) /* PRIV */
#define MADF_INNERTOP          (1<< 9) /* PRIV */
#define MADF_INNERBOTTOM       (1<< 10) /* PRIV */
#define MADF_FRAMEPHANTOM      (1<< 11) /* PRIV */
#define MADF_SELECTED          (1<< 12) /* PRIV */
#define MADF_PRESSED           (1<< 13) /* PRIV */
#define MADF_SHOWME            (1<< 14) /* PRIV */
#define MADF_SHOWSELSTATE      (1<< 15) /* PRIV */
#define MADF_CANDRAW           (1<< 16) /* PRIV */
#define MADF_SETUP             (1<< 17) /* PRIV */
#define MADF_MAXSIZE           (1<< 18) /* PRIV */
#define MADF_CYCLECHAIN        (1<< 20) /* PRIV */
#define MADF_ACTIVE            (1<< 21) /* PRIV */
#define MADF_DRAGGABLE         (1<< 22) /* PRIV */
#define MADF_DRAGGING          (1<< 23) /* PRIV */
#define MADF_DROPABLE          (1<< 24) /* PRIV */

#define MADF_INVIRTUALGROUP	(1<<29) /* PRIV UNDOC: The object is inside a virtual group */
#define MADF_ISVIRTUALGROUP	(1<<30) /* PRIV UNDOC: The object is a virtual group */


enum {
    MUIV_Font_Inherit = 0,
    MUIV_Font_Normal = -1,
    MUIV_Font_List = -2,
    MUIV_Font_Tiny = -3,
    MUIV_Font_Fixed = -4,
    MUIV_Font_Title = -5,
    MUIV_Font_Big = -6,
    MUIV_Font_Button = -7,
    MUIV_Font_Knob = -8,
    MUIV_Font_NegCount = -9,
};

enum {
    MUIV_Frame_None = 0,
    MUIV_Frame_Button,
    MUIV_Frame_ImageButton,
    MUIV_Frame_Text,
    MUIV_Frame_String,
    MUIV_Frame_ReadList,
    MUIV_Frame_InputList,
    MUIV_Frame_Prop,
    MUIV_Frame_Gauge,
    MUIV_Frame_Group,
    MUIV_Frame_PopUp,
    MUIV_Frame_Virtual,
    MUIV_Frame_Slider,
    MUIV_Frame_Knob,
    MUIV_Frame_Drag,
    MUIV_Frame_Count,
};

enum {
    MUIV_InputMode_None = 0,
    MUIV_InputMode_RelVerify,
    MUIV_InputMode_Immediate,
    MUIV_InputMode_Toggle,
};

enum {
    MUIV_DragQuery_Refuse = 0,
    MUIV_DragQuery_Accept,
};

enum {
    MUIV_DragReport_Abort =  0,
    MUIV_DragReport_Continue,
    MUIV_DragReport_Lock,
    MUIV_DragReport_Refresh,
};

#define MUIV_CreateBubble_DontHidePointer (1<<0)


/* The following stuff is Zune only and can is used by some zune subclasses */
#define MUIM_Layout                 (TAG_USER|0x10429abb)
struct  MUIP_Layout                 {ULONG MethodID;};

/* The following is considered as private and Zune only! */
#define MUIM_DragQueryExtended	    (METHOD_USER|0x10092033) /* PRIV - returns a object or NULL */
#define MUIM_Timer		    (METHOD_USER|0x10092032) /* PRIV */
struct  MUIP_DragQueryExtended	    {ULONG MethodID; Object *obj; LONG x; LONG y;}; /* PRIV */
struct  MUIP_Timer 		    {ULONG MethodID; }; /* PRIV */

/* A private functions and macros */
void __area_finish_minmax(Object *obj, struct MUI_MinMax *MinMaxInfo);

/*#define DRAW_BG_RECURSIVE (1<<1)*/
#define _vweight(obj)  (muiAreaData(obj)->mad_VertWeight)   /* accesses private members PRIV */
#define _hweight(obj)  (muiAreaData(obj)->mad_HorizWeight)  /* accesses private members PRIV */

extern const struct __MUIBuiltinClass _MUI_Area_desc; /* PRIV */

#endif
