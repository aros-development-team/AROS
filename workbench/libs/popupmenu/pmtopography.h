//
// pmtopography.h
//
// PopupMenu Library - Topographical Menu Map
//
// Copyright ©2000 Henrik Isaksson <henrik@boing.nu>
// All Rights Reserved.
//

#ifndef PM_TOPOGRAPHY_H
#define PM_TOPOGRAPHY_H

#ifndef PM_SHADOW_H
#include "pmshadow.h"
#endif

#define PMSHADOW_HORIZ		0x01	// Adjust shadow width
#define PMSHADOW_VERT		0x02	// Adjust shadow height
#define PMSHADOW_LEFT		0x04	// Adjust shadow left pos
#define PMSHADOW_TOP		0x08	// Adjust shadow top pos

struct PMTopographicRect {
	PMGLN		n;
	WORD		Left;
	WORD		Top;
	WORD		Right;
	WORD		Bottom;
	WORD		Height;		// Height over 'ground' measured in menu levels
};

typedef struct PMTopographicRect PMTR;
typedef struct MinList PMTRList;

#define PM_InitTopographicList()	((PMTRList *)PM_InitList())
#define PM_FreeTopographicList(l)	PM_FreeList((PMDList *)l)
#define PM_CopyTopographicList(l)	((PMTRList *)PM_CopyList((PMDList *)l))
#define PM_RegionOverlap(A, B)		PM_ShadowOverlap((PMSR *)A, (PMSR *)B)
#define PM_AddRegionToList(l, A)	PM_AddToList((PMDList *)l, (PMNode *)A)
#define PM_UnlinkRegion(l, A)		PM_Unlink((PMDList *)l, (PMNode *)A)
#define PM_FreeTopographicNode(A)	PM_FreeNode((PMNode *)A)

BOOL PM_AddTopographicRegion(PMTRList *lst,		// Add a rectangular region of height
	WORD l, WORD t, WORD r, WORD b, WORD h);	// 'h' to the topographic map.

BOOL PM_MapShadow(PMTRList *top, PMSRList *delta,
	WORD x, WORD y, WORD w, WORD h,
	UWORD menulevel, UWORD direction);

PMTR *PM_CopyTopographicNode(PMTR *A);

BOOL PM_SubTopographicRects(PMTRList *dest, PMTR *worknode, PMTR *rect);

#endif
