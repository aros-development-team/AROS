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

/*** Name *******************************************************************/
#define MUIC_Area                   "Area.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Area                   (MUIB_ZUNE | 0x00000200)

/*** Methods ****************************************************************/
#define MUIM_AskMinMax              (MUIB_MUI|0x00423874) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Cleanup                (MUIB_MUI|0x0042d985) /* MUI: V4  */ /* For Custom Classes only */
#define MUIM_ContextMenuBuild       (MUIB_MUI|0x00429d2e) /* MUI: V11 */
#define MUIM_ContextMenuChoice      (MUIB_MUI|0x00420f0e) /* MUI: V11 */
#define MUIM_CreateBubble	    (MUIB_MUI|0x00421c41) /* MUI: V18 */
#define MUIM_CreateDragImage	    (MUIB_MUI|0x0042eb6f) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */
#define MUIM_CreateShortHelp        (MUIB_MUI|0x00428e93) /* MUI: V11 */
#define MUIM_CustomBackfill	    (MUIB_MUI|0x00428d73) /* Undoc */
#define MUIM_DeleteBubble	    (MUIB_MUI|0x004211af) /* MUI: V18 */
#define MUIM_DeleteDragImage	    (MUIB_MUI|0x00423037) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */
#define MUIM_DeleteShortHelp        (MUIB_MUI|0x0042d35a) /* MUI: V11 */
#define MUIM_DoDrag		    (MUIB_MUI|0x004216bb) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */
#define MUIM_DragBegin	            (MUIB_MUI|0x0042c03a) /* MUI: V11 */
#define MUIM_DragDrop	            (MUIB_MUI|0x0042c555) /* MUI: V11 */
#define MUIM_DragFinish	            (MUIB_MUI|0x004251f0) /* MUI: V11 */
#define MUIM_DragQuery	            (MUIB_MUI|0x00420261) /* MUI: V11 */
#define MUIM_DragReport	            (MUIB_MUI|0x0042edad) /* MUI: V11 */
#define MUIM_Draw		    (MUIB_MUI|0x00426f3f) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_DrawBackground	    (MUIB_MUI|0x004238ca) /* MUI: V11 */
#define MUIM_GoActive		    (MUIB_MUI|0x0042491a) /* Undoc */
#define MUIM_GoInactive	            (MUIB_MUI|0x00422c0c) /* Undoc */
#define MUIM_HandleEvent	    (MUIB_MUI|0x00426d66) /* MUI: V16 */ /* For Custom Classes only */ 
#define MUIM_HandleInput	    (MUIB_MUI|0x00422a1a) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Hide		    (MUIB_MUI|0x0042f20f) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Setup		    (MUIB_MUI|0x00428354) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Show		    (MUIB_MUI|0x0042cc84) /* MUI: V4  */ /* For Custom Classes only */ 
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

#define MUIM_Layout                 (MUIB_Area | 0x00000000)
#define MUIM_DrawParentBackground   (MUIB_Area | 0x00000001)
#define MUIM_DragQueryExtended      (MUIB_Area | 0x00000002) /* PRIV - returns a object or NULL */
#define MUIM_Timer                  (MUIB_Area | 0x00000003) /* PRIV */
#define MUIM_UpdateInnerSizes       (MUIB_Area | 0x00000004) /* PRIV for now */
struct  MUIP_Layout                 {ULONG MethodID;};
struct  MUIP_DrawParentBackground   {ULONG MethodID; LONG left; LONG top; LONG width; LONG height; LONG xoffset; LONG yoffset; LONG flags;};
struct  MUIP_DragQueryExtended      {ULONG MethodID; Object *obj; LONG x; LONG y;}; /* PRIV */
struct  MUIP_Timer                  {ULONG MethodID; }; /* PRIV */
struct  MUIP_UpdateInnerSizes       {ULONG MethodID; }; /* PRIV */

struct MUI_DragImage
{
    struct BitMap *bm;
    WORD width;  /* exact width and height of bitmap */
    WORD height;
    WORD touchx; /* position of pointer click relative to bitmap */
    WORD touchy;
    ULONG flags; /* must be set to 0 */
};

