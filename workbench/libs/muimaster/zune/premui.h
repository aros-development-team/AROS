#ifndef __ZUNE_MUI_H_ADAPTATOR_H__
#define __ZUNE_MUI_H_ADAPTATOR_H__

/* included only by Zune apps, not Zune core !!! */
#ifdef _ZUNE_CORE
#error Zune core must not include this file
#endif

/*
 * defines marked IntuitionPlagued are too dependant of Intuition
 * they are replaced by too GDK-dependant features :)))
 */


/* BEGIN fake-inclusions */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif
#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif
#ifndef INTUITION_SCREENS_H
#include <intuition/screens.h>
#endif
#ifndef CLIB_INTUITION_PROTOS_H
#include <clib/intuition_protos.h>
#endif

/* END fake-inclusions */

/* BEGIN MUIP-hiding */

#define MUIP_NoNotifySet MUIP_NoNotifySet_Buggy
#define MUIP_HandleEvent MUIP_HandleEvent_IntuitionPlagued

/* END MUIP-hiding */

/* BEGIN pre-RenderInfo-fix */

#define MUI_AreaData MUI_AreaDataOrig
#define MUI_RenderInfo MUI_RenderInfoOrig

#define MUIP_Setup MUIP_Setup_RenderInfoOrig

struct Window;
struct RastPort;

/* END pre-RenderInfo-fix */

#include <zune/rootclass.h>
#include <zune/tagitem.h>
#include <zune/mui.h>
#include <gdk/gdktypes.h>

/* BEGIN MUIP-redefinitions */

#undef MUIP_NoNotifySet
#undef MUIP_HandleEvent

struct  MUIP_NoNotifySet                    { ULONG MethodID; ULONG attr; ULONG val; };
struct  MUIP_HandleEvent                    { ULONG MethodID; GdkEvent *imsg; LONG muikey; };

/* END MUIP-redefinitions */

/* BEGIN zune-MUIPEN-hack */

#undef MUIPEN
#define MUIPEN(pen) (pen)

/* END zune-MUIPEN-hack */

/* BEGIN RenderInfo-fix */

#undef MUI_RenderInfo
#undef MUI_AreaData

/*
 * inclusion of real MUI_RenderInfo and MUI_AreaData here
 */
struct MUI_RenderInfo
{
    Object          *mri_WindowObject;  /* valid between MUIM_Setup/MUIM_Cleanup */
    struct DrawInfo *mri_DrawInfo;
    ULONG            mri_Flags;         /* valid between MUIM_Setup/MUIM_Cleanup */
    GdkWindow       *mri_Window;        /* valid between MUIM_Show/MUIM_Hide */
    gulong          *mri_Pens;
    GdkGC           *mri_GC;
    GdkColormap     *mri_Colormap;
    GdkVisual       *mri_Visual;
    gint             mri_ScreenWidth;
    gint             mri_ScreenHeight;
    /* boring stuff follows ... Refer to core includes if curious */
};

struct MUI_AreaData
{
    struct MUI_RenderInfo *mad_RenderInfo;     /* RenderInfo for this object */
    ULONG priv7;
    GdkFont           *mad_Font;           /* Font */
    struct MUI_MinMax  mad_MinMax;         /* min/max/default sizes */
    struct IBox        mad_Box;            /* position and dimension */
    BYTE               mad_addleft;        /* frame & innerspacing left offset */
    BYTE               mad_addtop;         /* frame & innerspacing top offset  */
    BYTE               mad_subwidth;       /* frame & innerspacing add. width  */
    BYTE               mad_subheight;      /* frame & innerspacing add. height */
    ULONG              mad_Flags;          /* see definitions below */
    /* boring stuff follows ... Refer to core includes if curious */
};


#undef MUIP_Setup
struct MUIP_Setup { ULONG MethodID; struct MUI_RenderInfo *RenderInfo; }; /* Custom Class */

#ifndef MUI_NOSHORTCUTS
#undef _win
#undef _dri
#undef _screen
#undef _pens
#undef _window
#undef _rp
#endif

struct __zuneXFC2__
{
	struct MUI_NotifyData mnd;
	struct MUI_AreaData   mad;
};

#undef muiRenderInfo
#define muiRenderInfo(obj) (((struct __zuneXFC2__ *)(obj))->mad.mad_RenderInfo)

#ifndef MUI_NOSHORTCUTS

#define _win(obj)         (muiRenderInfo(obj)->mri_WindowObject)      /* valid between MUIM_Setup/Cleanup */
#define _dri(obj)         (muiRenderInfo(obj)->mri_DrawInfo)          /* valid between MUIM_Setup/Cleanup */
#define _pens(obj)        (muiRenderInfo(obj)->mri_Pens)              /* valid between MUIM_Setup/Cleanup */
#define _window(obj)      (muiRenderInfo(obj)->mri_Window)            /* valid between MUIM_Show/Hide */
#define _rp(obj)          (muiRenderInfo(obj)->mri_GC)          /* valid between MUIM_Show/Hide */
#define _gc(obj)          (muiRenderInfo(obj)->mri_GC)          /* valid between MUIM_Show/Hide */

#define _parent(obj)    (muiNotifyData(obj)->mnd_ParentObject)

#endif /* MUI_NOSHORTCUTS */

/* END RenderInfo-fix */

/* BEGIN macros fixes */

#undef SimpleButton
#define SimpleButton(label) MUI_MakeObject(MUIO_Button,GPOINTER_TO_UINT(label))

#undef CLabel
#define CLabel(label)  MUI_MakeObject(MUIO_Label,_U(label),MUIO_Label_Centered)

/* END macros fixes */


#endif /* __ZUNE_MUI_H_ADAPTATOR_H__ */
