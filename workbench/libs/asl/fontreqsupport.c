/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/diskfont.h>
#include <exec/memory.h>
#include <exec/initializers.h>
#include <dos/dos.h>
#include <stdio.h>
#include <string.h>
#include <clib/macros.h>

#include "asl_intern.h"
#include "fontreqsupport.h"
#include "fontreqhooks.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 1
#define ADEBUG 1

#include <aros/debug.h>

/*****************************************************************************************/

static WORD FOCompareFontNodes(struct IntFontReq *iforeq, struct Node *node1,
			   struct Node *node2, struct AslBase_intern *AslBase)
{
    return Stricmp(node1->ln_Name, node2->ln_Name);
}

/*****************************************************************************************/

static WORD FOCompareSizeNodes(struct IntFontReq *iforeq, struct Node *node1,
			   struct Node *node2, struct AslBase_intern *AslBase)
{
    return ((LONG)node1->ln_Name) - ((LONG)node2->ln_Name);
}

/*****************************************************************************************/

LONG FOGetFonts(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData 	 *udata = (struct FOUserData *)ld->ld_UserData;	
    struct IntFontReq 	 *iforeq = (struct IntFontReq *)ld->ld_IntReq;
    struct AvailFonts	 *avf;
    ULONG   	    	 afshortage, afsize = 100;
    WORD   	    	 i;
    
    FOFreeFonts(ld, AslBase);
    
    do
    {
    	udata->AFH = (struct AvailFontsHeader *)AllocVec(afsize, MEMF_ANY);
	if (udata->AFH)
	{
	    afshortage = AvailFonts((STRPTR)udata->AFH, afsize, AFF_MEMORY | AFF_DISK);

    	    if (afshortage)
	    {
	    	FreeVec(udata->AFH);
		afsize += afshortage;
	    }
	}
	
    } while (udata->AFH && afshortage);
    
    if (!udata->AFH) return ERROR_NO_FREE_STORE;
    
    avf = (struct AvailFonts *)&udata->AFH[1];
    
    for(i = 0; i < udata->AFH->afh_NumEntries;)
    {
    	struct AvailFonts   	*avf_start = avf;
	struct ASLLVFontReqNode	*fontnode;
	UWORD	    	     	num_sizes = 1;
	WORD	    	    	i2 = i;

    	while (i2 < udata->AFH->afh_NumEntries - 1)
	{
	    i2++; avf++;	    
	    if (strcmp(avf_start->af_Attr.ta_Name, avf->af_Attr.ta_Name)) break;
	    num_sizes++;
	}
		
    	i += num_sizes; avf = avf_start + num_sizes;

    	if (iforeq->ifo_Flags & FOF_FIXEDWIDTHONLY)
	{
	    if (avf_start->af_Attr.ta_Flags & FPF_PROPORTIONAL) continue;
	}
	
    	if (iforeq->ifo_FilterFunc)
	{
	    if (!(CallHookPkt(iforeq->ifo_FilterFunc, ld->ld_Req, &avf_start->af_Attr))) continue;
	}
	
	if (iforeq->ifo_HookFunc && (iforeq->ifo_Flags & FOF_FILTERFUNC))
	{
	    if (!(iforeq->ifo_HookFunc(FOF_FILTERFUNC,
		    	    	       &avf_start->af_Attr,
				       (struct FontRequester *)ld->ld_Req))) continue;
	}
	
	fontnode = AllocVecPooled(ld->ld_IntReq->ir_MemPool,
	    	    	    	  sizeof(*fontnode) + sizeof(struct Node) * num_sizes,
				  AslBase);
	
	if (fontnode)
	{
	    char *sp;
	    WORD len;
	    
	    sp = strchr(avf_start->af_Attr.ta_Name, '.');
	    if (sp)
	    {
	    	len = (IPTR)sp - (IPTR)avf_start->af_Attr.ta_Name - 1;;
	    }
	    else
	    {
	    	/* Paranoia: Should never happen */
		len = strlen(avf_start->af_Attr.ta_Name);
	    }
	    
	    strncpy(fontnode->Name, avf_start->af_Attr.ta_Name, len + 1);
	    fontnode->node.ln_Name = fontnode->Name;
	    fontnode->NumSizes = num_sizes;
	    
	    NEWLIST(&fontnode->SizeList);
	    
	    for(i2 = 0; i2 < num_sizes; i2++, avf_start++)
	    {
	    	UWORD size = avf_start->af_Attr.ta_YSize;
		
		if ((size < iforeq->ifo_MinHeight) ||
		    (size > iforeq->ifo_MaxHeight)) continue;
		    
	    	fontnode->SizeNode[i2].ln_Name = (char *)(IPTR)size;
	    	SortInNode(iforeq, &fontnode->SizeList, &fontnode->SizeNode[i2], (APTR)FOCompareSizeNodes, AslBase);
	    }

	    SortInNode(iforeq, &udata->NameListviewList, &fontnode->node, (APTR)FOCompareFontNodes, AslBase);

	}
	
    } /* for(i = 0; i < udata->AFH->afh_NumEntries; ) */
    
    return IsListEmpty(&udata->NameListviewList) ? ERROR_NO_MORE_ENTRIES : 0;
        
}

