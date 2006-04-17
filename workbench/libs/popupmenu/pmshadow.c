//
// pmshadow.c
//
// PopupMenu Library - Shadows
//
// Copyright (C)2000 Henrik Isaksson <henrik@boing.nu>
// All Rights Reserved.
//

#include "pmshadow.h"

#include <exec/memory.h>

#include <proto/exec.h>
#include <clib/alib_protos.h>

#include <stdio.h>

#define ADDSHADOW(l,s)			{ rectptr=PM_CopyShadowNode(s); if(rectptr) PM_AddShadowToList(l, rectptr); else fail=TRUE; }
#define INSIDEX(a, b1, b2)		(a>b1 && a<b2)
#define INSIDE(a, b1, b2)		(a>=b1 && a<=b2)
#define INTERVALOVERLAP(a1, a2, b1, b2)	(INSIDE(a1, b1, b2) || INSIDE(a2, b1, b2) || INSIDE(b1, a1, a2) || INSIDE(b2, a1, a2))

//
// PM_ShadowOverlap - return TRUE if the two rects touch each other anywhere
//
BOOL PM_ShadowOverlap(PMSR *A, PMSR *B)
{
	if(INTERVALOVERLAP(A->Top+1, A->Bottom, B->Top+1, B->Bottom)) {
		if(INTERVALOVERLAP(A->Left+1, A->Right, B->Left+1, B->Right)) {
			return TRUE;
		}
	}
	
	if((A->Top==B->Top && (A->Left==B->Left || A->Right==B->Right)) ||
	   (A->Bottom==B->Bottom && (A->Left==B->Left || A->Right==B->Right)))
	   	return TRUE; 
	
	return FALSE;
}

//
// PM_CopyShadowNode - copy a PMShadowRect.
//
PMSR *PM_CopyShadowNode(PMSR *A)
{
	PMSR *newnode=NULL;

	if(A) {
		((PMGLN *)A)->Length=sizeof(PMSR);

		newnode=(PMSR *)PM_CopyNode((PMNode *)A);
	}

	return newnode;
}

//
// PM_SubRects - subtract one rect from another to a list of new rects
//
BOOL PM_SubRects(PMSRList *dest, PMSR *worknode, PMSR *rect)
{
	PMSR tmprect, *rectptr;
	BOOL fail=FALSE;

	// Definition: "old shadow" = worknode
	//             "new shadow" = rect

	// Now, check existance and size of the following four regions:
	//
	// +-------------+
	// |      A      |
	// +---+-----+---+
	// | C | new | D |
	// +---+-----+---+
	// |      B      |
	// +-------------+

	/*printf("  (%ld %ld %ld %ld) <> (%ld %ld %ld %ld) transformed to:\n",
		rect->Left, rect->Top, rect->Right, rect->Bottom,
		worknode->Left, worknode->Top, worknode->Right, worknode->Bottom);*/

	/*if(rect->Left==worknode->Left && rect->Right==worknode->Right &&
	   rect->Top==worknode->Top && rect->Bottom==worknode->Bottom)
	   	return TRUE;*/

	tmprect.Left=worknode->Left;
	tmprect.Right=worknode->Right;

	// For A to exist, the rect's upper edge must be inside the
	// old shadow.

	if(INSIDEX(rect->Top, worknode->Top, worknode->Bottom)) {
		tmprect.Top=worknode->Top;
		tmprect.Bottom=rect->Top;

		// Add the new rect to the list

		//printf("A: %ld %ld %ld %ld\n", tmprect.Left, tmprect.Top, tmprect.Right, tmprect.Bottom);

		ADDSHADOW(dest, &tmprect);
	}

	// For B to exist, the rect's lower edge must be inside the
	// old shadow.

	if(INSIDEX(rect->Bottom, worknode->Top, worknode->Bottom)) {
		tmprect.Top=rect->Bottom;
		tmprect.Bottom=worknode->Bottom;

		// Add the new rect to the list

		//printf("B: %ld %ld %ld %ld\n", tmprect.Left, tmprect.Top, tmprect.Right, tmprect.Bottom);

		ADDSHADOW(dest, &tmprect);
	}

	tmprect.Top=MAX(rect->Top, worknode->Top);
	tmprect.Bottom=MIN(rect->Bottom, worknode->Bottom);

	// For C to exist, the left edge of the new shadow must be
	// inside the old one.

	if(INSIDEX(rect->Left, worknode->Left, worknode->Right)) {
		tmprect.Left=worknode->Left;
		tmprect.Right=rect->Left;

		// Add the new rect to the list

		//printf("C: %ld %ld %ld %ld\n", tmprect.Left, tmprect.Top, tmprect.Right, tmprect.Bottom);

		ADDSHADOW(dest, &tmprect);
	}

	// For D to exist, the right edge of the new shadow must be
	// inside the old one.

	if(INSIDEX(rect->Right, worknode->Left, worknode->Right)) {
		tmprect.Left=rect->Right;
		tmprect.Right=worknode->Right;

		// Add the new rect to the list

		//printf("D: %ld %ld %ld %ld\n", tmprect.Left, tmprect.Top, tmprect.Right, tmprect.Bottom);

		ADDSHADOW(dest, &tmprect);
	}

	//printf("--- End of transformation ---\n");

	return (!fail);
}

