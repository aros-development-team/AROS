/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Basic support functions for layers.library.
    Lang: English.
*/

#include <aros/asmcall.h>

#define _abcdef abcdef

/*
 *  Sections:
 *
 *  + Blitter
 *  + Hook
 *  + Layer
 *  + LayerInfo
 *  + Rectangle
 *  + Miscellaneous
 *
 */

/***************************************************************************/
/*                                 BLITTER                                 */
/***************************************************************************/

AROS_UFP4(void, _BltRPtoCR,
    AROS_UFPA(struct RastPort *,   rp,         A0),
    AROS_UFPA(struct ClipRect *,   cr,         A1),
    AROS_UFPA(ULONG,               Mode,       D0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

AROS_UFP4(void, _BltCRtoRP,
    AROS_UFPA(struct RastPort *,   rp,         A0),
    AROS_UFPA(struct ClipRect *,   cr,         A1),
    AROS_UFPA(ULONG,               Mode,       D0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

/***************************************************************************/
/*                                  HOOK                                   */
/***************************************************************************/

/***************************************************************************/
/*                                 LAYER                                   */
/***************************************************************************/

/***************************************************************************/
/*                               LAYERINFO                                 */
/***************************************************************************/

AROS_UFP2(BOOL, _AllocExtLayerInfo,
    AROS_UFPA(struct Layer_Info *, li,         A0),
    AROS_UFPA(struct LayersBase *, LayersBase, A6));

void _FreeExtLayerInfo(struct Layer_Info *li, struct LayersBase *LayersBase);

/***************************************************************************/
/*                                RECTANGLE                                */
/***************************************************************************/

/***************************************************************************/
/*                              MISCELLANEOUS                              */
/***************************************************************************/


/*-----------------------------------END-----------------------------------*/
