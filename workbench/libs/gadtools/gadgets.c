/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: GadTools gadget creation functions.
    Lang: English.
*/

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

    BOOL disabled;
    BOOL immediate;

    disabled = GetTagData(GA_Disabled, FALSE, taglist);
    immediate = GetTagData(GA_Immediate, FALSE, taglist);
    frame = (struct Image *)NewObject(NULL, FRAMEICLASS,
        IA_Width, stdgadtags[TAG_Width].ti_Data,
        IA_Height, stdgadtags[TAG_Height].ti_Data,
        IA_Resolution, vi->vi_dri->dri_Resolution,
	IA_FrameType, FRAME_BUTTON,
        TAG_DONE);
    if (!frame)
        return NULL;
    gad = (struct Gadget *)NewObject(NULL, BUTTONGCLASS,
        GA_Image, frame,
        GA_Disabled, disabled,
        GA_Immediate, immediate,
        GA_RelVerify, TRUE,
        TAG_MORE, stdgadtags);
    if (!gad)
        DisposeObject(frame);
    return gad;
}
