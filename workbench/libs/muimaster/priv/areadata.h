#ifndef __AREADATA_H__
#define __AREADATA_H__

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#include <minmax.h>
#include <eventhandler.h>

/******************************************************************************/
/******************************************************************************/

struct MUI_RenderInfo;
struct ZText;

struct MUI_AreaData
{
    struct MUI_RenderInfo *mad_RenderInfo; /* RenderInfo for this object */
    LONG               mad_FontPreset;     /* MUI font wanted -- PRIVATE */
    struct TextFont   *mad_Font;           /* Font (to draw) */
    struct MUI_MinMax  mad_MinMax;         /* min/max/default sizes */
    struct IBox        mad_Box;            /* position and dimension */
    BYTE               mad_addleft;        /* frame & innerspacing left offset */
    BYTE               mad_addtop;         /* frame & innerspacing top offset  */
    BYTE               mad_subwidth;       /* frame & innerspacing add. width  */
    BYTE               mad_subheight;      /* frame & innerspacing add. height */
    ULONG              mad_Flags;          /* see definitions below */
    /* ... private data follows ... */
    WORD               mad_InputMode;      /* how to react to events */
    WORD               mad_Frame;          /* frame setting -- private */
    STRPTR             mad_FrameTitle;     /* for groups. Req. mad_Frame > 0 */
    WORD               mad_HardHeight;     /* if harcoded dim (see flags)  */
    WORD               mad_HardWidth;      /* if harcoded dim (see flags)  */
#warning FIXME: mad_Background
#if 0
    struct MUI_ImageSpec *mad_Background;  /* bg setting */
    struct MUI_ImageSpec *mad_SelBack;     /* selected state background */
#endif
    WORD               mad_HardILeft;      /* hardcoded inner values */
    WORD               mad_HardIRight;
    WORD               mad_HardITop;
    WORD               mad_HardIBottom;
    UWORD              mad_HorizWeight;    /* weight values for layout */
    UWORD              mad_VertWeight;
    STRPTR             mad_ShortHelp;      /* bubble help */
    struct MUI_EventHandlerNode mad_ehn;
#ifndef _AROS
    ULONG              mad_PreTimeout_id; /* to wait before installing timer */
    ULONG              mad_Timeout_id;    /* intuiticks timer */
#endif
    ULONG              mad_Timeval;       /* just to trigger notifications */
    struct MUI_EventHandlerNode mad_ccn;  /* gross hack for control char */
    Object            *mad_ContextMenu;   /* menu strip */
    struct ZText      *mad_TitleText;     /* frame title */
    TEXT               mad_ControlChar;   /* key shortcut */
};


/******************************************************************************/
/******************************************************************************/

#define MADF_DRAWOBJECT        (1<< 0) /* completely redraw yourself */
#define MADF_DRAWUPDATE        (1<< 1) /* only update yourself */
#define MADF_FIXHEIGHT         (1<< 2)
#define MADF_FIXWIDTH          (1<< 3)
#define MADF_MAXHEIGHT         (1<< 4)
#define MADF_MAXWIDTH          (1<< 5)
#define MADF_FILLAREA          (1<< 6)
#define MADF_INNERLEFT         (1<< 7)
#define MADF_INNERRIGHT        (1<< 8)
#define MADF_INNERTOP          (1<< 9)
#define MADF_INNERBOTTOM       (1<< 10)
#define MADF_FRAMEPHANTOM      (1<< 11)
#define MADF_SELECTED          (1<< 12)
#define MADF_PRESSED           (1<< 13)
#define MADF_SHOWME            (1<< 14)
#define MADF_SHOWSELSTATE      (1<< 15)
#define MADF_CANDRAW           (1<< 16)
#define MADF_SETUP             (1<< 17)
#define MADF_MAXSIZE           (1<< 18)
#define MADF_DRAWALL           (1<< 19)
#define MADF_CYCLECHAIN        (1<< 20)
#define MADF_ACTIVE            (1<< 21)

#define DRAW_BG_RECURSIVE (1<<1)

#define _vweight(obj)  (muiAreaData(obj)->mad_VertWeight)
#define _hweight(obj)  (muiAreaData(obj)->mad_HorizWeight)

void __area_finish_minmax(Object *obj, struct MUI_MinMax *minmax);


#endif
