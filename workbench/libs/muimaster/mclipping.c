/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <exec/types.h>

#ifdef _AROS
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#endif

#include <zunepriv.h>
#include <areadata.h>
#include <renderinfo.h>

/* FIXME:
 * This has been brutally reverse-engineered from 'muimaster.library'.
 * I know I was not supposed to do that.
 */

#ifndef _AROS

static GSList *clip_list = NULL;
static GMemChunk *rectChunk = NULL;

static void
__zune_clipping_destroy (void)
{
    g_mem_chunk_destroy(rectChunk);
}

void
__zune_clipping_init (void)
{
    rectChunk = g_mem_chunk_create(GdkRectangle, 10, G_ALLOC_AND_FREE);
    g_atexit(__zune_clipping_destroy);
}


static GdkRectangle *
rectangle_new (WORD left, WORD top, WORD width, WORD height)
{
    GdkRectangle *rect = g_chunk_new0(GdkRectangle, rectChunk);
    rect->x = left;
    rect->y = top;
    rect->width = width;
    rect->height = height;
/*      g_print("rect %p\n", rect); */
    return rect;
}

static void
rectangle_destroy (GdkRectangle *rect)
{
/*  g_print("g_chunk_free %p %p\n", rect, rectChunk); */
    g_chunk_free(rect, rectChunk);
}

#else

/* check if region is entirely within given bounds */
int isRegionWithinBounds(struct Region *r, int left, int top, int width, int height)
{
	if ((left < r->bounds.MinX) && (left + width  - 1 > r->bounds.MaxX)
	 && (top  < r->bounds.MinY) && (top  + height - 1 > r->bounds.MaxY))
		return 1;

	return 0;
}

#endif


/*****************************************************************************

    NAME */
	AROS_LH5(APTR, MUI_AddClipping,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri,    A0),
	AROS_LHA(WORD                   , left,   D0),
	AROS_LHA(WORD                   , top,    D1),
	AROS_LHA(WORD                   , width,  D2),
	AROS_LHA(WORD                   , height, D3),

/*  LOCATION */
	struct Library *, MUIMasterBase, 28, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

#ifndef _AROS
    GdkRectangle *rect;

    g_return_val_if_fail(mri != NULL, NULL);

    rect = rectangle_new(left, top, width, height);

    clip_list = g_slist_prepend(clip_list, rect);
/*  g_print("clip list = %p\n", clip_list); */
    gdk_gc_set_clip_rectangle(mri->mri_RastPort, rect);
    return clip_list;
#else
    struct Rectangle rect;
    struct Region *r;

    if ((width >= MUI_MAXMAX) || (height >= MUI_MAXMAX))
        return (APTR)-1;

    if (mri->mri_rCount > 0)
    {
        if (isRegionWithinBounds(mri->mri_rArray[mri->mri_rCount-1], left, top, width, height))
            return (APTR)-1;
    }

    if ((r = NewRegion()) == NULL)
        return (APTR)-1;

    rect.MinX = left;
    rect.MinY = top;
    rect.MaxX = left + width  - 1;
    rect.MaxY = top  + height - 1;
    OrRectRegion(r, &rect);

    return MUI_AddClipRegion(mri, r);
#endif /* AROS */

    AROS_LIBFUNC_EXIT
}


/*
 * Remove current clip, then set previous clip or remove any clipping
 */
/*****************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_RemoveClipping,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri,    A0),
	AROS_LHA(APTR                   , handle, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 29, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

#ifndef _AROS
    GSList *node = (GSList *)handle;

    g_return_if_fail(mri != NULL);
    g_return_if_fail(handle != NULL);

/*  g_print("remove clip %p, num %d\n", handle, g_slist_length(clip_list)); */

    clip_list = g_slist_remove_link(clip_list, node);

    if (clip_list)
	gdk_gc_set_clip_rectangle(mri->mri_RastPort, (GdkRectangle *)clip_list->data);
    else
	gdk_gc_set_clip_rectangle(mri->mri_RastPort, NULL);

    rectangle_destroy((GdkRectangle *)node->data);
    g_slist_free_1(node);
#else
    MUI_RemoveClipRegion(mri, handle);
#endif /* AROS */

    AROS_LIBFUNC_EXIT
}