/*** Attributes *************************************************************/
#define MUIA_Background		(MUIB_MUI|0x0042545b) /* MUI: V4  is. LONG              */
#define MUIA_BottomEdge		(MUIB_MUI|0x0042e552) /* MUI: V4  ..g LONG              */
#define MUIA_ContextMenu		(MUIB_MUI|0x0042b704) /* MUI: V11 isg Object *          */
#define MUIA_ContextMenuTrigger	(MUIB_MUI|0x0042a2c1) /* MUI: V11 ..g Object *          */
#define MUIA_ControlChar        	(MUIB_MUI|0x0042120b) /* MUI: V4  isg char              */
#define MUIA_CustomBackfill		(MUIB_MUI|0x00420a63) /* undoc    i..                   */
#define MUIA_CycleChain         	(MUIB_MUI|0x00421ce7) /* MUI: V11 isg LONG              */
#define MUIA_Disabled           	(MUIB_MUI|0x00423661) /* MUI: V4  isg BOOL              */
#define MUIA_Draggable          	(MUIB_MUI|0x00420b6e) /* MUI: V11 isg BOOL              */
#define MUIA_Dropable           	(MUIB_MUI|0x0042fbce) /* MUI: V11 isg BOOL              */
#define MUIA_FillArea           	(MUIB_MUI|0x004294a3) /* MUI: V4  is. BOOL              */
#define MUIA_FixHeight          	(MUIB_MUI|0x0042a92b) /* MUI: V4  i.. LONG              */
#define MUIA_FixHeightTxt       	(MUIB_MUI|0x004276f2) /* MUI: V4  i.. STRPTR            */
#define MUIA_FixWidth           	(MUIB_MUI|0x0042a3f1) /* MUI: V4  i.. LONG              */
#define MUIA_FixWidthTxt        	(MUIB_MUI|0x0042d044) /* MUI: V4  i.. STRPTR            */
#define MUIA_Font               	(MUIB_MUI|0x0042be50) /* MUI: V4  i.g struct TextFont * */
#define MUIA_Frame              	(MUIB_MUI|0x0042ac64) /* MUI: V4  i.. LONG              */
#define MUIA_FramePhantomHoriz  	(MUIB_MUI|0x0042ed76) /* MUI: V4  i.. BOOL              */
#define MUIA_FrameTitle         	(MUIB_MUI|0x0042d1c7) /* MUI: V4  i.. STRPTR            */
#define MUIA_Height             	(MUIB_MUI|0x00423237) /* MUI: V4  ..g LONG              */
#define MUIA_HorizDisappear     	(MUIB_MUI|0x00429615) /* MUI: V11 isg LONG              */
#define MUIA_HorizWeight        	(MUIB_MUI|0x00426db9) /* MUI: V4  isg WORD              */
#define MUIA_InnerBottom        	(MUIB_MUI|0x0042f2c0) /* MUI: V4  i.g LONG              */
#define MUIA_InnerLeft          	(MUIB_MUI|0x004228f8) /* MUI: V4  i.g LONG              */
#define MUIA_InnerRight         	(MUIB_MUI|0x004297ff) /* MUI: V4  i.g LONG              */
#define MUIA_InnerTop           	(MUIB_MUI|0x00421eb6) /* MUI: V4  i.g LONG              */
#define MUIA_InputMode          	(MUIB_MUI|0x0042fb04) /* MUI: V4  i.. LONG              */
#define MUIA_LeftEdge           	(MUIB_MUI|0x0042bec6) /* MUI: V4  ..g LONG              */
#define MUIA_MaxHeight          	(MUIB_MUI|0x004293e4) /* MUI: V11 i.. LONG              */
#define MUIA_MaxWidth           	(MUIB_MUI|0x0042f112) /* MUI: V11 i.. LONG              */
#define MUIA_Pressed            	(MUIB_MUI|0x00423535) /* MUI: V4  ..g BOOL              */
#define MUIA_RightEdge          	(MUIB_MUI|0x0042ba82) /* MUI: V4  ..g LONG              */
#define MUIA_Selected           	(MUIB_MUI|0x0042654b) /* MUI: V4  isg BOOL              */
#define MUIA_ShortHelp          	(MUIB_MUI|0x00428fe3) /* MUI: V11 isg STRPTR            */
#define MUIA_ShowMe             	(MUIB_MUI|0x00429ba8) /* MUI: V4  isg BOOL              */
#define MUIA_ShowSelState       	(MUIB_MUI|0x0042caac) /* MUI: V4  i.. BOOL              */
#define MUIA_Timer              	(MUIB_MUI|0x00426435) /* MUI: V4  ..g LONG              */
#define MUIA_TopEdge            	(MUIB_MUI|0x0042509b) /* MUI: V4  ..g LONG              */
#define MUIA_VertDisappear      	(MUIB_MUI|0x0042d12f) /* MUI: V11 isg LONG              */
#define MUIA_VertWeight         	(MUIB_MUI|0x004298d0) /* MUI: V4  isg WORD              */
#define MUIA_Weight             	(MUIB_MUI|0x00421d1f) /* MUI: V4  i.. WORD              */
#define MUIA_Width              	(MUIB_MUI|0x0042b59c) /* MUI: V4  ..g LONG              */
#define MUIA_Window             	(MUIB_MUI|0x00421591) /* MUI: V4  ..g struct Window *   */
#define MUIA_WindowObject       	(MUIB_MUI|0x0042669e) /* MUI: V4  ..g Object *          */

#define MUIA_NestedDisabled             (MUIB_Area | 0x00000000) /* Zune 20030530  isg BOOL        */

#ifdef MUI_OBSOLETE	 		
#define MUIA_ExportID (MUIB_MUI|0x0042d76e) /* V4  isg ULONG */
#endif /* MUI_OBSOLETE */		

