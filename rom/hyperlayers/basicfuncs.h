/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Basic support functions for layers.library.
    Lang: English.
*/

#include "layers_intern.h"
#include <exec/types.h>
#include <graphics/gfx.h>
#include <graphics/clip.h>


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
               ULONG                Mode,
               struct LayersBase  * LayersBase);

void BltCRtoRP(struct RastPort *    rp,
               struct ClipRect *    cr,
               ULONG                Mode,
               struct LayersBase  * LayersBase);

/***************************************************************************/
/*                                  HOOK                                   */
/***************************************************************************/

void _CallLayerHook(struct Hook * h,
                    struct RastPort * rp,
                    struct Layer * L,
                    struct Rectangle * R,
                    WORD offsetx,
                    WORD offsety,
                    struct LayersBase * LayersBase);


/***************************************************************************/
/*                                 LAYER                                   */
/***************************************************************************/

void SetLayerPriorities(struct Layer_Info * li);
struct Layer * internal_WhichLayer(struct Layer * l, WORD x, WORD y);
struct Layer *CreateLayerTagList(struct Layer_Info *li, struct BitMap *bm, LONG x0, LONG y0, 
				 LONG x1, LONG y1, LONG flags, int priority, struct TagItem *tagList,
				 struct LayersBase *LayersBase);
void _FreeLayer(struct Layer * l, struct LayersBase *LayersBase);
struct Layer *GetFirstFamilyMember(struct Layer *l);


/***************************************************************************/
/*                               LAYERINFO                                 */
/***************************************************************************/

BOOL _AllocExtLayerInfo
    (struct Layer_Info * li, struct LayersBase *LayersBase);

void _FreeExtLayerInfo
    (struct Layer_Info * li, struct LayersBase *LayersBase);

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

//void _TranslateRect(struct Rectangle *rect, WORD dx, WORD dy);
struct ClipRect * internal_WhichClipRect(struct Layer * L, WORD x, WORD y);


/***************************************************************************/
/*                            RESOURCE HANDLING                            */
/***************************************************************************/

BOOL AddLayersResource(struct Layer_Info * li,
                       void *              ptr,
                       ULONG               Size);

struct ResourceNode * AddLayersResourceNode(struct Layer_Info * li);

struct ClipRect * _AllocClipRect(struct Layer * L, struct LayersBase *LayersBase);
void _FreeClipRect(struct ClipRect * CR, struct Layer * L, struct LayersBase *LayersBase);
void _FreeClipRectListBM(struct Layer * L, struct ClipRect * CR, struct LayersBase *LayersBase);

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
void UnsplitLayers(struct Layer_Info * LI, struct Rectangle * rect );
void CopyAndFreeClipRectsClipRects(struct Layer * L,
                                   struct ClipRect * srcCR,
                                   struct ClipRect * destCR);
int _CopyClipRectsToClipRects(struct Layer * l,
                              struct ClipRect * oldcr,
                              struct ClipRect * newcr,
                              int srcdx,
			      int destdx,
                              int backupmode,
                              int freelist,
                              int addtodamagelist,
			      struct LayersBase *LayersBase);
void UninstallClipRegionClipRects(struct Layer_Info * LI);
void InstallClipRegionClipRects(struct Layer_Info * LI);

/***************************************************************************/
/*                            TRANSPARENCY HOOK                            */
/***************************************************************************/

struct Region *doCreateLayer(struct Hook *shapeHook, struct Region *layershape,
			     LONG x0, LONG y0, LONG x1, LONG y1,
			     APTR data, struct LayersBase *LayersBase);
void doChangeLayer(struct Layer *l, struct LayersBase *LayersBase);

/*-----------------------------------END-----------------------------------*/

#define RECTAREA(r) (((r)->MaxX - (r)->MinX + 1) * ((r)->MaxY - (r)->MinY + 1))

#define DO_OVERLAP(r1,r2) (!( ((r1)->MinX > (r2)->MaxX) ||	\
                              ((r1)->MinY > (r2)->MaxY) ||	\
                              ((r1)->MaxX < (r2)->MinX) ||	\
                              ((r1)->MaxY < (r2)->MinY) ))

#define IS_EMPTYREGION(r) (NULL == (r)->RegionRectangle)

#define IS_ROOTLAYER(l) ((l) == (l)->LayerInfo->check_lp)

int _BackupPartsOfLayer(struct Layer * l, 
                        struct Region * hide_region,
                        int dx,
                        int backupsimplerefresh,
                        struct LayersBase *);

int _ShowPartsOfLayer(struct Layer * l, 
                      struct Region * show_region,
                      struct LayersBase *);

int _ShowLayer(struct Layer * l, struct LayersBase *LayersBase);

struct Layer * _FindFirstFamilyMember(struct Layer * l);

void _BackFillRegion(struct Layer * l, 
                     struct Region * r,
                     int addtodamagelist,
                     struct LayersBase * LayersBase);

struct ClipRect * _CreateClipRectsFromRegion(struct Region *r,
                                             struct Layer * l,
                                             int invisible,
                                             struct Region *inverter,
					     struct LayersBase *LayersBase);

struct Region *_InternalInstallClipRegion(struct Layer *l, struct Region *region,
    	    	    	    	    	  WORD srcdx, WORD destdx,
    	    	    	    	    	  struct LayersBase *LayersBase);

