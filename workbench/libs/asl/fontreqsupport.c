/*
    (C) 1995-97 AROS - The Amiga Research OS

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

static WORD FOCompareNodes(struct IntFontReq *iforeq, struct Node *node1,
			   struct Node *node2, struct AslBase_intern *AslBase)
{
    return Stricmp(node1->ln_Name, node2->ln_Name);
}

/*****************************************************************************************/

LONG FOGetFonts(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData 	 *udata = (struct FOUserData *)ld->ld_UserData;	
    struct IntFontReq 	 *iforeq = (struct IntFontReq *)ld->ld_IntReq;
    struct AvailFonts	 *avf;
    ULONG   	    	 afshortage, afsize = 100;
    WORD   	    	 i;
    
    do
    {
    	udata->AFH = (struct AvailFontsHeader *)AllocVec(afsize, MEMF_ANY);
	if (udata->AFH)
	{
	    afshortage = AvailFonts((STRPTR)udata->AFH,
	    	    	    	    afsize,
				    (iforeq->ifo_TextAttr.ta_Flags & FPF_ROMFONT) ? AFF_MEMORY : AFF_MEMORY | AFF_DISK);

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

    	if (iforeq->ifo_FilterFunc)
	{
	    if (!(CallHookPkt(iforeq->ifo_FilterFunc, ld->ld_Req, &avf->af_Attr))) continue;
	}
	
	if (iforeq->ifo_HookFunc && (iforeq->ifo_Flags & FOF_FILTERFUNC))
	{
	    if (!(iforeq->ifo_HookFunc(FOF_FILTERFUNC,
		    	    	       &avf->af_Attr,
				       (struct FontRequester *)ld->ld_Req))) continue;
	}

    	while (i2 < udata->AFH->afh_NumEntries - 1)
	{
	    i2++; avf++;	    
	    if (strcmp(avf_start->af_Attr.ta_Name, avf->af_Attr.ta_Name)) break;
	    num_sizes++;
	}
		
    	i += num_sizes; avf = avf_start + num_sizes;
	
	fontnode = AllocVecPooled(ld->ld_IntReq->ir_MemPool,
	    	    	    	  sizeof(*fontnode) + sizeof(struct Node) * num_sizes,
				  AslBase);
	
	if (fontnode)
	{
	    strcpy(fontnode->Name, avf_start->af_Attr.ta_Name);
	    fontnode->node.ln_Name = fontnode->Name;
	    fontnode->NumSizes = num_sizes;
	    
	    NEWLIST(&fontnode->SizeList);
	    
	    for(i2 = 0; i2 < num_sizes; i2++, avf_start++)
	    {
	    	fontnode->SizeNode[i2].ln_Name = (char *)(ULONG)avf_start->af_Attr.ta_YSize;
		AddTail(&fontnode->SizeList, &fontnode->SizeNode[i2]);
	    }

	    SortInNode(iforeq, &udata->NameListviewList, &fontnode->node, (APTR)FOCompareNodes, AslBase);

	}
	
    } /* for(i = 0; i < udata->AFH->afh_NumEntries; ) */
    
    return IsListEmpty(&udata->NameListviewList) ? ERROR_NO_MORE_ENTRIES : 0;
        
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/











