/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef LIBRARIES_MUI_H
#define LIBRARIES_MUI_H

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef GRAPHICS_GRAPHICS_H
#include <graphics/gfx.h>
#endif

#define MUIMASTER_NAME "muimaster.library"

/* Sometype defs in AROS */
#ifndef _AROS
#ifndef _AROS_TYPES_DEFINED
typedef unsigned long IPTR;
typedef long STACKLONG;
typedef unsigned long STACKULONG;
#define _AROS_TYPES_DEFINED
#endif
#endif

/* START PRIV */

/* This structure is used for the internal classes */

#ifndef _AROS
struct __MUIBuiltinClass {
    CONST_STRPTR name;
    CONST_STRPTR supername;
    ULONG        datasize;
    ULONG	   (*dispatcher)();
};

#else

#include <aros/asmcall.h>

struct __MUIBuiltinClass {
    CONST_STRPTR name;
    CONST_STRPTR supername;
    ULONG        datasize;

    AROS_UFP3(IPTR, (*dispatcher),
        AROS_UFPA(Class  *,  cl, A0),
        AROS_UFPA(Object *, obj, A2),
        AROS_UFPA(Msg     , msg, A1));
};
#endif
/* END PRIV */

#ifndef _MUI_CLASSES_NOTIFY_H
#include "classes/notify.h"
#endif

#ifndef _MUI_CLASSES_FAMILY_H
#include "classes/family.h"
#endif

#ifndef _MUI_CLASSES_APPLICATION_H
#include "classes/application.h"
#endif

/* the following prototypes here are only temporary so it can be compiled without problems */
#ifndef _AROS

#ifndef _COMPILER_H
#include "compiler.h"
#endif

__asm APTR MUI_AddClipping(register __a0 struct MUI_RenderInfo *mri, register __d0 WORD left, register __d1 WORD top, register __d2 WORD width, register __d3 WORD height);
__asm APTR MUI_AddClipRegion(register __a0 struct MUI_RenderInfo *mri, register __a1 struct Region *r);
__asm APTR MUI_AllocAslRequest(register __d0 unsigned long reqType, register __a0 struct TagItem *tagList);
__asm BOOL MUI_AslRequest(register __a0 APTR requester, register __a1 struct TagItem *tagList);
__asm BOOL MUI_BeginRefresh(register __a0 struct MUI_RenderInfo *mri, register __d0 ULONG flags);
__asm struct MUI_CustomClass *MUI_CreateCustomClass(register __a0 struct Library *base, register __a1 char *supername, register __a2 struct MUI_CustomClass *supermcc,register __d0 int datasize,register __a3 APTR dispatcher);
__asm BOOL MUI_DeleteCustomClass(register __a0 struct MUI_CustomClass *mcc);
__asm VOID MUI_DisposeObject(register __a0 Object *obj);
__asm VOID MUI_EndRefresh(register __a0 struct MUI_RenderInfo *mri, register __d0 ULONG flags);
__asm LONG MUI_Error(VOID);
__asm VOID MUI_FreeAslRequest(register __a0 APTR requester);
__asm VOID MUI_FreeClass(register __a0 struct IClass *classptr);
__asm struct IClass *MUI_GetClass(register __a0 char *classname);
__asm BOOL MUI_Layout(register __a0 Object *obj,register __d1 LONG left,register __d2 LONG top,register __d3 LONG width,register __d4 LONG height, register __d5 ULONG flags);
__asm Object *MUI_MakeObjectA(register __d0 LONG type, register __a0 ULONG *params);
__asm Object *MUI_NewObjectA(register __a0 char *classname,register __a1 struct TagItem *tags);
__asm LONG MUI_ObtainPen(register __a0 struct MUI_RenderInfo *mri, register __a1 struct MUI_PenSpec *spec, register __d0 ULONG flags);
__asm VOID MUI_Redraw(register __a0 Object *obj, register __d0 ULONG flags);
__asm VOID MUI_RejectIDCMP(register __a0 Object *obj, register __d0 ULONG flags);
__asm VOID MUI_ReleasePen(register __a0 struct MUI_RenderInfo *mri, register __d0 LONG pen);
__asm VOID MUI_RemoveClipping(register __a0 struct MUI_RenderInfo *mri, register __a1 APTR handle);
__asm VOID MUI_RemoveClipRegion(register __a0 struct MUI_RenderInfo *mri, register __a1 APTR handle);
__asm LONG MUI_RequestA(register __d0 APTR app, register __d1 APTR win, register __d2 LONGBITS flags, register __a0 char *title, register __a1 char *gadgets, register __a2 char *format, register __a3 APTR params);
__asm VOID MUI_RequestIDCMP(register __a0 Object *obj, register __d0 ULONG flags);
__asm LONG MUI_SetError(register __d0 LONG num);

