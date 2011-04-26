/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "vesagfxclass.h"
#include "compositing_intern.h"

#include <proto/exec.h>
#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>
#include <proto/utility.h>

#define MAX(a, b) a > b ? a : b
#define MIN(a, b) a < b ? a : b


static BOOL AndRectRect(struct _Rectangle *rect1,
    struct _Rectangle *rect2, struct _Rectangle *intersect)
{
    intersect->MinX = MAX(rect1->MinX, rect2->MinX);
    intersect->MinY = MAX(rect1->MinY, rect2->MinY);
    intersect->MaxX = MIN(rect1->MaxX, rect2->MaxX);
    intersect->MaxY = MIN(rect1->MaxY, rect2->MaxY);

    if ((intersect->MinX > intersect->MaxX) ||
        (intersect->MinY > intersect->MaxY))
        return FALSE;
    else
        return TRUE;
}


static struct StackBitMapNode *IsBitMapOnStack(struct PrivateData *compdata,
    OOP_Object *bm)
{
    struct StackBitMapNode *n = NULL;

    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->bm == bm)
            return n;
    }

    return NULL;
}


static VOID RecalculateVisibleRects(struct PrivateData *compdata)
{
    LONG lastscreenvisibleline = compdata->screenrect.MaxY;
    struct StackBitMapNode *n = NULL;

    ForeachNode(&compdata->bitmapstack, n)
    {
        /*  Stack bitmap bounding boxes equal screen bounding box taking into
            account topedge */
        SIPTR topedge;
        struct _Rectangle tmprect;
        OOP_Class *cl = OOP_OCLASS(n->bm);

        OOP_GetAttr(n->bm, aHidd_BitMap_TopEdge, &topedge);
        /* Copy screen rect */
        tmprect = compdata->screenrect;
        /* Set bottom and top values */
        tmprect.MinY = topedge;
        tmprect.MaxY = lastscreenvisibleline;
        /* Intersect both to make sure values are within screen limit */
        if (AndRectRect(&tmprect, &compdata->screenrect, &n->screenvisiblerect))
        {
            lastscreenvisibleline = n->screenvisiblerect.MinY - 1;
            n->isscreenvisible = TRUE;
        }
        else
            n->isscreenvisible = FALSE;

        D(bug("[Compositing] Bitmap %x, visible %d, (%d, %d) , (%d, %d)\n",
            n->bm, n->isscreenvisible,
            n->screenvisiblerect.MinX, n->screenvisiblerect.MinY,
            n->screenvisiblerect.MaxX, n->screenvisiblerect.MaxY));
    }
}


static BOOL TopBitMapChanged(struct PrivateData *compdata, OOP_Object *bm)
{
    OOP_Class *cl = OOP_OCLASS(bm);
    OOP_Object *sync = NULL;
    OOP_Object *pf = NULL;
    IPTR modeid, hdisp, vdisp, e, depth;

    OOP_GetAttr(bm, aHidd_BitMap_GfxHidd, &e);

    /* Sanity check */
    if (compdata->gfx != (OOP_Object *)e)
    {
        /* Top bitmap is not using the same driver as compositing. Fail. */
        D(bug("[Compositing] GfxHidd different than one used by compositing\n"));
        return FALSE;
    }

    /* Read display mode properties */
    OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);
    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[Compositing] Invalid ModeID\n"));
        return FALSE;
    }

    /* If the mode is already visible do nothing */
    if (modeid == compdata->screenmodeid)
        return TRUE;

    /* Get width and height of mode */
    struct pHidd_Gfx_GetMode __getmodemsg =
    {
        modeID:	modeid,
        syncPtr:	&sync,
        pixFmtPtr:	&pf,
    }, *getmodemsg = &__getmodemsg;

    getmodemsg->mID = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_GetMode);
    OOP_DoMethod(compdata->gfx, (OOP_Msg)getmodemsg);

    OOP_GetAttr(sync, aHidd_Sync_HDisp, &hdisp);
    OOP_GetAttr(sync, aHidd_Sync_VDisp, &vdisp);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);

    compdata->screenrect.MinX = 0;
    compdata->screenrect.MinY = 0;
    compdata->screenrect.MaxX = hdisp - 1;
    compdata->screenrect.MaxY = vdisp - 1;

    return TRUE;
}