/*****************************************************************************************/

void FOFreeFonts(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData 	    *udata = (struct FOUserData *)ld->ld_UserData;
    struct ASLVFontReqNode  *node, *succ;    
    struct TagItem  	    set_tags[] =
    {
    	{ASLLV_Labels,	NULL},
	{TAG_DONE   	    }
    };
    
    udata->ActiveFont = NULL;

    if (udata->NameListview) SetGadgetAttrsA((struct Gadget *)udata->NameListview, ld->ld_Window, NULL, set_tags);
    if (udata->SizeListview) SetGadgetAttrsA((struct Gadget *)udata->SizeListview, ld->ld_Window, NULL, set_tags);
   
    ForeachNodeSafe(&udata->NameListviewList, node, succ)
    {
    	FreeVecPooled(node, AslBase);
    }
    
    NEWLIST(&udata->NameListviewList);
    NEWLIST(&udata->SizeListviewList);
    
    if (udata->AFH)
    {
    	FreeVec(udata->AFH);
	udata->AFH = NULL;
    }
}

/*****************************************************************************************/

struct ASLLVFontReqNode *FOGetActiveFont(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData 	*udata = (struct FOUserData *)ld->ld_UserData;    
    IPTR		active;
    
    GetAttr(ASLLV_Active, udata->NameListview, &active);
    
    return (struct ASLLVFontReqNode *)FindListNode(&udata->NameListviewList, active);
}

/*****************************************************************************************/

