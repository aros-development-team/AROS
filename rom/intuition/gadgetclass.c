/*
    Copyright  1995-2026, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
*/

#include <exec/types.h>

#include <dos/dos.h>
#include <dos/dosextens.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/icclass.h>

#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <clib/macros.h>

#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#ifndef __MORPHOS__

struct ICData;
#include "intuition_intern.h"
#include <aros/asmcall.h>
#include <proto/alib.h>
#include "gadgets.h"

#endif /* !__MORPHOS__ */

#define DEBUG_GADGET(x) ;

/****************************************************************************/

/*
 * Many gadget attributes do no more than store one coordinate and/or change
 * one bit in Flags, Activation or GadgetType. They are described by the table
 * below, which is indexed by (tag - GA_Dummy) and used by both OM_SET and
 * OM_GET:
 *
 *  - OM_SET stores the value in the coordinate, if the row names one, and
 *    sets or clears the bit, if the row names one.
 *  - OM_GET returns the coordinate if the row names one, otherwise TRUE or
 *    FALSE depending on the bit.
 *
 * A row with gad_How == 0 is an attribute that is not handled this way.
 */
struct GadgetAttrDesc
{
    UBYTE gad_Coord;    /* offset of the WORD the value is stored in, or 0 */
    UBYTE gad_Word;     /* offset of the UWORD that holds gad_Bit, or 0 */
    UWORD gad_Bit;      /* GFLG_, GACT_ or GTYP_ bit */
    UBYTE gad_How;      /* GAD_ flags */
};

/* What OM_SET does with gad_Bit */
#define GAD_SET     0x01    /* always sets it */
#define GAD_CLEAR   0x02    /* always clears it */
#define GAD_BOOL    0x03    /* sets it if ti_Data is non-zero, clears it otherwise */
#define GAD_OPMASK  0x03
/* OM_SET reports a visual change */
#define GAD_VISUAL  0x04

#define GAD_ROW(tag)    [(tag) - GA_Dummy]
#define GAD_OFFSET(m)   offsetof(struct ExtGadget, m)
#define GAD_NOWORD      0, 0

