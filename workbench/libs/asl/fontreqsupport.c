/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

/*****************************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/diskfont.h>
#include <exec/memory.h>
#include <exec/initializers.h>
#include <intuition/imageclass.h>
#include <dos/dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <clib/macros.h>

#include "asl_intern.h"
#include "fontreqsupport.h"
#include "fontreqhooks.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 0
//#define ADEBUG 0

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

static int AVFCompare(struct AvailFonts *one, struct AvailFonts *two)
{
    int retval = strcmp(one->af_Attr.ta_Name, two->af_Attr.ta_Name);
    
    if (!retval) retval = ((int)one->af_Attr.ta_YSize) -
    	    	    	  ((int)two->af_Attr.ta_YSize);
			  
    return retval; 
}

/*****************************************************************************************/

static void SortAvailFonts(struct AvailFontsHeader *afh, struct AslBase_intern *AslBase)
{
    struct AvailFonts 	*avf;
    WORD    	    	numentries;
    
    avf = (struct AvailFonts *)&afh[1];
    numentries = afh->afh_NumEntries;
    if (numentries < 2) return;
    
    qsort(avf,
    	  numentries,
	  sizeof(*avf),
	  (int (*)(const void *, const void *))AVFCompare);	 
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
    
    SortAvailFonts(udata->AFH, AslBase);
    
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
#ifdef __MORPHOS__
	    {
		ULONG ret;

		REG_A4 = (ULONG)iforeq->ifo_IntReq.ir_BasePtr;	/* Compatability */
		REG_A0 = (ULONG)iforeq->ifo_FilterFunc;
		REG_A2 = (ULONG)ld->ld_Req;
		REG_A1 = (ULONG)&avf_start->af_Attr;
		ret = (*MyEmulHandle->EmulCallDirect68k)(iforeq->ifo_FilterFunc->h_Entry);

		if (!ret) continue;
	    }
#else
	    if (!(CallHookPkt(iforeq->ifo_FilterFunc, ld->ld_Req, &avf_start->af_Attr))) continue;
#endif
	}
	
	if (iforeq->ifo_HookFunc && (iforeq->ifo_Flags & FOF_FILTERFUNC))
	{
#ifdef __MORPHOS__
	    {
		ULONG ret;
		UWORD *funcptr = iforeq->ifo_HookFunc;
		ULONG *p = (ULONG *)REG_A7 - 3;

		p[0] = (ULONG)FOF_FILTERFUNC;
		p[1] = (ULONG)&avf_start->af_Attr;
		p[2] = (ULONG)ld->ld_Req;
		REG_A7 = (ULONG)p;

		if (*funcptr >= (UWORD)0xFF00)
		    REG_A7 -= 4;

		REG_A4 = (ULONG)iforeq->ifo_IntReq.ir_BasePtr;	/* Compatability */

		ret = (ULONG)(*MyEmulHandle->EmulCallDirect68k)(funcptr);

		if (*funcptr >= (UWORD)0xFF00)
		    REG_A7 += 4;

		REG_A7 += (3*4);

		if (!ret) continue;
	    }
#else
	    if (!(iforeq->ifo_HookFunc(FOF_FILTERFUNC,
		    	    	       &avf_start->af_Attr,
				       (struct FontRequester *)ld->ld_Req))) continue;
#endif
	}
	
	fontnode = MyAllocVecPooled(ld->ld_IntReq->ir_MemPool,
	    	    	    	  sizeof(*fontnode) + sizeof(struct Node) * num_sizes,
				  AslBase);
	
	if (fontnode)
	{
	    char  *sp;
	    WORD  len;
	    UWORD prevsize = 0;
	    
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
	    fontnode->TAttr = avf_start->af_Attr;
	    fontnode->TAttr.ta_Name = fontnode->Name;
	    
	    fontnode->NumSizes = num_sizes;
	    
	    NEWLIST(&fontnode->SizeList);
	    
	    for(i2 = 0; i2 < num_sizes; i2++, avf_start++)
	    {
	    	UWORD size = avf_start->af_Attr.ta_YSize;
		
		if (size == prevsize) continue;
		
		if ((size < iforeq->ifo_MinHeight) ||
		    (size > iforeq->ifo_MaxHeight)) continue;
		    
	    	fontnode->SizeNode[i2].ln_Name = (char *)(IPTR)size;
	    	SortInNode(iforeq, &fontnode->SizeList, &fontnode->SizeNode[i2], (APTR)FOCompareSizeNodes, AslBase);

    	    	prevsize = size;
	    }

	    SortInNode(iforeq, &udata->NameListviewList, &fontnode->node, (APTR)FOCompareFontNodes, AslBase);

	}
	
    } /* for(i = 0; i < udata->AFH->afh_NumEntries; ) */
    
    if (udata->NameListview)
    {
	struct TagItem set_tags[] =
	{
    	    {ASLLV_Labels,	(IPTR)&udata->NameListviewList  },
	    {TAG_DONE   	    	    	    	    	}
	};
    	STRPTR fontname;
	IPTR   fontsize;
	
    	SetGadgetAttrsA((struct Gadget *)udata->NameListview, ld->ld_Window, NULL, set_tags);
	
	GetAttr(STRINGA_TextVal, udata->NameString, (IPTR *)&fontname);
	GetAttr(STRINGA_LongVal, udata->SizeString, (IPTR *)&fontsize);
	
	FORestore(ld, fontname, fontsize, AslBase);

    }
    
    return IsListEmpty(&udata->NameListviewList) ? ERROR_NO_MORE_ENTRIES : 0;
        
}