//
// PM_AddShadow - Add a new shadow to sigma and add the change to delta.
//
BOOL PM_AddShadow(PMSRList *sigma, PMSRList *delta)
{
	PMSR *sigmanode, *nextsigmanode;
	PMSR *deltanode, *nextdeltanode;
	BOOL fail=FALSE;

	//
	// Find old (sigma) shadows overlapped by the new one.
	//
	sigmanode = (PMSR *)(sigma->mlh_Head); /* First node */
	if(!IsListEmpty((struct List *)sigma))
	while((nextsigmanode = (PMSR *)PM_NextNode(sigmanode))) {

		deltanode = (PMSR *)(delta->mlh_Head); /* First node */
		while((nextdeltanode = (PMSR *)PM_NextNode(deltanode))) {

			if(PM_ShadowOverlap(deltanode, sigmanode)) {
				PM_UnlinkShadow(delta, deltanode);	// Remove from delta list
				PM_SubRects(delta, deltanode, sigmanode);
				PM_FreeShadowNode(deltanode);		// Free the old shadow
			}

			deltanode=nextdeltanode;
		}

		sigmanode = nextsigmanode;
	}

	//
	// Now we add whatever is left in delta to sigma.
	//
	deltanode = (PMSR *)(delta->mlh_Head); /* First node */
	while((nextdeltanode = (PMSR *)PM_NextNode(deltanode))) {

		PMSR *cpy=PM_CopyShadowNode(deltanode);
		if(cpy) {
			PM_AddShadowToList(sigma, cpy);
		}

		deltanode=nextdeltanode;
	}

	return (!fail);
}

//
// PM_SubMenuRect - subtract a menu rect from the shadow list.
//
BOOL PM_SubMenuRect(PMSRList *sigma, WORD l, WORD t, WORD w, WORD h)
{
	PMSR *sigmanode, *nextsigmanode;
	PMSR rect;
	BOOL fail=FALSE;

	rect.Left=l;
	rect.Top=t;
	rect.Right=l+w;
	rect.Bottom=t+h;

	//
	// Find old (sigma) shadows overlapped by the new one.
	//
	sigmanode = (PMSR *)(sigma->mlh_Head); /* First node */
	while((nextsigmanode = (PMSR *)PM_NextNode(sigmanode))) {

		if(PM_ShadowOverlap(&rect, sigmanode)) {
			PM_UnlinkShadow(sigma, sigmanode);	// Remove from delta list
			PM_SubRects(sigma, sigmanode, &rect);
			PM_FreeShadowNode(sigmanode);		// Free the old shadow
		}


		sigmanode = nextsigmanode;
	}

	return (!fail);
}

//
// PM_AddRect - add a rectangle to a list of shadows.
//
BOOL PM_AddRect(PMSRList *list, WORD l, WORD t, WORD r, WORD b)
{
	PMSR tmprect, *cpy;

	tmprect.Left=l;
	tmprect.Top=t;
	tmprect.Right=r;
	tmprect.Bottom=b;

	cpy=PM_CopyShadowNode(&tmprect);
	if(cpy) {
		PM_AddShadowToList(list, cpy);
		return TRUE;
	}
	return FALSE;
}
