/*
    Copyright  2002-2010, The AROS Development Team. All rights reserved.
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
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
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

#ifndef __AROS__
#define MUIMASTER_NAME "zunemaster.library"
#define MUIMASTER_VMIN    0
#define MUIMASTER_VLATEST 0
#else
#define MUIMASTER_NAME "muimaster.library"
#define MUIMASTER_VMIN    11
#define MUIMASTER_VLATEST 19
#endif

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

#define MUIOBJMACRO_START(class) (APTR) \
({                                      \
     ClassID __class = (ClassID) class; \
     enum { __ismuiobjmacro = 1 };      \
     IPTR __tags[] = {0

#define BOOPSIOBJMACRO_START(class) (APTR) \
({                                         \
     Class *__class = (Class *) class;     \
     enum { __ismuiobjmacro = 0 };         \
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

#ifdef __amigaos4__
#   define MUIOBJMACRO_START(class) (IZuneMaster->MUI_NewObject)(class
#   define BOOPSIOBJMACRO_START(class) (IIntuition->NewObject)(class, NULL
#else
#   define MUIOBJMACRO_START(class) MUI_NewObject(class
#   define BOOPSIOBJMACRO_START(class) NewObject(class, NULL
#endif

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

/* These cannot be enums, since they will
 * not be passed properly in varadic
 * functions by some compilers.
 */
#define MUIV_Font_Inherit  ((IPTR)0)
#define MUIV_Font_Normal   ((IPTR)-1)
#define MUIV_Font_List     ((IPTR)-2)
#define MUIV_Font_Tiny     ((IPTR)-3)
#define MUIV_Font_Fixed    ((IPTR)-4)
#define MUIV_Font_Title    ((IPTR)-5)
#define MUIV_Font_Big      ((IPTR)-6)
#define MUIV_Font_Button   ((IPTR)-7)
#define MUIV_Font_Knob     ((IPTR)-8)
#define MUIV_Font_NegCount ((IPTR)-9)

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

#ifndef _MUI_CLASSES_WINDOW_H
#include "classes/window.h"
#endif

#ifndef _MUI_CLASSES_AREA_H
#include "classes/area.h"
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

#ifndef _MUI_CLASSES_FLOATTEXT_H
#include "classes/floattext.h"
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

#ifndef _MUI_CLASSES_DTPIC_H
#include "classes/dtpic.h"
#endif

#ifndef _MUI_CLASSES_PALETTE_H
#include "classes/palette.h"
#endif

#ifndef _MUI_CLASSES_TITLE_H
#include "classes/title.h"
#endif

/**************************************************************************
 Zune/MUI Image and Background definition
**************************************************************************/
/* configured by the user within the prefs programm */
#define MUII_WindowBack        0UL
#define MUII_RequesterBack     1UL
#define MUII_ButtonBack        2UL
#define MUII_ListBack          3UL
#define MUII_TextBack          4UL
#define MUII_PropBack          5UL
#define MUII_PopupBack         6UL
#define MUII_SelectedBack      7UL
#define MUII_ListCursor        8UL
#define MUII_ListSelect        9UL
#define MUII_ListSelCur        10UL
#define MUII_ArrowUp           11UL
#define MUII_ArrowDown         12UL
#define MUII_ArrowLeft         13UL
#define MUII_ArrowRight        14UL
#define MUII_CheckMark         15UL
#define MUII_RadioButton       16UL
#define MUII_Cycle             17UL
#define MUII_PopUp             18UL
#define MUII_PopFile           19UL
#define MUII_PopDrawer         20UL
#define MUII_PropKnob          21UL
#define MUII_Drawer            22UL
#define MUII_HardDisk          23UL
#define MUII_Disk              24UL
#define MUII_Chip              25UL
#define MUII_Volume            26UL
#define MUII_RegisterBack      27UL
#define MUII_Network           28UL
#define MUII_Assign            29UL
#define MUII_TapePlay          30UL
#define MUII_TapePlayBack      31UL
#define MUII_TapePause         32UL
#define MUII_TapeStop          33UL
#define MUII_TapeRecord        34UL
#define MUII_GroupBack         35UL
#define MUII_SliderBack        36UL
#define MUII_SliderKnob        37UL
#define MUII_TapeUp            38UL
#define MUII_TapeDown          39UL
#define MUII_PageBack          40UL
#define MUII_ReadListBack      41UL
#define MUII_Count             42UL

/* direct color's and combinations */
#define MUII_BACKGROUND        128UL
#define MUII_SHADOW            129UL
#define MUII_SHINE             130UL
#define MUII_FILL              131UL
#define MUII_SHADOWBACK        132UL
#define MUII_SHADOWFILL        133UL
#define MUII_SHADOWSHINE       134UL
#define MUII_FILLBACK          135UL
#define MUII_FILLSHINE         136UL
#define MUII_SHINEBACK         137UL
#define MUII_FILLBACK2         138UL
#define MUII_HSHINEBACK        139UL
#define MUII_HSHADOWBACK       140UL
#define MUII_HSHINESHINE       141UL
#define MUII_HSHADOWSHADOW     142UL
#define MUII_MARKSHINE         143UL
#define MUII_MARKHALFSHINE     144UL
#define MUII_MARKBACKGROUND    145UL
#define MUII_LASTPAT           146UL


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
