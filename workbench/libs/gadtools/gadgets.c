/*
   (C) 1997 AROS - The Amiga Replacement OS
   $Id$

   Desc: GadTools gadget creation functions
   Lang: English
*/
#include <stdio.h>
#include <proto/exec.h>
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
#include <gadgets/aroscheckbox.h>
#include <gadgets/arosmx.h>
#include "gadtools_intern.h"

struct Gadget *makebutton(struct GadToolsBase_intern *GadToolsBase,
			  struct TagItem stdgadtags[],
			  struct VisualInfo *vi,
			  struct TagItem *taglist)
{
    struct Gadget *obj;
    Class *cl;
    struct TagItem tags[] =
    {
	{GA_Disabled, FALSE},
	{GA_Immediate, FALSE},
	{GA_RelVerify, TRUE},
	{TAG_MORE, (IPTR) NULL}
    };

    cl = makebuttonclass(GadToolsBase);
    if (!cl)
	return NULL;

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GA_Immediate, FALSE, taglist);
    tags[3].ti_Data = (IPTR) stdgadtags;

    obj = (struct Gadget *) NewObjectA(cl, NULL, tags);

    return obj;
}


struct Gadget *makecheckbox(struct GadToolsBase_intern *GadToolsBase,
			    struct TagItem stdgadtags[],
			    struct VisualInfo *vi,
			    struct TagItem *taglist)
{
    struct Gadget *obj;
    struct TagItem tags[] =
    {
	{GA_Disabled, FALSE},
	{GTCB_Checked, FALSE},
	{TAG_MORE, (IPTR) NULL}
    };

    if (!GadToolsBase->aroscbbase)
        GadToolsBase->aroscbbase = OpenLibrary("SYS:Classes/Gadgets/aroscheckbox.gadget", 0);
    if (!GadToolsBase->aroscbbase)
        return NULL;

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GTCB_Checked, FALSE, taglist);
    tags[2].ti_Data = (IPTR) stdgadtags;

    if (!GetTagData(GTCB_Scaled, FALSE, taglist)) {
	stdgadtags[TAG_Width].ti_Data = CHECKBOX_WIDTH;
	stdgadtags[TAG_Height].ti_Data = CHECKBOX_HEIGHT;
    }
    obj = (struct Gadget *) NewObjectA(NULL, AROSCHECKBOXCLASS, tags);

    return obj;
}


struct Gadget *makemx(struct GadToolsBase_intern *GadToolsBase,
		      struct TagItem stdgadtags[],
		      struct VisualInfo *vi,
		      struct TagItem *taglist)
{
    struct Gadget *gad;
    int labels = 0;
    UWORD height;
    STRPTR *labellist;
    struct TagItem *tag, tags[] =
    {
	{GA_Disabled, FALSE},
	{GTMX_Labels, (IPTR) NULL},
	{GTMX_Active, 0},
	{GTMX_Spacing, 1},
	{GA_LabelPlace, GV_LabelPlace_Above},
	{TAG_MORE, (IPTR) NULL}
    };

    if (!GadToolsBase->arosmxbase)
        GadToolsBase->arosmxbase = OpenLibrary("SYS:Classes/Gadgets/arosmutualexclude.gadget", 0);
    if (!GadToolsBase->arosmxbase)
        return NULL;

    height = stdgadtags[TAG_Height].ti_Data;

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    labellist = (STRPTR *) GetTagData(GTMX_Labels, (IPTR) NULL, taglist);
    if (!labellist)
	return NULL;
    tags[1].ti_Data = (IPTR) labellist;
    tags[2].ti_Data = GetTagData(GTMX_Active, 0, taglist);
    tags[3].ti_Data = GetTagData(GTMX_Spacing, 1, taglist);
    tag = FindTagItem(GTMX_TitlePlace, taglist);
    if (tag)
    {
        switch (tag.ti_Data)
        {
        case PLACETEXT_LEFT:
            tags[4].ti_Data = GV_LabelPlace_Left;
            break;
        case PLACETEXT_RIGHT:
            tags[4].ti_Data = GV_LabelPlace_Right;
            break;
        case PLACETEXT_ABOVE:
            tags[4].ti_Data = GV_LabelPlace_Above;
            break;
        case PLACETEXT_BELOW:
            tags[4].ti_Data = GV_LabelPlace_Below;
            break;
        }
    }
    tags[5].ti_Data = (IPTR) stdgadtags;

    if (!GetTagData(GTMX_Scaled, FALSE, taglist)) {
	stdgadtags[TAG_Width].ti_Data = MX_WIDTH;
	height = MX_HEIGHT;
    }
    while (labellist[labels])
	labels++;
    stdgadtags[TAG_Height].ti_Data = (height + tags[3].ti_Data) * labels - tags[3].ti_Data;

    gad = (struct Gadget *) NewObjectA(NULL, AROSMXCLASS, tags);

    return gad;
}