static VOID RedrawBitmap(struct PrivateData *compdata,
    OOP_Object *bm, WORD x, WORD y, WORD width, WORD height)
{
    struct StackBitMapNode *n = NULL;

    /* Check if passed bitmap is in stack, ignore if not */
    if ((n = IsBitMapOnStack(compdata, bm)) == NULL)
        return;

    if (!n->isscreenvisible)
        return;

    OOP_Class *cl = OOP_OCLASS(bm);
    SIPTR leftedge, topedge;
    struct _Rectangle srcrect;
    struct _Rectangle srcindstrect;
    struct _Rectangle dstandvisrect;

    OOP_GetAttr(bm, aHidd_BitMap_LeftEdge, &leftedge);
    OOP_GetAttr(bm, aHidd_BitMap_TopEdge, &topedge);

    /* Rectangle in source bitmap coord system */
    srcrect.MinX = x;
    srcrect.MinY = y;
    srcrect.MaxX = x + width - 1;
    srcrect.MaxY = y + height - 1;

    /* Source bitmap rectangle in destination (screen) coord system */
    srcindstrect.MinX = srcrect.MinX + leftedge;
    srcindstrect.MaxX = srcrect.MaxX + leftedge;
    srcindstrect.MinY = srcrect.MinY + topedge;
    srcindstrect.MaxY = srcrect.MaxY + topedge;

    /* Find intersection of bitmap visible screen rect and srcindst rect */
    if (AndRectRect(&srcindstrect, &n->screenvisiblerect, &dstandvisrect)
        && compdata->refresh_hook != NULL)
    {
        /* Intersection is valid. Blit. */
        compdata->refresh_hook(compdata->gfx, bm,
            dstandvisrect.MinX - leftedge, dstandvisrect.MinY - topedge,
            dstandvisrect.MaxX - leftedge, dstandvisrect.MaxY - topedge);
    }
}


static VOID RedrawVisibleScreen(struct PrivateData *compdata)
{
    struct StackBitMapNode *n = NULL;
    ULONG first_used_line = compdata->screenrect.MaxY;

    /* Recalculate visible rects per screen */
    RecalculateVisibleRects(compdata);

    /* Refresh all bitmaps on stack */
    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->isscreenvisible)
        {
            OOP_Class *cl = OOP_OCLASS(n->bm);
            IPTR width, height;
            OOP_GetAttr(n->bm, aHidd_BitMap_Width, &width);
            OOP_GetAttr(n->bm, aHidd_BitMap_Height, &height);

            RedrawBitmap(compdata, n->bm, 0, 0, width, height);
            if (n->screenvisiblerect.MinY < first_used_line)
                first_used_line = n->screenvisiblerect.MinY;
        }
    }

    /* Clean up area revealed by drag */
    /* TODO: Find all areas which might have been revealed, not only top -
       This will happen when there are bitmaps of different sizes composited */
    if (first_used_line > compdata->first_used_line
        && compdata->refresh_hook != NULL)
        compdata->refresh_hook(compdata->gfx, NULL, 0,
            compdata->first_used_line, compdata->screenrect.MaxX,
            first_used_line - 1);

    compdata->first_used_line = first_used_line;
}


static VOID RedrawVisibleRect(struct PrivateData *compdata,
    WORD x, WORD y, WORD width, WORD height)
{
    struct StackBitMapNode *n = NULL;
    struct _Rectangle tmprect = {x, y, x + width - 1, y + height - 1}, slice;
    SIPTR leftedge, topedge;

    /* Draw a slice from each bitmap that intersects the redraw region */
    ForeachNode(&compdata->bitmapstack, n)
    {
        if (n->isscreenvisible)
        {
            OOP_Class *cl = OOP_OCLASS(n->bm);
            OOP_GetAttr(n->bm, aHidd_BitMap_LeftEdge, &leftedge);
            OOP_GetAttr(n->bm, aHidd_BitMap_TopEdge, &topedge);

            if (AndRectRect(&n->screenvisiblerect, &tmprect, &slice))
                RedrawBitmap(compdata, n->bm,
                    slice.MinX - leftedge,
                    slice.MinY - topedge,
                    slice.MaxX - slice.MinX + 1,
                    slice.MaxY - slice.MinY + 1);
        }
    }
}


static VOID PurgeBitMapStack(struct PrivateData *compdata)
{
    struct StackBitMapNode *curr, *next;

    ForeachNodeSafe(&compdata->bitmapstack, curr, next)
    {
        Remove((struct Node *)curr);
        FreeMem(curr, sizeof(struct StackBitMapNode));
    }

    NEWLIST(&compdata->bitmapstack);
}


/* PUBLIC METHODS */

OOP_Object *METHOD(Compositing, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    D(bug("[Compositing] New\n"));
    if (o)
    {
        struct PrivateData *compdata = OOP_INST_DATA(cl, o);

        NEWLIST(&compdata->bitmapstack);
        compdata->screenmodeid  = vHidd_ModeID_Invalid;
        InitSemaphore(&compdata->semaphore);

        compdata->gfx = (OOP_Object *)GetTagData(aHidd_Compositing_GfxHidd,
            0, msg->attrList);

        if (compdata->gfx != NULL)
        {
            /* Create GC object that will be used for drawing operations */
            compdata->gc = HIDD_Gfx_NewGC(compdata->gfx, NULL);
        }

        compdata->refresh_hook = (APTR)GetTagData(
            aHidd_Compositing_RefreshCallBack, 0, msg->attrList);

        if ((compdata->gfx == NULL) || (compdata->gc == NULL))
        {
            /* Creation failed */
            OOP_MethodID disposemid;
            disposemid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg)&disposemid);
            o = NULL;
        }
    }

    return o;
}