__inline static Object *MUI_NewObject(char *classname, int tag,...)
{
    return MUI_NewObjectA(classname, (struct TagItem*)&tag);
}

__inline static Object *MUI_MakeObject(LONG type,...)
{
    return MUI_MakeObjectA(type, ((ULONG*)&type)+1);
}

__inline static APTR MUI_AllocAslRequestTags(unsigned long reqType,...)
{
    return MUI_AllocAslRequest(reqType, (struct TagItem*)(((ULONG*)&reqType)+1));
}

__inline static BOOL MUI_AslRequestTags(APTR requester,...)
{
    return MUI_AslRequest(requester, (struct TagItem*)(((ULONG*)&requester)+1));
}

__inline static LONG MUI_Request(APTR app, APTR win, LONGBITS flags, char *title, char *gadgets, char *format, ...)
{
    return MUI_RequestA(app,win,flags,title,gadgets,format,(((ULONG*)&format)+1));
}

#endif


/**************************************************************************
 Here are the possible Objecttypes for MUI_MakeObject()
**************************************************************************/
enum
{
    MUIO_Label = 1,    /* STRPTR label, ULONG flags */
    MUIO_Button,       /* STRPTR label */
    MUIO_Checkmark,    /* STRPTR label */
    MUIO_Cycle,        /* STRPTR label, STRPTR *entries */
    MUIO_Radio,        /* STRPTR label, STRPTR *entries */
    MUIO_Slider,       /* STRPTR label, LONG min, LONG max */
    MUIO_String,       /* STRPTR label, LONG maxlen */
    MUIO_PopButton,    /* STRPTR imagespec */
    MUIO_HSpace,       /* LONG space */
    MUIO_VSpace,       /* LONG space */
    MUIO_HBar,         /* LONG space */
    MUIO_VBar,         /* LONG space */
    MUIO_MenustripNM,  /* struct NewMenu *nm, ULONG flags */
    MUIO_Menuitem,     /* STRPTR label, STRPTR shortcut, ULONG flags, ULONG data  */
    MUIO_BarTitle,     /* STRPTR label */
    MUIO_NumericButton /* STRPTR label, LONG min, LONG max, STRPTR format */
};

/* flag for MUIO_Menuitem */
#define MUIO_Menuitem_CopyStrings (1<<30)

/* flags for MUIO_Label type */
#define MUIO_Label_SingleFrame   (1<< 8)
#define MUIO_Label_DoubleFrame   (1<< 9)
#define MUIO_Label_LeftAligned   (1<<10)
#define MUIO_Label_Centered      (1<<11)
#define MUIO_Label_FreeVert      (1<<12)

/* flag for MUIO_MenustripNM */
#define MUIO_MenustripNM_CommandKeyCheck (1<<0) /* check for "localized" menu items such as "O\0Open" */

struct MUI_MinMax
{
    WORD MinWidth;
    WORD MinHeight;
    WORD MaxWidth;
    WORD MaxHeight;
    WORD DefWidth;
    WORD DefHeight;
};

/* special maximum dimension in case it is unlimited */
#define MUI_MAXMAX 10000 

/* Number of pens, the single definintion is below */
#define MPEN_COUNT 8

/* The mask for pens from MUI_ObtainPen() and a macro */
#define MUIPEN_MASK 0x0000ffff
#define MUIPEN(pen) ((pen) & MUIPEN_MASK)


