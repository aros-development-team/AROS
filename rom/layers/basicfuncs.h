/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Basic support functions for layers.library.
    Lang: English.
*/

#include <aros/asmcall.h>

/*
 *  Sections:
 *
 *  + Blitter
 *  + Hook
 *  + Layer
 *  + LayerInfo
 *  + Rectangle
 *  + Resource Handling
 *  + Miscellaneous
 *
 */

/***************************************************************************/
/*                                 BLITTER                                 */
/***************************************************************************/

AROS_UFP4(void, BltRPtoCR,
    AROS_UFPA(struct RastPort *,   rp,         A0),
    AROS_UFPA(struct ClipRect *,   cr,         A1),
    AROS_UFPA(ULONG,               Mode,       D0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP4(void, BltCRtoRP,
    AROS_UFPA(struct RastPort *,   rp,         A0),
    AROS_UFPA(struct ClipRect *,   cr,         A1),
    AROS_UFPA(ULONG,               Mode,       D0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

/***************************************************************************/
/*                                  HOOK                                   */
/***************************************************************************/

AROS_UFP8(void, CallLayerHook,
    AROS_UFPA(struct Hook *,       h,          A2),
    AROS_UFPA(struct Layer *,      l,          D0),
    AROS_UFPA(struct RastPort *,   rp,         A0),
    AROS_UFPA(struct Rectangle *,  r1,         A1),
    AROS_UFPA(struct Rectangle *,  r2,         A3),
    AROS_UFPA(WORD,                BaseX,      D1),
    AROS_UFPA(WORD,                BaseY,      D2),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

/***************************************************************************/
/*                                 LAYER                                   */
/***************************************************************************/

void SetLayerPriorities(struct Layer_Info * li);

/***************************************************************************/
/*                               LAYERINFO                                 */
/***************************************************************************/

AROS_UFP2(BOOL, _AllocExtLayerInfo,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP2(void, _FreeExtLayerInfo,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP2(ULONG, _InitLIExtra,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP2(void, _ExitLIExtra,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP2(BOOL, SafeAllocExtLI,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP2(void, SafeFreeExtLI,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

/***************************************************************************/
/*                                RECTANGLE                                */
/***************************************************************************/

struct ClipRect * internal_WhichClipRect(struct Layer * L, WORD x, WORD y);

AROS_UFP3(void, ClearRect,
    AROS_UFPA(struct RastPort *,   rp,         A0),
    AROS_UFPA(struct Rectangle *,  r,          A1),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP3(void, IntersectRects,
    AROS_UFPA(struct Rectangle *, r1,          A0),
    AROS_UFPA(struct Rectangle *, r2,          A1),
    AROS_UFPA(struct Rectangle *, Result,      A2));

AROS_UFP2(BOOL, Overlap,
    AROS_UFPA(struct Rectangle *, r1, A0),
    AROS_UFPA(struct Rectangle *, r2, A1));

AROS_UFP2(BOOL, ContainsRect,
    AROS_UFPA(struct Rectangle *, Bound,     A0),
    AROS_UFPA(struct Rectangle *, InnerRect, A1));

AROS_UFP3(void, AddClipRect,
    AROS_UFPA(struct Layer *,      l,          A0),
    AROS_UFPA(struct ClipRect *,   cr,         A1),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP3(void, CopyCR,
    AROS_UFPA(struct ClipRect *,   source,     A0),
    AROS_UFPA(struct ClipRect *,   dest,       A1),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

/***************************************************************************/
/*                            RESOURCE HANDLING                            */
/***************************************************************************/

AROS_UFP4(BOOL, AddLayersResource,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(void *,              ptr,        A1),
    AROS_UFPA(ULONG,               Size,       D0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP2(struct ResourceNode *, AddLayersResourceNode,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP3(BOOL, AllocCRBitMap,
    AROS_UFPA(struct Layer *,      l,          A0),
    AROS_UFPA(struct ClipRect *,   cr,         A1),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP2(struct ClipRect *, AllocClipRect,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP2(void, DisposeClipRect,
    AROS_UFPA(struct ClipRect *,   cr,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP2(void, FreeCRBitMap,
    AROS_UFPA(struct ClipRect *,   cr,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP4(void *, AllocLayerStruct,
    AROS_UFPA(ULONG,               Size,       D0),
    AROS_UFPA(ULONG,               Flags,      D1),
    AROS_UFPA(struct Layer_Info *, li,         D2),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP3(void, FreeLayerResources,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(BOOL,                flag,       D0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

/***************************************************************************/
/*                              MISCELLANEOUS                              */
/***************************************************************************/


/*-----------------------------------END-----------------------------------*/