VOID METHOD(Compositing, Hidd_Compositing, BitMapStackChanged)
{
    struct HIDD_ViewPortData *vpdata;
    struct PrivateData *compdata = OOP_INST_DATA(cl, o);

    D(bug("[Compositing] BitMapStackChanged\n"));

    ObtainSemaphore(&compdata->semaphore);

    /* Free all items which are already on the list */
    PurgeBitMapStack(compdata);

    if (!msg->data)
    {
        ReleaseSemaphore(&compdata->semaphore);
        return; /* TODO: BLANK SCREEN */
    }

    /* Switch mode if needed */
    if (!TopBitMapChanged(compdata, msg->data->Bitmap))
    {
        /* Something bad happened. Yes, bitmap stack is already erased -
         * that's ok */
        D(bug("[Compositing] Failed to change top bitmap\n"));
        ReleaseSemaphore(&compdata->semaphore);
        return;
    }

    /* Copy bitmaps pointers to our stack */
    for (vpdata = msg->data; vpdata; vpdata = vpdata->Next)
    {
        /* Check if the passed bitmap can be composited together with screen
           bitmap */
        struct StackBitMapNode *n =
            AllocMem(sizeof(struct StackBitMapNode), MEMF_ANY | MEMF_CLEAR);
        n->bm = vpdata->Bitmap;
        n->isscreenvisible = FALSE;
        AddTail(&compdata->bitmapstack, (struct Node *)n);
    }

    /* Redraw bitmap stack */
    RedrawVisibleScreen(compdata);

    ReleaseSemaphore(&compdata->semaphore);
}


VOID METHOD(Compositing, Hidd_Compositing, BitMapRectChanged)
{
    struct PrivateData *compdata = OOP_INST_DATA(cl, o);

    ObtainSemaphoreShared(&compdata->semaphore);
    RedrawBitmap(compdata, msg->bm, msg->x, msg->y,
        msg->width, msg->height);
    ReleaseSemaphore(&compdata->semaphore);
}


VOID METHOD(Compositing, Hidd_Compositing, BitMapPositionChanged)
{
    struct PrivateData *compdata = OOP_INST_DATA(cl, o);
    D(bug("[Compositing] BitMapPositionChanged\n")); 

    ObtainSemaphoreShared(&compdata->semaphore);

    /* Check if passed bitmap is in stack, ignore if not */
    if (IsBitMapOnStack(compdata, msg->bm) != NULL)
    {
        /* Redraw bitmap stack */
        RedrawVisibleScreen(compdata);
    }

    ReleaseSemaphore(&compdata->semaphore);
}


VOID METHOD(Compositing, Hidd_Compositing, ValidateBitMapPositionChange)
{
    struct PrivateData *compdata = OOP_INST_DATA(cl, o);
    struct StackBitMapNode *n = NULL;
      D(bug("[Compositing] ValidateBitMapPositionChange %d,%d\n",
	         *msg->newxoffset,*msg->newyoffset));

    ObtainSemaphoreShared(&compdata->semaphore);

    /* Check if passed bitmap is in stack, ignore if not */
    if ((n = IsBitMapOnStack(compdata, msg->bm)) != NULL)
    {
        IPTR width, height;
        LONG limit;

        OOP_GetAttr(msg->bm, aHidd_BitMap_Width, &width);
        OOP_GetAttr(msg->bm, aHidd_BitMap_Height, &height);

        /* Check x position */
        limit = compdata->screenrect.MaxX + 1 - width;
        if (*(msg->newxoffset) > 0)
            *(msg->newxoffset) = 0;

        if (*(msg->newxoffset) < limit)
            *(msg->newxoffset) = limit;

        /* Check y position */
        limit = compdata->screenrect.MaxY + 1 - height;
        if (*(msg->newyoffset) > compdata->screenrect.MaxY - 14) /* Limit for drag */
            *(msg->newyoffset) = compdata->screenrect.MaxY - 14;

        if (*(msg->newyoffset) < limit) /* Limit for scroll */
            *(msg->newyoffset) = limit;
    }

    ReleaseSemaphore(&compdata->semaphore);
}


VOID METHOD(Compositing, Hidd_Compositing, DisplayRectChanged)
{
    struct PrivateData *compdata = OOP_INST_DATA(cl, o);

    ObtainSemaphoreShared(&compdata->semaphore);
    RedrawVisibleRect(compdata, msg->x, msg->y,
        msg->width, msg->height);
    ReleaseSemaphore(&compdata->semaphore);
}

