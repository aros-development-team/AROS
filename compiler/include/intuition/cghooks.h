#ifndef INTUITION_CGHOOKS_H
#define INTUITION_CGHOOKS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Custom gadgets
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

struct GadgetInfo
{
    struct Screen    * gi_Screen;
    struct Window    * gi_Window;
    struct Requester * gi_Requester;
    struct RastPort  * gi_RastPort;
    struct Layer     * gi_Layer;
    struct IBox        gi_Domain;

    struct
    {
        UBYTE DetailPen;
        UBYTE BlockPen;
    } gi_Pens;

    struct DrawInfo * gi_DrInfo;

    ULONG gi_Reserved[6];
};

struct PGX
{
    struct IBox pgx_Container;
    struct IBox pgx_NewKnob;
};

#define CUSTOM_HOOK(gadget) ((struct Hook *) (gadget)->MutualExclude)

#endif /* INTUITION_CGHOOKS_H */