static const struct GadgetAttrDesc gadgetattrs[] =
{
    /*      attribute           coordinate              flag word                   bit                 OM_SET */
    GAD_ROW(GA_Left)        = { GAD_OFFSET(LeftEdge),   GAD_NOWORD,                                     GAD_VISUAL              },
    GAD_ROW(GA_Top)         = { GAD_OFFSET(TopEdge),    GAD_NOWORD,                                     GAD_VISUAL              },
    GAD_ROW(GA_Width)       = { GAD_OFFSET(Width),      GAD_OFFSET(Flags),          GFLG_RELWIDTH,      GAD_CLEAR | GAD_VISUAL  },
    GAD_ROW(GA_Height)      = { GAD_OFFSET(Height),     GAD_OFFSET(Flags),          GFLG_RELHEIGHT,     GAD_CLEAR | GAD_VISUAL  },
    GAD_ROW(GA_RelRight)    = { GAD_OFFSET(LeftEdge),   GAD_OFFSET(Flags),          GFLG_RELRIGHT,      GAD_SET | GAD_VISUAL    },
    GAD_ROW(GA_RelBottom)   = { GAD_OFFSET(TopEdge),    GAD_OFFSET(Flags),          GFLG_RELBOTTOM,     GAD_SET | GAD_VISUAL    },
    GAD_ROW(GA_RelWidth)    = { GAD_OFFSET(Width),      GAD_OFFSET(Flags),          GFLG_RELWIDTH,      GAD_SET | GAD_VISUAL    },
    GAD_ROW(GA_RelHeight)   = { GAD_OFFSET(Height),     GAD_OFFSET(Flags),          GFLG_RELHEIGHT,     GAD_SET | GAD_VISUAL    },
    GAD_ROW(GA_RelSpecial)  = { 0,                      GAD_OFFSET(Flags),          GFLG_RELSPECIAL,    GAD_BOOL | GAD_VISUAL   },
    GAD_ROW(GA_Selected)    = { 0,                      GAD_OFFSET(Flags),          GFLG_SELECTED,      GAD_BOOL | GAD_VISUAL   },
    GAD_ROW(GA_Disabled)    = { 0,                      GAD_OFFSET(Flags),          GFLG_DISABLED,      GAD_BOOL | GAD_VISUAL   },
    GAD_ROW(GA_TabCycle)    = { 0,                      GAD_OFFSET(Flags),          GFLG_TABCYCLE,      GAD_BOOL                },
    GAD_ROW(GA_GZZGadget)   = { 0,                      GAD_OFFSET(GadgetType),     GTYP_GZZGADGET,     GAD_BOOL                },
    GAD_ROW(GA_SysGadget)   = { 0,                      GAD_OFFSET(GadgetType),     GTYP_SYSGADGET,     GAD_BOOL                },
    GAD_ROW(GA_EndGadget)   = { 0,                      GAD_OFFSET(Activation),     GACT_ENDGADGET,     GAD_BOOL                },
    GAD_ROW(GA_Immediate)   = { 0,                      GAD_OFFSET(Activation),     GACT_IMMEDIATE,     GAD_BOOL                },
    GAD_ROW(GA_RelVerify)   = { 0,                      GAD_OFFSET(Activation),     GACT_RELVERIFY,     GAD_BOOL                },
    GAD_ROW(GA_FollowMouse) = { 0,                      GAD_OFFSET(Activation),     GACT_FOLLOWMOUSE,   GAD_BOOL                },
    GAD_ROW(GA_RightBorder) = { 0,                      GAD_OFFSET(Activation),     GACT_RIGHTBORDER,   GAD_BOOL                },
    GAD_ROW(GA_LeftBorder)  = { 0,                      GAD_OFFSET(Activation),     GACT_LEFTBORDER,    GAD_BOOL                },
    GAD_ROW(GA_TopBorder)   = { 0,                      GAD_OFFSET(Activation),     GACT_TOPBORDER,     GAD_BOOL                },
    GAD_ROW(GA_BottomBorder)= { 0,                      GAD_OFFSET(Activation),     GACT_BOTTOMBORDER,  GAD_BOOL                },
    GAD_ROW(GA_ToggleSelect)= { 0,                      GAD_OFFSET(Activation),     GACT_TOGGLESELECT,  GAD_BOOL                },
};

#define NUM_GADGETATTRS (sizeof(gadgetattrs) / sizeof(gadgetattrs[0]))

/* The offsets are kept in bytes, and 0 stands for "none" */
_Static_assert(sizeof(struct ExtGadget) <= 256, "gadget member offsets must fit a byte");
_Static_assert(GAD_OFFSET(LeftEdge) != 0 && GAD_OFFSET(Flags) != 0, "offset 0 means no member");

/* Returns the table row of an attribute, or NULL if it has none */
static const struct GadgetAttrDesc *gadgetattr(ULONG attrid)
{
    ULONG idx = attrid - GA_Dummy;

    if ((idx < NUM_GADGETATTRS) && gadgetattrs[idx].gad_How)
        return &gadgetattrs[idx];

    return NULL;
}

/****************************************************************************/

