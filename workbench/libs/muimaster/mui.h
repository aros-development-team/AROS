/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef LIBRARIES_MUI_H
#define LIBRARIES_MUI_H

#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef GRAPHICS_GRAPHICS_H
#   include <graphics/gfx.h>
#endif
#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif

#ifdef __AROS__
#   ifndef AROS_ASMCALL_H
#       include <aros/asmcall.h>
#   endif
#   define SAVEDS
#else
#   include "support_amigaos.h"
#endif


#include "mui_identifiers.h"

#define MUIMASTER_NAME "muimaster.library"
#define MUIMASTER_VMIN    41
#define MUIMASTER_VLATEST 41

/* 
    With the following define a typical dispatcher will looks like this:
    BOOPSI_DISPATCHER(IPTR,IconWindow_Dispatcher,cl,obj,msg)
*/
#define BOOPSI_DISPATCHER(rettype,name,cl,obj,msg) \
    AROS_UFH3(SAVEDS rettype, name,\
        AROS_UFHA(Class  *, cl,  A0),\
        AROS_UFHA(Object *, obj, A2),\
        AROS_UFHA(Msg     , msg, A1)) {AROS_USERFUNC_INIT;
#define BOOPSI_DISPATCHER_END AROS_USERFUNC_EXIT;}
#define BOOPSI_DISPATCHER_PROTO(rettype,name,cl,obj,msg) \
    AROS_UFP3(SAVEDS rettype, name,\
        AROS_UFPA(Class  *, cl,  A0),\
        AROS_UFPA(Object *, obj, A2),\
        AROS_UFPA(Msg     , msg, A1))


/* START PRIV */

/* This structure is used for the internal classes */

struct __MUIBuiltinClass {
    CONST_STRPTR name;
    CONST_STRPTR supername;
    ULONG        datasize;

#ifndef __AROS__
    ULONG	   (*dispatcher)();
#else
    AROS_UFP3(IPTR, (*dispatcher),
        AROS_UFPA(Class  *,  cl, A0),
        AROS_UFPA(Object *, obj, A2),
        AROS_UFPA(Msg     , msg, A1));
#endif
};

/* END PRIV */

#if defined(MUIMASTER_YES_INLINE_STDARG) && \
    !defined(NO_INLINE_STDARG)           && \
    !defined(__SASC)

#define MUIOBJMACRO_START(class)   \
({                                 \
     ClassID __class = class;      \
     enum { __ismuiobjmacro = 1 }; \
     IPTR __tags[] = {0

#define BOOPSIOBJMACRO_START(class) \
({                                  \
     Class  *__class = class;       \
     enum { __ismuiobjmacro = 0 };  \
     IPTR __tags[] = {0

#define OBJMACRO_END                                                            \
     TAG_DONE};                                                                 \
     (                                                                          \
         __ismuiobjmacro                                                        \
         ? MUI_NewObjectA((ClassID)__class, (struct TagItem *)(__tags + 1)) \
         : NewObjectA((Class *)__class, NULL, (struct TagItem *)(__tags + 1))   \
     );                                                                         \
})

#else

#define MUIOBJMACRO_START(class) MUI_NewObject(class
#define BOOPSIOBJMACRO_START(class) NewObject(class, NULL

#define OBJMACRO_END TAG_DONE)

#endif

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
#ifndef __AROS__

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
__asm BOOL MUI_Layout(register __a0 Object *obj, register __d0 LONG left, register __d1 LONG top, register __d2 LONG width, register __d3 LONG height, register __d4 ULONG flags);
__asm Object *MUI_MakeObjectA(register __d0 LONG type, register __a0 ULONG *params);
__asm Object *MUI_NewObjectA(register __a0 char *classname,register __a1 struct TagItem *tags);
__asm LONG MUI_ObtainPen(register __a0 struct MUI_RenderInfo *mri, register __a1 struct MUI_PenSpec *spec, register __d0 ULONG flags);
__asm VOID MUI_Redraw(register __a0 Object *obj, register __d0 ULONG flags);
__asm VOID MUI_RejectIDCMP(register __a0 Object *obj, register __d0 ULONG flags);
__asm VOID MUI_ReleasePen(register __a0 struct MUI_RenderInfo *mri, register __d0 LONG pen);
__asm VOID MUI_RemoveClipping(register __a0 struct MUI_RenderInfo *mri, register __a1 APTR handle);
__asm VOID MUI_RemoveClipRegion(register __a0 struct MUI_RenderInfo *mri, register __a1 APTR handle);
__asm LONG MUI_RequestA(register __d0 APTR app, register __d1 APTR win, register __d2 LONGBITS flags, register __a0 CONST_STRPTR title, register __a1 CONST_STRPTR gadgets, register __a2 CONST_STRPTR format, register __a3 APTR params);
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

#endif /* !__AROS__ */


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
    MUIO_NumericButton,/* STRPTR label, LONG min, LONG max, STRPTR format */
    
    MUIO_CoolButton = 111, /* STRPTR label, APTR CoolImage, ULONG flags */
    MUIO_ImageButton,      /* CONST_STRPTR label, CONST_STRPTR imagePath */
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

/* flag for MUI_CoolButton  */
#define MUIO_CoolButton_CoolImageID (1<<0)

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

typedef enum {
    PST_MUI = 'm',
    PST_CMAP = 'p',
    PST_RGB = 'r',
    PST_SYS = 's',
} PenSpecType;

/* MUI_PenSpec is a an ascii spec like this:

   "m5"     	    	    	(mui pen #5)
   "p123"   	    	    	(cmap entry #123)
   "rFFFFFFFF,00000000,00000000 (rgb #FF0000)
   "s3"                         (system pen #3)
   
   It needs to be like this, because for example nlist has
   default penspecs in it's source encoded like above which
   it directly passes to MUI_ObtainBestPen */
   
struct MUI_PenSpec
{
    UBYTE ps_buf[32];
};


struct MUI_FrameSpec
{
    char buf[32];
};

struct MUI_RGBcolor
{
    ULONG red;
    ULONG green;
    ULONG blue;
};

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

#ifndef _MUI_CLASSES_DATASPACE_H
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

#ifndef _MUI_CLASSES_CHUNKYIMAGE_H
#include "classes/chunkyimage.h"
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

#ifndef _MUI_CLASSES_IMAGEDISPLAY_H
#include "classes/imagedisplay.h"
#endif

#ifndef _MUI_CLASSES_POPASL_H
#include "classes/popasl.h"
#endif

#ifndef _MUI_CLASSES_SETTINGSGROUP_H
#include "classes/settingsgroup.h"
#endif

#ifndef _MUI_CLASSES_SETTINGS_H
#include "classes/settings.h"
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

#ifndef _MUI_CLASSES_SCALE_H
#include "classes/scale.h"
#endif

#ifndef _MUI_CLASSES_RADIO_H
#include "classes/radio.h"
#endif

#ifndef _MUI_CLASSES_ICONLIST_H
#include "classes/iconlist.h"
#endif

#ifndef _MUI_CLASSES_ICONLISTVIEW_H
#include "classes/iconlistview.h"
#endif

#ifndef _MUI_CLASSES_BALANCE_H
#include "classes/balance.h"
#endif

#ifndef _MUI_CLASSES_PENDISPLAY_H
#include "classes/pendisplay.h"
#endif

#ifndef _MUI_CLASSES_PENADJUST_H
#include "classes/penadjust.h"
#endif

#ifndef _MUI_CLASSES_POPPEN_H
#include "classes/poppen.h"
#endif

#ifndef _MUI_CLASSES_COLORFIELD_H
#include "classes/colorfield.h"
#endif

#ifndef _MUI_CLASSES_COLORADJUST_H
#include "classes/coloradjust.h"
#endif

#ifndef _MUI_CLASSES_MCCPREFS_H
#include "classes/mccprefs.h"
#endif

#ifndef _MUI_CLASSES_FRAMEADJUST_H
#include "classes/frameadjust.h"
#endif

#ifndef _MUI_CLASSES_FRAMEDISPLAY_H
#include "classes/framedisplay.h"
#endif

#ifndef _MUI_CLASSES_POPFRAME_H
#include "classes/popframe.h"
#endif

#ifndef _MUI_CLASSES_VOLUMELIST_H
#include "classes/volumelist.h"
#endif

#ifndef _MUI_CLASSES_DIRLIST_H
#include "classes/dirlist.h"
#endif

#ifndef _MUI_CLASSES_NUMERICBUTTON_H
#include "classes/numericbutton.h"
#endif

#ifndef _MUI_CLASSES_POPLIST_H
#include "classes/poplist.h"
#endif

#ifndef _MUI_CLASSES_POPSCREEN_H
#include "classes/popscreen.h"
#endif

#ifndef _MUI_CLASSES_CRAWLING_H
#include "classes/crawling.h"
#endif

#ifndef _MUI_CLASSES_LEVELMETER_H
#include "classes/levelmeter.h"
#endif

#ifndef _MUI_CLASSES_KNOB_H
#include "classes/knob.h"
#endif

/**************************************************************************
 Zune/MUI Image and Background definition
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

#endif /* LIBRARIES_MUI_H */