/* Possible keyevents (user configurable) */
enum
{
    MUIKEY_RELEASE      = -2, /* this one is faked only, and thereforce not configurable */
    MUIKEY_NONE         = -1,
    MUIKEY_PRESS        = 0,
    MUIKEY_TOGGLE       = 1,
    MUIKEY_UP           = 2,
    MUIKEY_DOWN         = 3,
    MUIKEY_PAGEUP       = 4,
    MUIKEY_PAGEDOWN     = 5,
    MUIKEY_TOP          = 6,
    MUIKEY_BOTTOM       = 7,
    MUIKEY_LEFT         = 8,
    MUIKEY_RIGHT        = 9,
    MUIKEY_WORDLEFT     = 10,
    MUIKEY_WORDRIGHT    = 11,
    MUIKEY_LINESTART    = 12,
    MUIKEY_LINEEND      = 13,
    MUIKEY_GADGET_NEXT  = 14,
    MUIKEY_GADGET_PREV  = 15,
    MUIKEY_GADGET_OFF   = 16,
    MUIKEY_WINDOW_CLOSE = 17,
    MUIKEY_WINDOW_NEXT  = 18,
    MUIKEY_WINDOW_PREV  = 19,
    MUIKEY_HELP         = 20,
    MUIKEY_POPUP        = 21,
    MUIKEY_COUNT        = 22
};

/* The mask definitions of the above keys */
#define MUIKEYF_PRESS        (1<<MUIKEY_PRESS)
#define MUIKEYF_TOGGLE       (1<<MUIKEY_TOGGLE)
#define MUIKEYF_UP           (1<<MUIKEY_UP)
#define MUIKEYF_DOWN         (1<<MUIKEY_DOWN)
#define MUIKEYF_PAGEUP       (1<<MUIKEY_PAGEUP)
#define MUIKEYF_PAGEDOWN     (1<<MUIKEY_PAGEDOWN)
#define MUIKEYF_TOP          (1<<MUIKEY_TOP)
#define MUIKEYF_BOTTOM       (1<<MUIKEY_BOTTOM)
#define MUIKEYF_LEFT         (1<<MUIKEY_LEFT)
#define MUIKEYF_RIGHT        (1<<MUIKEY_RIGHT)
#define MUIKEYF_WORDLEFT     (1<<MUIKEY_WORDLEFT)
#define MUIKEYF_WORDRIGHT    (1<<MUIKEY_WORDRIGHT)
#define MUIKEYF_LINESTART    (1<<MUIKEY_LINESTART)
#define MUIKEYF_LINEEND      (1<<MUIKEY_LINEEND)
#define MUIKEYF_GADGET_NEXT  (1<<MUIKEY_GADGET_NEXT)
#define MUIKEYF_GADGET_PREV  (1<<MUIKEY_GADGET_PREV)
#define MUIKEYF_GADGET_OFF   (1<<MUIKEY_GADGET_OFF)
#define MUIKEYF_WINDOW_CLOSE (1<<MUIKEY_WINDOW_CLOSE)
#define MUIKEYF_WINDOW_NEXT  (1<<MUIKEY_WINDOW_NEXT)
#define MUIKEYF_WINDOW_PREV  (1<<MUIKEY_WINDOW_PREV)
#define MUIKEYF_HELP         (1<<MUIKEY_HELP)
#define MUIKEYF_POPUP        (1<<MUIKEY_POPUP)

struct MUI_CustomClass
{
    APTR mcc_UserData;                  /* freely usable */

    /* Zune/MUI had the following libraries opened for you */
    struct Library *mcc_UtilityBase;
    struct Library *mcc_DOSBase;
    struct Library *mcc_GfxBase;
    struct Library *mcc_IntuitionBase;

    struct IClass *mcc_Super;           /* the boopsi class' superclass */
    struct IClass *mcc_Class;           /* the boopsi class */

    /* the following stuff is private */

    struct Library *mcc_Module;         /* non-null if external class */
};

#undef MPEN_COUNT

typedef enum {
    MPEN_SHINE      = 0,
    MPEN_HALFSHINE  = 1,
    MPEN_BACKGROUND = 2,
    MPEN_HALFSHADOW = 3,
    MPEN_SHADOW     = 4,
    MPEN_TEXT       = 5,
    MPEN_FILL       = 6,
    MPEN_MARK       = 7,
    MPEN_COUNT      = 8,
} MPen;