void FOChangeActiveFont(struct LayoutData *ld, WORD delta, UWORD quali, struct AslBase_intern *AslBase)
{
    struct FOUserData 	*udata = (struct FOUserData *)ld->ld_UserData;    
    IPTR 		active, total, visible;
   
    GetAttr(ASLLV_Active , udata->NameListview, &active );
    GetAttr(ASLLV_Total  , udata->NameListview, &total  );
    GetAttr(ASLLV_Visible, udata->NameListview, &visible);
    
    if (total)
    {
	if (quali & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
	{
            delta *= (visible - 1);
	}
	else if (quali & (IEQUALIFIER_LALT | IEQUALIFIER_RALT | IEQUALIFIER_CONTROL))
	{
            delta *= total;
	}

	active += delta;

	if (((LONG)active) < 0) active = 0;
	if (active >= total) active = total - 1;

	FOActivateFont(ld, active, 0, AslBase);
    }
}

/*****************************************************************************************/

void FOChangeActiveSize(struct LayoutData *ld, WORD delta, UWORD quali, struct AslBase_intern *AslBase)
{
    struct FOUserData 	*udata = (struct FOUserData *)ld->ld_UserData;    
    IPTR 		active, total, visible;
   
    GetAttr(ASLLV_Active , udata->SizeListview, &active );
    GetAttr(ASLLV_Total  , udata->SizeListview, &total  );
    GetAttr(ASLLV_Visible, udata->SizeListview, &visible);
    
    if (total)
    {
	if (quali & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
	{
            delta *= (visible - 1);
	}
	else if (quali & (IEQUALIFIER_LALT | IEQUALIFIER_RALT | IEQUALIFIER_CONTROL))
	{
            delta *= total;
	}

	active += delta;

	if (((LONG)active) < 0) active = 0;
	if (active >= total) active = total - 1;

	FOActivateSize(ld, active, AslBase);
    }
}

/*****************************************************************************************/

void FOActivateFont(struct LayoutData *ld, WORD which, LONG size, struct AslBase_intern *AslBase)
{
    struct FOUserData 	    *udata = (struct FOUserData *)ld->ld_UserData;    
//  struct IntFontReq 	    *iforeq = (struct IntFontReq *)ld->ld_IntReq;
    struct ASLLVFontReqNode *fontnode;
    struct Node     	    *node;
    struct TagItem 	    name_tags[] =
    {
        {ASLLV_Active		, 0		},
	{ASLLV_MakeVisible	, 0		},
	{TAG_DONE				}
    };
    struct TagItem  	    size_tags[] =
    {
    	{ASLLV_Labels	    	, 0 	    	},
	{TAG_DONE   	    	    	    	}
    };
    
    fontnode = (struct ASLLVFontReqNode *)FindListNode(&udata->NameListviewList, which);
    
    if (!fontnode) return;
    
    udata->ActiveFont = fontnode;
    
    name_tags[0].ti_Data = name_tags[1].ti_Data = which;
    SetGadgetAttrsA((struct Gadget *)udata->NameListview, ld->ld_Window, NULL, name_tags);

    size_tags[0].ti_Data = (IPTR)&fontnode->SizeList;        
    ForeachNode(&fontnode->SizeList, node)
    {
	MARK_UNSELECTED(node);
    }
    SetGadgetAttrsA((struct Gadget *)udata->SizeListview, ld->ld_Window, NULL, size_tags);
    
    FOSetFontString(fontnode->node.ln_Name, ld, AslBase);	
}

/*****************************************************************************************/

void FOActivateSize(struct LayoutData *ld, WORD which, struct AslBase_intern *AslBase)
{
    struct FOUserData 	    *udata = (struct FOUserData *)ld->ld_UserData;    
//  struct IntFontReq 	    *iforeq = (struct IntFontReq *)ld->ld_IntReq;
    struct Node     	    *node;
    struct TagItem 	    size_tags[] =
    {
        {ASLLV_Active		, 0		},
	{ASLLV_MakeVisible	, 0		},
	{TAG_DONE				}
    };
    
    if (!udata->ActiveFont) return;
    
    node = FindListNode(&udata->ActiveFont->SizeList, which);    
    if (!node) return;
    
    size_tags[0].ti_Data = size_tags[1].ti_Data = which;
    SetGadgetAttrsA((struct Gadget *)udata->SizeListview, ld->ld_Window, NULL, size_tags);
}

/*****************************************************************************************/

void FOSetFontString(STRPTR name, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData 	*udata = (struct FOUserData *)ld->ld_UserData;	
    struct TagItem 	set_tags[] =
    {
	{STRINGA_TextVal	, (IPTR)name	},
	{TAG_DONE				}
    };

    SetGadgetAttrsA((struct Gadget *)udata->NameString, ld->ld_Window, NULL, set_tags);
}

/*****************************************************************************************/

void FOSetSizeString(LONG size, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData 	*udata = (struct FOUserData *)ld->ld_UserData;	
    struct TagItem 	set_tags[] =
    {
	{STRINGA_LongVal	, (IPTR)size	},
	{TAG_DONE				}
    };

    SetGadgetAttrsA((struct Gadget *)udata->SizeString, ld->ld_Window, NULL, set_tags);
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/