struct MUI_ImageSpec_intern;

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
    CONST_STRPTR       mad_FrameTitle;     /* for groups. Req. mad_Frame > 0 */
    WORD               mad_HardHeight;     /* if harcoded dim (see flags)  */
    WORD               mad_HardWidth;      /* if harcoded dim (see flags)  */
    CONST_STRPTR       mad_HardWidthTxt;
    CONST_STRPTR       mad_HardHeightTxt;
    struct MUI_ImageSpec_intern *mad_Background;  /* bg setting */
    struct MUI_ImageSpec_intern *mad_SelBack;     /* selected state background */
    CONST_STRPTR       mad_BackgroundSpec;
    WORD               mad_HardILeft;      /* hardcoded inner values */
    WORD               mad_HardIRight;
    WORD               mad_HardITop;
    WORD               mad_HardIBottom;
    UWORD              mad_HorizWeight;    /* weight values for layout */
    UWORD              mad_VertWeight;
    CONST_STRPTR       mad_ShortHelp;      /* bubble help */
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

    LONG               mad_DisableCount; /* counts number of disables */
    /* END PRIV */
};


/* Flags during MUIM_Draw */
#define MADF_DRAWOBJECT        (1<< 0) /* draw object completely */
#define MADF_DRAWUPDATE        (1<< 1) /* update object */

#define MADF_DRAWALL           (1<< 19)


/* mad_Flags, private one */
#define MADF_DRAW_XXX          (1<< 2) /* PRIV - mui verified, what use ? */
#define MADF_DRAGGABLE         (1<< 3) /* PRIV - mui verified */
#define MADF_MAXHEIGHT         (1<< 4) /* PRIV - share bit 6 in mui */
#define MADF_CYCLECHAIN        (1<< 5) /* PRIV - mui verified */
#define MADF_MAXWIDTH          (1<< 6) /* PRIV - share bit 6 in mui */
#define MADF_DRAGGING          (1<< 7) /* PRIV - zune-specific ? */
#define MADF_OWNBG	       (1<< 8) /* PRIV - zune-specific ? */
#define MADF_SHOWME            (1<< 9) /* PRIV - mui verified */
#define MADF_BORDERGADGET      (1<< 10) /* PRIV - is a border gadget; zune-specific ? */
#define MADF_DRAWFRAME         (1<< 11) /* PRIV - nearly mui verified */
#define MADF_DRAW_XXX_2        (1<< 12) /* PRIV - mui verified, what use ? */
#define MADF_DROPABLE          (1<< 13) /* PRIV - mui verified */
#define MADF_CANDRAW           (1<< 14) /* PRIV - zune-specific */
#define MADF_DISABLED          (1<< 15) /* PRIV - mui verified */
#define MADF_SHOWSELSTATE      (1<< 16) /* PRIV - mui verified */
#define MADF_PRESSED           (1<< 17) /* PRIV - nearly mui verified */
#define MADF_SELECTED          (1<< 18) /* PRIV - mui verified */
#define MADF_FIXHEIGHT         (1<< 19) /* PRIV - zune-specific */
#define MADF_FILLAREA          (1<< 20) /* PRIV - mui verified */
#define MADF_FIXWIDTH          (1<< 22) /* PRIV - zune-specific */
#define MADF_FIXHEIGHTTXT      (1<< 22) /* PRIV - mui verified (unused in zune) */
#define MADF_INNERLEFT         (1<< 23) /* PRIV - mui verified */
#define MADF_INNERTOP          (1<< 24) /* PRIV - mui verified */
#define MADF_INNERRIGHT        (1<< 25) /* PRIV - mui verified */
#define MADF_INNERBOTTOM       (1<< 26) /* PRIV - mui verified */
#define MADF_FRAMEPHANTOM      (1<< 27) /* PRIV - mui verified */
#define MADF_SETUP             (1<< 28) /* PRIV - zune-specific */

#define MADF_INVIRTUALGROUP	(1<<29) /* PRIV UNDOC: The object is inside a virtual group */
#define MADF_ISVIRTUALGROUP	(1<<30) /* PRIV UNDOC: The object is a virtual group */

#define MADF_DRAWFLAGS (MADF_DRAWOBJECT | MADF_DRAWUPDATE | MADF_DRAW_XXX \
    | MADF_DRAWFRAME | MADF_DRAW_XXX_2)


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

/* A private functions and macros */
void __area_finish_minmax(Object *obj, struct MUI_MinMax *MinMaxInfo);

/*#define DRAW_BG_RECURSIVE (1<<1)*/
#define _vweight(obj)  (muiAreaData(obj)->mad_VertWeight)   /* accesses private members PRIV */
#define _hweight(obj)  (muiAreaData(obj)->mad_HorizWeight)  /* accesses private members PRIV */

extern const struct __MUIBuiltinClass _MUI_Area_desc; /* PRIV */

#endif /* _MUI_CLASSES_AREA_H */