typedef struct {
    MPen fg;
    MPen bg;
} MPenCouple;

typedef enum {
    PST_MUI,
    PST_CMAP,
    PST_RGB,
} PenSpecType;

struct MUI_PenSpec {
    union {
	char buf[32]; /* constraint PenSpec size */
	struct {
	    PenSpecType ps_Type;
	    union {
		MPen     mui;   /* MUI pen number */
		ULONG    cmap;  /* colormap entry */
		struct {
			  struct {
			     UWORD red;
			     UWORD green;
			     UWORD blue;
			     WORD pixel;
			  } rgb;
		    STRPTR   text;
		} c;
	    } v;
	} s;
    } u;
};

#define ps_penType u.s.ps_Type
#define ps_rgbColor u.s.v.c.rgb
#define ps_rgbText u.s.v.c.text
#define ps_mui u.s.v.mui
#define ps_cmap u.s.v.cmap

#ifndef _MUI_FRAME_H
#include "frame.h"
#endif

#ifndef _MUI_CLASSES_NOTIFY_H
#include "classes/notify.h"
#endif

#ifndef _MUI_CLASSES_AREA_H
#include "classes/area.h"
#endif

#ifndef _MUI_CLASSES_WINDOW_H
#include "classes/window.h"
#endif

#ifndef _MUI_CLASSES_GROUP_H
#include "classes/group.h"
#endif

#ifndef _MUI_CLASSES_RECTANGLE_H
#include "classes/rectangle.h"
#endif

#ifndef _MUI_CLASSES_TEXT_H
#include "classes/text.h"
#endif

#ifndef _MUI_CLASSES_NUMERIC_H
#include "classes/numeric.h"
#endif

#ifndef _MUI_CLASSES_SLIDER_H
#include "classes/slider.h"
#endif

#ifndef _MUI_CLASSES_STRING_H
#include "classes/string.h"
#endif

#ifndef _MUI_CLASSES_BOOPSI_H
#include "classes/boopsi.h"
#endif

#ifndef _MUI_CLASSES_PROP_H
#include "classes/prop.h"
#endif

#ifndef _MUI_CLASSES_SCROLLBAR_H
#include "classes/scrollbar.h"
#endif

#ifndef _MUI_CLASSES_REGISTER_H
#include "classes/register.h"
#endif

#ifndef _MUI_CLASSES_MENUITEM_H
#include "classes/menuitem.h"
#endif

#ifndef _MUI_CLASSES_DATATSPACE_H
#include "classes/dataspace.h"
#endif

#ifndef _MUI_CLASSES_VIRTGROUP_H
#include "classes/virtgroup.h"
#endif

#ifndef _MUI_CLASSES_SCROLLGROUP_H
#include "classes/scrollgroup.h"
#endif

#ifndef _MUI_CLASSES_SCROLLBUTTON_H
#include "classes/scrollbutton.h"
#endif

#ifndef _MUI_CLASSES_SEMAPHORE_H
#include "classes/semaphore.h"
#endif

#ifndef _MUI_CLASSES_BITMAP_H
#include "classes/bitmap.h"
#endif

#ifndef _MUI_CLASSES_BODYCHUNK_H
#include "classes/bodychunk.h"
#endif

#ifndef _MUI_CLASSES_LISTVIEW_H
#include "classes/listview.h"
#endif

#ifndef _MUI_CLASSES_LIST_H
#include "classes/list.h"
#endif

#ifndef _MUI_CLASSES_POPSTRING_H
#include "classes/popstring.h"
#endif

#ifndef _MUI_CLASSES_POPOBJECT_H
#include "classes/popobject.h"
#endif

#ifndef _MUI_CLASSES_CYCLE_H
#include "classes/cycle.h"
#endif

#ifndef _MUI_CLASSES_GAUGE_H
#include "classes/gauge.h"
#endif

#ifndef _MUI_CLASSES_IMAGE_H
#include "classes/image.h"
#endif

