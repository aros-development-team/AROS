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
    struct Gadget *obj;
    Class *cl;
    struct TagItem tags[] = {
	{GA_Disabled, FALSE},
        {GA_Immediate, FALSE},
        {GA_RelVerify, TRUE},
	{TAG_MORE, 0L}
    };

    cl = makebuttonclass(GadToolsBase);
    if (!cl)
	return NULL;

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GA_Immediate, FALSE, taglist);
    tags[3].ti_Data = (IPTR)stdgadtags;

    obj = (struct Gadget *)NewObjectA(cl, NULL, tags);

    return obj;
}


struct Gadget *makecheckbox(struct GadToolsBase_intern *GadToolsBase,
                            struct TagItem stdgadtags[],
			    struct VisualInfo *vi,
                            struct TagItem *taglist)
{
    struct Gadget *obj;
    Class *cl;
    struct TagItem tags[] = {
	{GA_RelVerify, TRUE},
	{GA_Disabled, FALSE},
	{GTCB_Checked, FALSE},
	{TAG_MORE, 0L}
    };

    cl = makecheckclass(GadToolsBase);
    if (!cl)
	return NULL;

    tags[1].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[2].ti_Data = GetTagData(GTCB_Checked, FALSE, taglist);
    tags[3].ti_Data = (IPTR)stdgadtags;

    if (!GetTagData(GTCB_Scaled, FALSE, taglist))
    {
	stdgadtags[TAG_Width].ti_Data = CHECKBOX_WIDTH;
	stdgadtags[TAG_Height].ti_Data = CHECKBOX_HEIGHT;
    }

    obj = (struct Gadget *)NewObjectA(cl, NULL, tags);

    return obj;
}