/*****************************************************************************************/

void FOFreeFonts(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData 	    *udata = (struct FOUserData *)ld->ld_UserData;
    struct ASLVFontReqNode  *node, *succ;    
    struct TagItem  	    set_tags[] =
    {
    	{ASLLV_Labels,	0   },
	{TAG_DONE   	    }
    };
    
    udata->ActiveFont = NULL;

    if (udata->NameListview) SetGadgetAttrsA((struct Gadget *)udata->NameListview, ld->ld_Window, NULL, set_tags);
    if (udata->SizeListview) SetGadgetAttrsA((struct Gadget *)udata->SizeListview, ld->ld_Window, NULL, set_tags);
   
    ForeachNodeSafe(&udata->NameListviewList, node, succ)
    {
    	MyFreeVecPooled(node, AslBase);
    }
    
    NEWLIST(&udata->NameListviewList);
    
    if (udata->AFH)
    {
    	FreeVec(udata->AFH);
	udata->AFH = NULL;
    }
}

/*****************************************************************************************/

VOID FOUpdatePreview(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData 	    *udata = (struct FOUserData *)ld->ld_UserData;
    struct IntReq 	    *intreq = ld->ld_IntReq;
    struct IntFontReq 	    *iforeq = (struct IntFontReq *)ld->ld_IntReq;
    struct TextAttr 	     ta;
    struct TextFont 	    *font;
    IPTR    	    	     val;
    STRPTR  	    	     name;
    
    GetAttr(STRINGA_LongVal, udata->SizeString, &val);
    ta.ta_YSize = (WORD)val;
    ta.ta_Flags = 0;
    ta.ta_Style = 0;
    
    GetAttr(STRINGA_TextVal, udata->NameString, (IPTR *)&name);
    if ((name = VecPooledCloneString(name, ".font", intreq->ir_MemPool, AslBase)))
    {
    	UBYTE style = FS_NORMAL;
	UBYTE apen = iforeq->ifo_FrontPen;
	UBYTE bpen = iforeq->ifo_BackPen;
	
    	ta.ta_Name = name;
	
	if (iforeq->ifo_Flags & FOF_DOSTYLE)
	{
	    style = FOGetStyle(ld, AslBase);
	}
	
	if (iforeq->ifo_Flags & FOF_DOFRONTPEN)
	{
	    apen = FOGetFGColor(ld, AslBase);
	}
	
	if (iforeq->ifo_Flags & FOF_DOBACKPEN)
	{
	    bpen = FOGetBGColor(ld, AslBase);
	}
	
	font = OpenDiskFont(&ta);
	{
	    struct TagItem settags[] =
	    {
	    	{ASLFP_Font , (IPTR)font    },
		{ASLFP_Style, style	    },
		{ASLFP_APen , apen	    },
		{ASLFP_BPen , bpen  	    },
		{TAG_DONE   	    	    }
	    };
	    
	    SetGadgetAttrsA((struct Gadget *)udata->Preview, ld->ld_Window, NULL, settags);
	}  
	   
	if (udata->PreviewFont) CloseFont(udata->PreviewFont);
	udata->PreviewFont = font;
	
    	MyFreeVecPooled(name, AslBase);
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

void FOChangeActiveFont(struct LayoutData *ld, WORD delta, UWORD quali, BOOL jump, struct AslBase_intern *AslBase)
{
    struct FOUserData 	*udata = (struct FOUserData *)ld->ld_UserData;    
    IPTR 		active, total, visible, size;
   
    GetAttr(ASLLV_Active   , udata->NameListview, &active );
    GetAttr(ASLLV_Total    , udata->NameListview, &total  );
    GetAttr(ASLLV_Visible  , udata->NameListview, &visible);
    GetAttr(STRINGA_LongVal, udata->SizeString  , &size   );
    
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
    	else if (jump)
	{
            /* try to jump to first item which matches text in name string gadget,
	       but only if text in string gadget mismatches actual active
	       item's text (in this case move normally = one step)) */

    	    struct ASLLVFontReqNode *node;
	    UBYTE   	    	    buffer[MAXFONTNAME + 2];
	    STRPTR  	    	    val;
	    WORD    	    	    i, len;
	    BOOL    	    	    dojump = TRUE;

	    GetAttr(STRINGA_TextVal, udata->NameString, (IPTR *)&val);
	    strcpy(buffer, val);

	    len = strlen(buffer);

	    if (len)
	    {
		if (((LONG)active) >= 0)
		{
		    if ((node = (struct ASLLVFontReqNode *)FindListNode(&udata->NameListviewList, (WORD)active)))
		    {
	        	if (stricmp(node->node.ln_Name, buffer) == 0) dojump = FALSE;
		    }     
		}

		if (dojump)
		{
		    i = 0;
		    ForeachNode(&udata->NameListviewList, node)
		    {
			if (Strnicmp((CONST_STRPTR)node->node.ln_Name, (CONST_STRPTR)buffer, len) == 0)
			{
			    active = i;
			    delta = 0;
			    break;
			}
			i++;
		    }

		} /* if (dojump) */

	    } /* if (len) */
	}
	
	active += delta;

	if (((LONG)active) < 0) active = 0;
	if (active >= total) active = total - 1;

	FOActivateFont(ld, active, (LONG)size, AslBase);
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
	{ASLLV_Active	    	, (IPTR)-1    	},
	{ASLLV_MakeVisible  	, 0 	    	},
	{TAG_DONE   	    	    	    	}
    };
    WORD    	    	    sizelvindex = 0;
    
    fontnode = (struct ASLLVFontReqNode *)FindListNode(&udata->NameListviewList, which);
    udata->ActiveFont = fontnode;
    
    if (!fontnode)
    {
    	SetGadgetAttrsA((struct Gadget *)udata->SizeListview, ld->ld_Window, NULL, size_tags);

	name_tags[0].ti_Data = (IPTR)-1;
	name_tags[1].ti_Tag = TAG_IGNORE;
        SetGadgetAttrsA((struct Gadget *)udata->NameListview, ld->ld_Window, NULL, name_tags);	
    }
    else
    {
	name_tags[0].ti_Data = name_tags[1].ti_Data = which;
	SetGadgetAttrsA((struct Gadget *)udata->NameListview, ld->ld_Window, NULL, name_tags);

	size_tags[0].ti_Data = (IPTR)&fontnode->SizeList;        
	ForeachNode(&fontnode->SizeList, node)
	{
	    MARK_UNSELECTED(node);
	    if ((LONG)node->ln_Name == size)
	    {
	    	size_tags[1].ti_Data = size_tags[2].ti_Data = sizelvindex;
	    }
	    sizelvindex++;
	}
	SetGadgetAttrsA((struct Gadget *)udata->SizeListview, ld->ld_Window, NULL, size_tags);

	FOSetFontString(fontnode->node.ln_Name, ld, AslBase);	
    }

    FOUpdatePreview(ld, AslBase);
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
    
    if (which >= 0)
    {
        node = FindListNode(&udata->ActiveFont->SizeList, which);    
    }
    else
    {
    	LONG size  = -which;
    	BOOL found = FALSE;
	
    	which = 0;
	ForeachNode(&udata->ActiveFont->SizeList, node)
	{
	    if ((LONG)node->ln_Name == size)
	    {
	    	found = TRUE;
		break;
	    }
	    which++;
	}
	
	if (!found) node = NULL;
    }
    
    size_tags[0].ti_Data = node ? which : -1;
    size_tags[1].ti_Data = node ? which : 0;
    
    SetGadgetAttrsA((struct Gadget *)udata->SizeListview, ld->ld_Window, NULL, size_tags);

    if (node) FOSetSizeString((LONG)node->ln_Name, ld, AslBase);    

    FOUpdatePreview(ld, AslBase);
}

/*****************************************************************************************/

void FORestore(struct LayoutData *ld, STRPTR fontname, LONG fontsize, struct AslBase_intern *AslBase)
{
    struct FOUserData 	    *udata = (struct FOUserData *)ld->ld_UserData;
    struct IntFontReq 	    *iforeq = (struct IntFontReq *)ld->ld_IntReq;
    struct ASLLVFontReqNode *fontnode;
    struct TagItem  	    set_tags[] =
    {
    	{ASLLV_Labels	, 0 	},
	{TAG_DONE   	    	}
    };
    UBYTE   	    	    initialfontname[MAXFONTNAME + 2];  
    char    	    	    *sp;
    WORD    	    	    i = 0;
    
    strncpy(initialfontname, fontname, MAXFONTNAME + 1);
    if ((sp = strchr(initialfontname, '.'))) *sp = '\0';
	
    FOSetSizeString(fontsize, ld, AslBase);
    
    SetGadgetAttrsA((struct Gadget *)udata->SizeListview, ld->ld_Window, NULL, set_tags);
    
    ForeachNode(&udata->NameListviewList, fontnode)
    {
    	if (stricmp(fontnode->node.ln_Name, initialfontname) == 0) break;
	i++;
    }
        
    if (iforeq->ifo_Flags & FOF_DODRAWMODE)
    {
    	FOSetDrawMode(ld, iforeq->ifo_DrawMode, AslBase);
    }
    
    if (iforeq->ifo_Flags & FOF_DOSTYLE)
    {
    	FOSetStyle(ld, iforeq->ifo_TextAttr.ta_Style, AslBase);
    }

    if (iforeq->ifo_Flags & FOF_DOFRONTPEN)
    {
    	FOSetFGColor(ld, iforeq->ifo_FrontPen, AslBase);
    }
    
    if (iforeq->ifo_Flags & FOF_DOBACKPEN)
    {
    	FOSetBGColor(ld, iforeq->ifo_BackPen, AslBase);
    }
    
    FOActivateFont(ld, i, fontsize, AslBase);    
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

LONG FOGetDrawMode(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData   *udata = (struct FOUserData *)ld->ld_UserData;  
    IPTR		active;
    
    ASSERT(udata->DrawModeGadget);
    
    GetAttr(ASLCY_Active, udata->DrawModeGadget, &active);
    
    return (LONG)active;
}

/*****************************************************************************************/

void FOSetDrawMode(struct LayoutData *ld, UWORD id, struct AslBase_intern *AslBase)
{
    struct FOUserData   *udata = (struct FOUserData *)ld->ld_UserData;  
    struct TagItem	set_tags[] =
    {
        {ASLCY_Active	, id		},
	{TAG_DONE		   	}
    };
    
    ASSERT(udata->DrawModeGadget);
    
    SetGadgetAttrsA((struct Gadget *)udata->DrawModeGadget, ld->ld_Window, NULL, set_tags);
    
}

/*****************************************************************************************/

UBYTE FOGetStyle(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData   *udata = (struct FOUserData *)ld->ld_UserData;  
    IPTR		style;
    
    ASSERT(udata->StyleGadget);
    
    GetAttr(ASLFS_Style, udata->StyleGadget, &style);
    
    return (UBYTE)style;
}

/*****************************************************************************************/

void FOSetStyle(struct LayoutData *ld, UBYTE style, struct AslBase_intern *AslBase)
{
    struct FOUserData   *udata = (struct FOUserData *)ld->ld_UserData;  
    struct TagItem	set_tags[] =
    {
        {ASLFS_Style	, style		},
	{TAG_DONE		   	}
    };
    
    ASSERT(udata->StyleGadget);
    
    SetGadgetAttrsA((struct Gadget *)udata->StyleGadget, ld->ld_Window, NULL, set_tags);
    
}

/*****************************************************************************************/

UBYTE FOGetFGColor(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData   *udata = (struct FOUserData *)ld->ld_UserData;  
    IPTR		color;
    
    ASSERT(udata->FGColorGadget);
    
    GetAttr(ASLCP_Color, udata->FGColorGadget, &color);
    
    return (UBYTE)color;
}

/*****************************************************************************************/

void FOSetFGColor(struct LayoutData *ld, UBYTE color, struct AslBase_intern *AslBase)
{
    struct FOUserData   *udata = (struct FOUserData *)ld->ld_UserData;  
    struct TagItem	set_tags[] =
    {
        {ASLCP_Color	, color		},
	{TAG_DONE		   	}
    };
    
    ASSERT(udata->FGColorGadget);
    
    SetGadgetAttrsA((struct Gadget *)udata->FGColorGadget, ld->ld_Window, NULL, set_tags);
    
}

/*****************************************************************************************/

UBYTE FOGetBGColor(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData   *udata = (struct FOUserData *)ld->ld_UserData;  
    IPTR		color;
    
    ASSERT(udata->BGColorGadget);
    
    GetAttr(ASLCP_Color, udata->BGColorGadget, &color);
    
    return (UBYTE)color;
}

/*****************************************************************************************/

void FOSetBGColor(struct LayoutData *ld, UBYTE color, struct AslBase_intern *AslBase)
{
    struct FOUserData   *udata = (struct FOUserData *)ld->ld_UserData;  
    struct TagItem	set_tags[] =
    {
        {ASLCP_Color	, color		},
	{TAG_DONE		   	}
    };
    
    ASSERT(udata->BGColorGadget);
    
    SetGadgetAttrsA((struct Gadget *)udata->BGColorGadget, ld->ld_Window, NULL, set_tags);
    
}

/*****************************************************************************************/