/***************************************************************************

    NAME */
	AROS_LH2(APTR, MUI_AddClipRegion,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri,    A0),
	AROS_LHA(struct Region         *, region, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 30, MUIMaster)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

#ifndef _AROS
#warning FIXME: not implemented
#else
    struct Window *w = mri->mri_Window;
    struct Layer  *l;
    APTR result;

    if (w != NULL)
        l = w->WLayer;
    else
        l = mri->mri_RastPort->Layer;

    if ((l == NULL) || (region == NULL) || (mri->mri_rCount == 10))
        return (APTR)-1;

    if (mri->mri_rCount != 0)
        /* NOTE: ignoring the result here... */
        AndRegionRegion(mri->mri_rArray[mri->mri_rCount-1], region);

    if ((w != NULL) && (mri->mri_Flags & MUIMRI_REFRESHMODE))
        EndRefresh(w, FALSE);

    if ((w != NULL) && !(w->Flags & WFLG_SIMPLE_REFRESH))
        LockLayerInfo(&w->WScreen->LayerInfo);

    result = InstallClipRegion(l, region);

    if ((w != NULL) && !(w->Flags & WFLG_SIMPLE_REFRESH))
        UnlockLayerInfo(&w->WScreen->LayerInfo);

    if ((w != NULL) && (mri->mri_Flags & MUIMRI_REFRESHMODE))
        BeginRefresh(w);

    mri->mri_rArray[mri->mri_rCount++] = region;

    return result;
#endif /* AROS */

    AROS_LIBFUNC_EXIT
}


/***************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_RemoveClipRegion,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri,    A0),
	AROS_LHA(APTR                   , handle, A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 31, MUIMaster)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

#ifndef _AROS
#warning FIXME: not implemented
#else
    struct Window *w = mri->mri_Window;
    struct Layer  *l;

    if (handle == (APTR)-1)
        return;

    if (w != NULL)
        l = w->WLayer;
    else
        l = mri->mri_RastPort->Layer;

    if (l == NULL)
        return;

    mri->mri_rCount--;

    if ((w != NULL) && (mri->mri_Flags & MUIMRI_REFRESHMODE))
        EndRefresh(w, FALSE);

    if ((w != NULL) && !(w->Flags & WFLG_SIMPLE_REFRESH))
        LockLayerInfo(&w->WScreen->LayerInfo);

    InstallClipRegion(l, (mri->mri_rCount > 0)
        ? mri->mri_rArray[mri->mri_rCount-1] : NULL);

    if ((w != NULL) && !(w->Flags & WFLG_SIMPLE_REFRESH))
        UnlockLayerInfo(&w->WScreen->LayerInfo);

    if ((w != NULL) && (mri->mri_Flags & MUIMRI_REFRESHMODE))
        BeginRefresh(w);

    DisposeRegion(mri->mri_rArray[mri->mri_rCount]);
    mri->mri_rArray[mri->mri_rCount] = NULL;
#endif /* AROS */

    AROS_LIBFUNC_EXIT
}


/***************************************************************************

    NAME */
	AROS_LH2(BOOL, MUI_BeginRefresh,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri,   A0),
	AROS_LHA(ULONG                  , flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 32, MUIMaster)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

#ifdef _AROS

    struct Window *w = mri->mri_Window;
    struct Layer  *l;

    if ((w == NULL) || !(w->Flags & WFLG_SIMPLE_REFRESH))
        return 0;

    l = w->WLayer;

    /* doesn't need refreshing */
    if (!(l->Flags & LAYERREFRESH))
        return 0;

    /* already refreshing */
    if (mri->mri_Flags & MUIMRI_REFRESHMODE)
        return 0;

    mri->mri_Flags |= MUIMRI_REFRESHMODE;
    LockLayerInfo(&w->WScreen->LayerInfo);
    BeginRefresh(w);
    return 1;

#endif

    AROS_LIBFUNC_EXIT
}


/***************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_EndRefresh,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri,   A0),
	AROS_LHA(ULONG                  , flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 33, MUIMaster)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

#ifdef _AROS

    struct Window *w = mri->mri_Window;

    if (w == NULL)
        return;

    EndRefresh(w, TRUE);
    UnlockLayerInfo(&w->WScreen->LayerInfo);
    mri->mri_Flags &= ~MUIMRI_REFRESHMODE;
    return;

#endif

    AROS_LIBFUNC_EXIT
}


/*** EOF ***/