/* set gadget attributes
*/
static ULONG set_gadgetclass(Class *cl, struct ExtGadget *eg, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct TagItem *tag, *tstate = msg->ops_AttrList;
    const struct GadgetAttrDesc *desc;
    IPTR                  tidata;
    ULONG                 retval = 0UL; /* set to non-zero to signal visual changes */

    while ( (tag = NextTagItem(&tstate)) )
    {
        tidata = tag->ti_Data;

        desc = gadgetattr(tag->ti_Tag);
        if (desc)
        {
            if (desc->gad_Coord)
            {
                *(WORD *)((UBYTE *)eg + desc->gad_Coord) = (WORD)tidata;
            }

            if (desc->gad_Word)
            {
                UWORD *word = (UWORD *)((UBYTE *)eg + desc->gad_Word);
                BOOL   set;

                switch (desc->gad_How & GAD_OPMASK)
                {
                    case GAD_SET:
                        set = TRUE;
                        break;

                    case GAD_CLEAR:
                        set = FALSE;
                        break;

                    default: /* GAD_BOOL */
                        set = (tidata != 0);
                        break;
                }

                if (set)
                {
                    *word |= desc->gad_Bit;
                }
                else
                {
                    *word &= ~desc->gad_Bit;
                }
            }

            if (desc->gad_How & GAD_VISUAL)
            {
                retval = 1UL;
            }

            continue;
        }

        switch(tag->ti_Tag)
        {
            case GA_Bounds:
                if (tidata)
                {
                    eg->BoundsLeftEdge = ((struct IBox *)tidata)->Left;
                    eg->BoundsTopEdge  = ((struct IBox *)tidata)->Top;
                    eg->BoundsWidth    = ((struct IBox *)tidata)->Width;
                    eg->BoundsHeight   = ((struct IBox *)tidata)->Height;
                    eg->MoreFlags |= GMORE_BOUNDS;
                }
                retval = 1UL;
                break;

            case GA_GadgetHelp:
                if (tidata)
                {
                    eg->MoreFlags |= GMORE_GADGETHELP;
                }
                else
                {
                    eg->MoreFlags &= ~GMORE_GADGETHELP;
                }
                retval = 1UL;
                break;

            case GA_Next:
                eg->NextGadget = (struct ExtGadget *)tidata;
                break;

            case GA_Previous:
                if( (tidata != 0L) && (msg->MethodID == OM_NEW) )
                {
                    eg->NextGadget = ((struct ExtGadget *)tidata)->NextGadget;
                    ((struct ExtGadget *)tidata)->NextGadget = eg;
                }
                break;

            case GA_IntuiText:
                eg->GadgetText = (struct IntuiText *)tidata;
                if (tidata)
                {
                    eg->Flags &= ~GFLG_LABELMASK;
                    eg->Flags |= GFLG_LABELITEXT;
                }
                retval = 1UL;
                break;

            case GA_Text:
                eg->GadgetText = (struct IntuiText *)tidata;
                if (tidata)
                {
                    eg->Flags &= ~GFLG_LABELMASK;
                    eg->Flags |= GFLG_LABELSTRING;
                }
                retval = 1UL;
                break;

            case GA_LabelImage:
                eg->GadgetText = (struct IntuiText *)tidata;
                if (tidata)
                {
                    eg->Flags &= ~GFLG_LABELMASK;
                    eg->Flags |= GFLG_LABELIMAGE;
                }
                retval = 1UL;
                break;

            case GA_Image:
                eg->GadgetRender = (APTR)tidata;
                if (tidata)
                {
                    eg->Flags |= GFLG_GADGIMAGE;
                }
                retval = 1UL;
                break;

            case GA_Border:
                eg->GadgetRender = (APTR)tidata;
                if (tidata)
                {
                    eg->Flags &= ~GFLG_GADGIMAGE;
                }
                retval = 1UL;
                break;

            case GA_SelectRender:
                eg->SelectRender = (APTR)tidata;
                {
                    eg->Flags &= ~GFLG_GADGHIGHBITS;
                    eg->Flags |= (GFLG_GADGIMAGE | GFLG_GADGHIMAGE);
                }
                retval = 1UL;
                break;

            case GA_SpecialInfo:
                eg->SpecialInfo = (APTR)tidata;
                break;

            case GA_Highlight:
                eg->Flags &= ~GFLG_GADGHIGHBITS;
                eg->Flags |= tidata & GFLG_GADGHIGHBITS;
                break;

            case GA_SysGType:
                eg->GadgetType &= ~GTYP_SYSTYPEMASK;
                eg->GadgetType |= tidata & GTYP_SYSTYPEMASK;
                break;

            case GA_ID:
                /* GA_ID should NOT be set if this is a OM_UPDATE.
                ** This is because gadgets should send their GA_ID
                ** when doing a OM_NOTIFY, so that the receiver
                ** might see who sent the message.
                ** But we surely don't want to change the GA_ID
                ** of the reciver to that of the sender.
                */
                if (msg->MethodID != OM_UPDATE)
                {
                    eg->GadgetID = tidata;
                }
                break;

            case GA_UserData:
                eg->UserData = (APTR)tidata;
                DEBUG_GADGET(dprintf("set_gadgetclass: UserData 0x%lx\n",tidata));
                break;

            case ICA_TARGET:
                ((struct GadgetData *)eg)->IC.ic_Target = (Object *)tidata;
                break;

            case ICA_MAP:
                ((struct GadgetData *)eg)->IC.ic_Mapping = (struct TagItem *)tidata;
                break;

        } /* switch tag */

    } /* while NextTagItem */

#if 0
    /* This seems to be wrong here. Instead buttongclass is where
       something like this happens, so look there (stegerg) */

    if ((msg->MethodID == OM_NEW) &&
            (eg->Flags & GFLG_GADGIMAGE) &&
            (eg->GadgetRender != NULL))
    {
        if (eg->Width  == 0) eg->Width  = ((struct Image *)eg->GadgetRender)->Width;
        if (eg->Height == 0) eg->Height = ((struct Image *)eg->GadgetRender)->Height;
    }
#endif

    return retval;
}


