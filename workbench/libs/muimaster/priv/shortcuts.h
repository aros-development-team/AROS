/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __SHORTCUTS_H__
#define __SHORTCUTS_H__

#include <notifydata.h>
#include <areadata.h>

struct __zuneXFC2__
{
	struct MUI_NotifyData mnd;
	struct MUI_AreaData   mad;
};

#define muiNotifyData(obj) (&(((struct __zuneXFC2__ *)(obj))->mnd))
#define muiAreaData(obj)   (&(((struct __zuneXFC2__ *)(obj))->mad))

#define muiGlobalInfo(obj) (((struct __zuneXFC2__ *)(obj))->mnd.mnd_GlobalInfo)
#define muiUserData(obj)   (((struct __zuneXFC2__ *)(obj))->mnd.mnd_UserData)
#define muiRenderInfo(obj) (((struct __zuneXFC2__ *)(obj))->mad.mad_RenderInfo)

#define _app(obj)         (muiGlobalInfo(obj)->mgi_ApplicationObject) /* valid between MUIM_Setup/Cleanup */
#define _win(obj)         (muiRenderInfo(obj)->mri_WindowObject)      /* valid between MUIM_Setup/Cleanup */
#define _dri(obj)         (muiRenderInfo(obj)->mri_DrawInfo)          /* valid between MUIM_Setup/Cleanup */
#define _pens(obj)        (muiRenderInfo(obj)->mri_Pens)              /* valid between MUIM_Setup/Cleanup */
#define _window(obj)      (muiRenderInfo(obj)->mri_Window)            /* valid between MUIM_Show/Hide */
#define _rp(obj)          (muiRenderInfo(obj)->mri_RastPort)          /* valid between MUIM_Show/Hide */
#define _gc(obj)          (muiRenderInfo(obj)->mri_RastPort)          /* valid between MUIM_Show/Hide */
#define _left(obj)        (muiAreaData(obj)->mad_Box.Left)            /* valid during MUIM_Draw */
#define _top(obj)         (muiAreaData(obj)->mad_Box.Top)             /* valid during MUIM_Draw */
#define _width(obj)       (muiAreaData(obj)->mad_Box.Width)           /* valid during MUIM_Draw */
#define _height(obj)      (muiAreaData(obj)->mad_Box.Height)          /* valid during MUIM_Draw */
#define _right(obj)       (_left(obj)+_width(obj)-1)                  /* valid during MUIM_Draw */
#define _bottom(obj)      (_top(obj)+_height(obj)-1)                  /* valid during MUIM_Draw */
#define _addleft(obj)     (muiAreaData(obj)->mad_addleft  )           /* valid during MUIM_Draw */
#define _addtop(obj)      (muiAreaData(obj)->mad_addtop   )           /* valid during MUIM_Draw */
#define _subwidth(obj)    (muiAreaData(obj)->mad_subwidth )           /* valid during MUIM_Draw */
#define _subheight(obj)   (muiAreaData(obj)->mad_subheight)           /* valid during MUIM_Draw */
#define _mleft(obj)       (_left(obj)+_addleft(obj))                  /* valid during MUIM_Draw */
#define _mtop(obj)        (_top(obj)+_addtop(obj))                    /* valid during MUIM_Draw */
#define _mwidth(obj)      (_width(obj)-_subwidth(obj))                /* valid during MUIM_Draw */
#define _mheight(obj)     (_height(obj)-_subheight(obj))              /* valid during MUIM_Draw */
#define _mright(obj)      (_mleft(obj)+_mwidth(obj)-1)                /* valid during MUIM_Draw */
#define _mbottom(obj)     (_mtop(obj)+_mheight(obj)-1)                /* valid during MUIM_Draw */
#define _font(obj)        (muiAreaData(obj)->mad_Font)                /* valid between MUIM_Setup/Cleanup */
#define _minwidth(obj)    (muiAreaData(obj)->mad_MinMax.MinWidth)     /* valid between MUIM_Show/Hide */
#define _minheight(obj)   (muiAreaData(obj)->mad_MinMax.MinHeight)    /* valid between MUIM_Show/Hide */
#define _maxwidth(obj)    (muiAreaData(obj)->mad_MinMax.MaxWidth)     /* valid between MUIM_Show/Hide */
#define _maxheight(obj)   (muiAreaData(obj)->mad_MinMax.MaxHeight)    /* valid between MUIM_Show/Hide */
#define _defwidth(obj)    (muiAreaData(obj)->mad_MinMax.DefWidth)     /* valid between MUIM_Show/Hide */
#define _defheight(obj)   (muiAreaData(obj)->mad_MinMax.DefHeight)    /* valid between MUIM_Show/Hide */
#define _flags(obj)       (muiAreaData(obj)->mad_Flags)

#define _parent(obj)    (muiNotifyData(obj)->mnd_ParentObject)

#endif /* __SHORTCUTS_H__ */
