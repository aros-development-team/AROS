/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: GadTools gadget creation functions
    Lang: English
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#include "gadtools_intern.h"
#include <stdio.h>

struct Gadget *makebutton(struct GadToolsBase_intern *GadToolsBase,
                          struct TagItem stdgadtags[],
			  struct VisualInfo *vi,
                          struct TagItem *taglist)
{
    struct Gadget *gad;
    struct Image *frame;
    struct TagItem tags[5];
    ULONG resolution;

    resolution = (vi->vi_dri->dri_Resolution.X<<16)+vi->vi_dri->dri_Resolution.Y;

    tags[0].ti_Tag = IA_Width;
    tags[0].ti_Data = stdgadtags[TAG_Width].ti_Data;
    tags[1].ti_Tag = IA_Height;
    tags[1].ti_Data = stdgadtags[TAG_Height].ti_Data;
    tags[2].ti_Tag = IA_Resolution;
    tags[2].ti_Data = (IPTR)resolution;
    tags[3].ti_Tag = IA_FrameType;
    tags[3].ti_Data = FRAME_BUTTON;
    tags[4].ti_Tag = TAG_DONE;
    frame = (struct Image *)NewObjectA(NULL, FRAMEICLASS, tags);
    if (!frame)
        return NULL;

    tags[0].ti_Tag = GA_Image;
    tags[0].ti_Data = (IPTR)frame;
    tags[1].ti_Tag = GA_Disabled;
    tags[1].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[2].ti_Tag = GA_Immediate;
    tags[2].ti_Data = GetTagData(GA_Immediate, FALSE, taglist);
    tags[3].ti_Tag = GA_RelVerify;
    tags[3].ti_Data = TRUE;
    tags[4].ti_Tag = TAG_MORE;
    tags[4].ti_Data = (IPTR)stdgadtags;
    gad = (struct Gadget *)NewObjectA(NULL, FRBUTTONCLASS, tags);
    if (!gad)
        DisposeObject(frame);
    return gad;
}


struct Gadget *makecheckbox(struct GadToolsBase_intern *GadToolsBase,
                            struct TagItem stdgadtags[],
			    struct VisualInfo *vi,
                            struct TagItem *taglist)
{
    struct Gadget *obj;
    Class *cl;
    struct TagItem tags[] = {
	{GA_RelVerify, 1L},
	{GA_Disabled, 0L},
	{GTCB_Checked, 0L},
	{TAG_DONE, 0L}
    };

    cl = makecheckclass(GadToolsBase);
    if (!cl)
	return NULL;

    tags[1].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[2].ti_Data = GetTagData(GTCB_Checked, FALSE, taglist);

    if (!GetTagData(GTCB_Scaled, FALSE, taglist))
    {
	stdgadtags[TAG_Width].ti_Data = CHECKBOX_WIDTH;
	stdgadtags[TAG_Height].ti_Data = CHECKBOX_HEIGHT;
    }

    obj = (struct Gadget *)NewObjectA(cl, NULL, tags);

    return obj;
}