IPTR GadgetClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct ExtGadget *eg = (struct ExtGadget *)DoSuperMethodA(cl, o, (Msg)msg);

    if (eg)
    {
        /* set some defaults */
        /*
         * The instance object is cleared memory!
         * SetMem (eg, 0, sizeof(struct GadgetData));
         */
        eg->Flags         = GFLG_EXTENDED;
        eg->GadgetType    = GTYP_CUSTOMGADGET;
    eg->MoreFlags     = GMORE_BOOPSIGADGET;
        eg->MutualExclude = (IPTR)&((Class *)o)->cl_Dispatcher;

        /* Handle our special tags - overrides defaults */
        set_gadgetclass(cl, eg, msg);
    }
    
    return (IPTR)eg;
}

IPTR GadgetClass__OM_SET(Class *cl, struct ExtGadget *eg, struct opSet *msg)
{
    return DoSuperMethodA(cl, (Object *)eg, (Msg)msg) + set_gadgetclass(cl, eg, (struct opSet *)msg);
}

IPTR GadgetClass__OM_NOTIFY(Class *cl, struct GadgetData *gd, struct opUpdate *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: OM_NOTIFY\n"));
    DoNotify(cl, (Object *)gd, &(gd->IC), msg);
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: OM_NOTIFY done\n"));

    return (IPTR)0;
}

IPTR GadgetClass__OM_DISPOSE(Class *cl, struct GadgetData *gd, Msg msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    FreeICData((struct ICData *)&gd->IC);
    return DoSuperMethodA(cl, (Object *)gd, (Msg)msg);
}

/* get gadget attributes - gadgetclass really has no gettable
 * attributes, but we will implement some useful ones anyway. ;0
 */
