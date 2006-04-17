//
// pmtopography.h
//
// PopupMenu Library - Topographical Menu Map
//
// Copyright (C)2000 Henrik Isaksson <henrik@boing.nu>
// All Rights Reserved.
//

#include "pmtopography.h"

#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>

//
// PM_AddTopographicRegion - add a rectangular region of height h to the topographic map.
//
BOOL PM_AddTopographicRegion(PMTRList *lst, WORD l, WORD t, WORD r, WORD b, WORD h)
{
    PMTR rect, *tmprect;
    PMTR *worknode, *nextnode;
    BOOL fail=FALSE;

    rect.Left=l;
    rect.Top=t;
    rect.Right=r;
    rect.Bottom=b;
    rect.Height=h;
    rect.n.Length=sizeof(PMTR);

    //kprintf("Adding topographic region: %ld %ld %ld %ld, height: %ld\n", l, t, r, b, h);

    //
    // Step 1: Remove all parts in l that would overlap rect. 
    //
    worknode = (PMTR *)(lst->mlh_Head); /* First node */
    while((nextnode = (PMTR *)PM_NextNode(worknode))) {
        if(PM_RegionOverlap(worknode, &rect)) {
            PM_UnlinkRegion(lst, worknode);
            PM_SubTopographicRects(lst, worknode, &rect);
            PM_FreeTopographicNode(worknode);
        }
        worknode = nextnode;
    }
    
    //
    // Step 2: Add rect to l.
    //
    tmprect=PM_CopyTopographicNode(&rect);
    if(tmprect) {
        PM_AddRegionToList(lst, tmprect);
    } else fail=TRUE;

    return (!(BOOL)fail);
}

//
// PM_CopyTopographicNode - copy a PMTopographicRect, or allocate a new if A is NULL.
//
PMTR *PM_CopyTopographicNode(PMTR *A)
{
    PMTR *newnode=NULL;

    newnode=(PMTR *)AllocVec(sizeof(PMTR), MEMF_ANY);
    if(newnode) {
        if(A) {
            CopyMem(A, newnode, sizeof(PMTR));
        }
    }

    return newnode;
}

//
// PM_SubTopographicRects - like PM_SubRects but preserves height
//
BOOL PM_SubTopographicRects(PMTRList *dest, PMTR *worknode, PMTR *rect)
{
    PMTR tmprect, *rectptr;
    BOOL fail=FALSE;

    tmprect.Height=worknode->Height;

    tmprect.Left=worknode->Left;
    tmprect.Right=worknode->Right;


    tmprect.n.Length=sizeof(tmprect);

    if(rect->Top>worknode->Top && rect->Top<worknode->Bottom) {
        tmprect.Top=worknode->Top;
        tmprect.Bottom=rect->Top;

        rectptr=PM_CopyTopographicNode(&tmprect);
        if(rectptr) PM_AddToList((PMDList *)dest, (PMNode *)rectptr);
        else fail=TRUE;
    }

    if(rect->Bottom>worknode->Top && rect->Bottom<worknode->Bottom) {
        tmprect.Top=rect->Bottom;
        tmprect.Bottom=worknode->Bottom;

        rectptr=PM_CopyTopographicNode(&tmprect);
        if(rectptr) PM_AddToList((PMDList *)dest, (PMNode *)rectptr);
        else fail=TRUE;
    }

    tmprect.Top=MAX(rect->Top, worknode->Top);
    tmprect.Bottom=MIN(rect->Bottom, worknode->Bottom);

    if(rect->Left>worknode->Left && rect->Left<worknode->Right) {
        tmprect.Left=worknode->Left;
        tmprect.Right=rect->Left;

        rectptr=PM_CopyTopographicNode(&tmprect);
        if(rectptr) PM_AddToList((PMDList *)dest, (PMNode *)rectptr);
        else fail=TRUE;
    }

    if(rect->Right>worknode->Left && rect->Right<worknode->Right) {
        tmprect.Left=rect->Right;
        tmprect.Right=worknode->Right;

        rectptr=PM_CopyTopographicNode(&tmprect);
        if(rectptr) PM_AddShadowToList((PMDList *)dest, (PMNode *)rectptr);
        else fail=TRUE;
    }

    return (!(BOOL)fail);
}

//
// PM_MapShadow
//
BOOL PM_MapShadow(PMTRList *top, PMSRList *delta,
    WORD l, WORD t, WORD r, WORD b,
    UWORD menulevel, UWORD direction)
{
    PMTR *topnode, *nexttopnode;
    PMSR rect, shrect, *newnode;
    BOOL fail=FALSE;
    UWORD dx, dy, ss;

    shrect.Left=l;
    shrect.Top=t;
    shrect.Right=r;
    shrect.Bottom=b;
    
    ss=menulevel*2+1+4;
    
    if(direction&PMSHADOW_HORIZ) {
        shrect.Right=l+ss;
    }

    if(direction&PMSHADOW_VERT) {
        shrect.Bottom=t+ss;
    }
    
    topnode = (PMTR *)(top->mlh_Head); /* First node */
    while((nexttopnode = (PMTR *)PM_NextNode(topnode))) {

            dx=dy=0;

            //kprintf("Mapping shadow to region: %ld %ld %ld %ld, height: %ld\n", topnode->Left, topnode->Top, topnode->Right, topnode->Bottom, topnode->Height);
        

            if(direction&PMSHADOW_HORIZ) dx=topnode->Height*2;
            if(direction&PMSHADOW_VERT) dy=topnode->Height*2;

            rect.Left=MAX(shrect.Left, topnode->Left);
            rect.Right=MIN(shrect.Right-dx, topnode->Right);
            rect.Top=MAX(shrect.Top, topnode->Top);
            rect.Bottom=MIN(shrect.Bottom-dy, topnode->Bottom);
            rect.n.Length=sizeof(PMSR);
        
            if(direction&PMSHADOW_LEFT) {
                if(rect.Left<l+ss-(topnode->Height*2))
                    rect.Left=l+ss-(topnode->Height*2);
            }

            if(direction&PMSHADOW_TOP) {
                if(rect.Top<t+ss-(topnode->Height*2))
                    rect.Top=t+ss-(topnode->Height*2);
            }
        
            if(rect.Right>rect.Left && rect.Bottom>rect.Top) {
                newnode=PM_CopyShadowNode(&rect);
                if(newnode) {
                    AddHead((struct List *)delta, (struct Node *)newnode);
                }
            }           

        topnode=nexttopnode;
    }

    return (!(BOOL)fail);
}