#ifndef _MUI_CLASSES_POPASL_H
#include "classes/popasl.h"
#endif

#ifndef _MUI_CLASSES_SETTINGSGROUP_H
#include "classes/settingsgroup.h"
#endif

#ifndef _MUI_CLASSES_ABOUTMUI_H
#include "classes/aboutmui.h"
#endif

#ifndef _MUI_CLASSES_CONFIGDATA_H
#include "classes/configdata.h"
#endif

#ifndef _MUI_CLASSES_IMAGEADJUST_H
#include "classes/imageadjust.h"
#endif

#ifndef _MUI_CLASSES_POPIMAGE_H
#include "classes/popimage.h"
#endif

/**************************************************************************
 Zune/MUI Image and Backgriund definition
**************************************************************************/
enum {
    /* configured by the user within the prefs programm */
    MUII_WindowBack     = 0,
    MUII_RequesterBack  = 1,
    MUII_ButtonBack     = 2,
    MUII_ListBack       = 3,
    MUII_TextBack       = 4,
    MUII_PropBack       = 5,
    MUII_PopupBack      = 6,
    MUII_SelectedBack   = 7,
    MUII_ListCursor     = 8,
    MUII_ListSelect     = 9,
    MUII_ListSelCur     = 10,
    MUII_ArrowUp        = 11,
    MUII_ArrowDown      = 12,
    MUII_ArrowLeft      = 13,
    MUII_ArrowRight     = 14,
    MUII_CheckMark      = 15,
    MUII_RadioButton    = 16,
    MUII_Cycle          = 17,
    MUII_PopUp          = 18,
    MUII_PopFile        = 19,
    MUII_PopDrawer      = 20,
    MUII_PropKnob       = 21,
    MUII_Drawer         = 22,
    MUII_HardDisk       = 23,
    MUII_Disk           = 24,
    MUII_Chip           = 25,
    MUII_Volume         = 26,
    MUII_RegisterBack   = 27,
    MUII_Network        = 28,
    MUII_Assign         = 29,
    MUII_TapePlay       = 30,
    MUII_TapePlayBack   = 31,
    MUII_TapePause      = 32,
    MUII_TapeStop       = 33,
    MUII_TapeRecord     = 34,
    MUII_GroupBack      = 35,
    MUII_SliderBack     = 36,
    MUII_SliderKnob     = 37,
    MUII_TapeUp         = 38,
    MUII_TapeDown       = 39,
    MUII_PageBack       = 40,
    MUII_ReadListBack   = 41,
    MUII_Count          = 42,

    /* direct color's and combinations */
    MUII_BACKGROUND     = 128,
    MUII_SHADOW         = 129,
    MUII_SHINE          = 130,
    MUII_FILL           = 131,
    MUII_SHADOWBACK     = 132,
    MUII_SHADOWFILL     = 133,
    MUII_SHADOWSHINE    = 134,
    MUII_FILLBACK       = 135,
    MUII_FILLSHINE      = 136,
    MUII_SHINEBACK      = 137,
    MUII_FILLBACK2      = 138,
    MUII_HSHINEBACK     = 139,
    MUII_HSHADOWBACK    = 140,
    MUII_HSHINESHINE    = 141,
    MUII_HSHADOWSHADOW  = 142,
    MUII_MARKSHINE      = 143,
    MUII_MARKHALFSHINE  = 144,
    MUII_MARKBACKGROUND = 145,
    MUII_LASTPAT        = 146
};

#include "prefs.h" /* PRIV */

/**************************************************************************
 For ARexx
**************************************************************************/
struct MUI_Command
{
    char        *mc_Name;
    char        *mc_Template;
    LONG         mc_Parameters;
    struct Hook *mc_Hook;
    LONG         mc_Reserved[5];
};

#define MC_TEMPLATE_ID ((STRPTR)~0)

#define MUI_RXERR_BADDEFINITION  -1
#define MUI_RXERR_OUTOFMEMORY    -2
#define MUI_RXERR_UNKNOWNCOMMAND -3
#define MUI_RXERR_BADSYNTAX      -4

#ifndef _MUI_MACROS_H
#include "macros.h"
#endif

#endif