IPTR GadgetClass__OM_GET(Class *cl, struct ExtGadget *eg, struct opGet *msg)
{
    const struct GadgetAttrDesc *desc = gadgetattr(msg->opg_AttrID);
    IPTR val;

    if (desc)
    {
        if (desc->gad_Coord)
        {
            val = (IPTR) *(WORD *)((UBYTE *)eg + desc->gad_Coord);
        }
        else
        {
            val = (IPTR)((*(UWORD *)((UBYTE *)eg + desc->gad_Word) & desc->gad_Bit) ? TRUE : FALSE);
        }

        *msg->opg_Storage = val;
        return 1UL;
    }

    switch (msg->opg_AttrID)
    {
        case GA_ID:
            val = (IPTR)eg->GadgetID;
            break;

        case GA_UserData:
            val = (IPTR)eg->UserData;
            break;

        case GA_GadgetHelp:
            val = (IPTR)(eg->MoreFlags & GMORE_GADGETHELP) ? TRUE : FALSE;
            break;

        case GA_Next:
            val = (IPTR)eg->NextGadget;
            break;

        case GA_IntuiText:
            val = (IPTR)(((eg->Flags & GFLG_LABELMASK) == GFLG_LABELITEXT) ? eg->GadgetText : 0);
            break;

        case GA_Text:
            val = (IPTR)((eg->Flags & GFLG_LABELSTRING) ? eg->GadgetText : 0);
            break;

        case GA_LabelImage:
            val = (IPTR)((eg->Flags & GFLG_LABELIMAGE) ? eg->GadgetText : 0);
            break;

        case GA_Image:
            val = (IPTR)((eg->Flags & GFLG_GADGIMAGE) ? eg->GadgetRender : 0);
            break;

        case GA_Border:
            val = (IPTR)((eg->Flags & GFLG_GADGIMAGE) ? 0 : eg->GadgetRender);
            break;

        case GA_SelectRender:
            val = (IPTR)(((eg->Flags & GFLG_GADGHIGHBITS) == GFLG_GADGHIMAGE) ? eg->SelectRender : 0);
            break;

        case GA_SpecialInfo:
            val = (IPTR)eg->SpecialInfo;
            break;

        case GA_Highlight:
            val = (IPTR)((eg->Flags & GFLG_GADGHIGHBITS) ? TRUE : FALSE);
            break;

        case GA_SysGType:
            val = (IPTR)(eg->GadgetType & GTYP_SYSTYPEMASK);
            break;

        case ICA_TARGET:
            val = (IPTR)((struct GadgetData *)eg)->IC.ic_Target;
            break;

        case ICA_MAP:
            val = (IPTR)((struct GadgetData *)eg)->IC.ic_Mapping;
            break;

        case GA_Bounds:
            if (msg->opg_Storage)
            {
                struct IBox *ibox = (struct IBox *)msg->opg_Storage;

                ibox->Left = eg->BoundsLeftEdge;
                ibox->Top = eg->BoundsTopEdge;
                ibox->Width = eg->BoundsWidth;
                ibox->Height = eg->BoundsHeight;
            }
            return 1UL;


        default:
        #if 0
            /* DONT DO THIS!! For example BGUI propclass relies on this not happening!! */

            val = (IPTR)NULL;
            */
        #endif
            return 0UL;

    } /* switch attrid) */

    *msg->opg_Storage = val;

    return 1UL;
}

/* test if we should try to activate this gadget...
 */
IPTR GadgetClass__GM_HITTEST(Class *cl, Object *o, struct gpHitTest *gpht)
{
    return (IPTR)GMR_GADGETHIT;
}

/* Methods follows that only need to return a value because they should be handled by
 * the subclasses
 */
IPTR GadgetClass__One(Class *cl, Object *o, Msg msg)
{
     return (IPTR)1;
}

IPTR GadgetClass__Zero(Class *cl, Object *o, Msg msg)
{
    return (IPTR)0;
}

IPTR GadgetClass_NoReuse(Class *cl, Object *o, Msg msg)
{
    return (IPTR)GMR_NOREUSE;
}

IPTR GadgetClass__GM_HELPTEST(Class *cl, Object *o, Msg msg)
{
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: GM_HELPTEST\n"));
    return (IPTR)GMR_HELPHIT;
}

IPTR GadgetClass__ICM_SETLOOP(Class *cl, struct GadgetData *gd, Msg msg)
{
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: ICM_SETLOOP\n"));
    gd->IC.ic_LoopCounter += 1UL;
    
    return (IPTR)0;
}

IPTR GadgetClass__ICM_CLEARLOOP(Class *cl, struct GadgetData *gd, Msg msg)
{
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: ICM_CLEARLOOP\n"));
    gd->IC.ic_LoopCounter -= 1UL;

    return (IPTR)0;
}

IPTR GadgetClass__ICM_CHECKLOOP(Class *cl, struct GadgetData *gd, Msg msg)
{
    DEBUG_GADGET(dprintf("dispatch_gadgetclass: ICM_CHECKLOOP\n"));
    return (IPTR)gd->IC.ic_LoopCounter;
}
