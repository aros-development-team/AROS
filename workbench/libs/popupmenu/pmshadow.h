//
// pmshadow.h
//
// PopupMenu Library - Shadows
//
// Copyright (C)2000 Henrik Isaksson <henrik@boing.nu>
// All Rights Reserved.
//

#ifndef PM_SHADOW_H
#define PM_SHADOW_H

#ifndef PM_DLIST_H
#include "pmdlist.h"
#endif

#define MAX(a,b)	(a>b?a:b)
#define MIN(a,b)	(a<b?a:b)

struct PMShadowRect {
	PMGLN		n;
	WORD		Left;
	WORD		Top;
	WORD		Right;
	WORD		Bottom;
};

typedef struct PMShadowRect PMSR;
typedef struct MinList PMSRList;

#define PM_InitShadowList		(PMSRList *)PM_InitList
#define PM_FreeShadowList(l)		PM_FreeList((PMDList *)l)
#define PM_CopyShadowList(l)		(PMSRList *)PM_CopyList((PMDList *)l)	
#define PM_AddShadowToList(l, A)	PM_AddToList((PMDList *)l, (PMNode *)A)
#define PM_UnlinkShadow(l, A)		PM_Unlink((PMDList *)l, (PMNode *)A)
#define PM_FreeShadowNode(A)		PM_FreeNode((PMNode *)A)
//#define PM_CopyShadowNode(A)		(PMSR *)PM_CopyNode((PMNode *)A)
/*
void PM_AddShadowToList(PMSRList *l, PMSR *A);	// Add A to l. *
void PM_UnlinkShadow(PMSRList *l, PMSR *A);	// Remove A from l. *
void PM_FreeShadowNode(PMSR *A);		// Free a shadow node. *
*/
PMSR *PM_CopyShadowNode(PMSR *A);		// Copy a shadow node. *
	

//PMSRList *PM_InitShadowList(void);		// Create a new list header *
//void PM_FreeShadowList(PMSRList *list);		// Free a list of shadow rects *
//PMSRList *PM_CopyShadowList(PMSRList *list);	// Copy a list of shadow rects *

BOOL PM_AddRect(PMSRList *list,			// Add a rectangle to a
	WORD l, WORD t, WORD r, WORD b);	// list of shadows. *

BOOL PM_AddShadow(PMSRList *sigma,		// Add shadows in delta to
		PMSRList *delta);		// sigma, and modify shadows
						// in delta so they will not
						// overlap shadows in sigma. *

BOOL PM_SubMenuRect(PMSRList *sigma,		// Subract a rectangle from
	WORD l, WORD t, WORD w, WORD h);	// list. *


//
// Support functions.
//

BOOL PM_ShadowOverlap(PMSR *A, PMSR *B);	// Does A touch B anywhere? *
BOOL PM_SubRects(PMSRList *dest,		// Remove parts of worknode
	PMSR *worknode, PMSR *rect);		// that are overlapped by rect. *

#endif
