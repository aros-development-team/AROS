/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Basic support functions for layers.library.
    Lang: English.
*/

#include "layers_intern.h"
#include <exec/types.h>
#include <graphics/gfx.h>

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

void BltRPtoCR(struct RastPort *    rp,
               struct ClipRect *    cr,
               ULONG                Mode);

void BltCRtoRP(struct RastPort *    rp,
               struct ClipRect *    cr,
               ULONG                Mode);

/***************************************************************************/
/*                                  HOOK                                   */
/***************************************************************************/

void _CallLayerHook(struct Hook * h,
                    struct RastPort * rp,
                    struct Layer * L,
                    struct Rectangle * R,
                    WORD offsetx,
                    WORD offsety);


/***************************************************************************/
/*                                 LAYER                                   */
/***************************************************************************/

void SetLayerPriorities(struct Layer_Info * li);
struct Layer * internal_WhichLayer(struct Layer * l, WORD x, WORD y);

/***************************************************************************/
/*                               LAYERINFO                                 */
/***************************************************************************/

BOOL _AllocExtLayerInfo
    (struct Layer_Info * li);

void _FreeExtLayerInfo
    (struct Layer_Info * li);

ULONG _InitLIExtra
    (struct Layer_Info * li, struct LayersBase * LayersBase);

void _ExitLIExtra
    (struct Layer_Info * li, struct LayersBase * LayersBase);

BOOL SafeAllocExtLI
    (struct Layer_Info * li, struct LayersBase * LayersBase);

void SafeFreeExtLI
    (struct Layer_Info * li, struct LayersBase * LayersBase);

/***************************************************************************/
/*                                RECTANGLE                                */
/***************************************************************************/

struct ClipRect * internal_WhichClipRect(struct Layer * L, WORD x, WORD y);


/***************************************************************************/
/*                            RESOURCE HANDLING                            */
/***************************************************************************/

BOOL AddLayersResource(struct Layer_Info * li,
                       void *              ptr,
                       ULONG               Size);

struct ResourceNode * AddLayersResourceNode(struct Layer_Info * li);

struct ClipRect * _AllocClipRect(struct Layer * L);
void _FreeClipRect(struct ClipRect * CR, struct Layer * L);


void FreeCRBitMap(struct ClipRect *   cr);

void * AllocLayerStruct(ULONG                Size,
                        ULONG                Flags,
                        struct Layer_Info *  li,
                        struct LayersBase *  LayersBase);

void FreeLayerResources(struct Layer_Info *  li,
                        BOOL                 flag);

/***************************************************************************/
/*                              MISCELLANEOUS                              */
/***************************************************************************/

void CleanTopLayer(struct Layer_Info * LI);
void CleanupLayers(struct Layer_Info * LI);

/*-----------------------------------END-----------------------------------*/
