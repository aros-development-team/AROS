/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#define USE_BOOPSI_STUBS
#include <utility/tagitem.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <clib/boopsistubs.h>
#include "datatypes_intern.h"

struct LayoutMessage
{
    struct Message        lm_Msg;
    struct DataTypesBase *lm_dtb;
    Object               *lm_object;
    struct Window        *lm_window;
    struct gpLayout       lm_gplayout;
    struct GadgetInfo     lm_ginfo;
};


void AsyncLayouter(void)
{
    struct DataTypesBase *dtb;
    struct LayoutMessage *lm;
    struct DTSpecialInfo *dtsi;
    Object *object;
    
    struct Process *MyProc = (struct Process *)FindTask(NULL);
    
    WaitPort(&MyProc->pr_MsgPort);
    lm = (struct LayoutMessage *)GetMsg(&MyProc->pr_MsgPort);
    
    dtb = lm->lm_dtb;
    
    object = lm->lm_object;
    dtsi = ((struct Gadget *)object)->SpecialInfo;
   
    ObtainSemaphore(&dtsi->si_Lock);
   
    lm->lm_gplayout.MethodID = DTM_ASYNCLAYOUT;
    
    do
    {
	dtsi->si_Flags &= ~DTSIF_NEWSIZE;
	
	DoMethodA(object, (Msg)&lm->lm_gplayout);
	
    } while(dtsi->si_Flags & DTSIF_NEWSIZE);
   
    setattrs((struct Library *)dtb, object, DTA_LayoutProc, NULL, TAG_DONE);
   
    dtsi->si_Flags &= ~(DTSIF_LAYOUT | DTSIF_NEWSIZE);
   
    ReleaseSemaphore(&dtsi->si_Lock);
   
    DoGad_OM_NOTIFY((struct Library *)dtb, object, lm->lm_window, NULL, 0, 
		    GA_ID, (ULONG)((struct Gadget *)object)->GadgetID,
		    DTA_Data,         object,
		    DTA_TopVert,      dtsi->si_TopVert,
		    DTA_VisibleVert,  dtsi->si_VisVert,
		    DTA_TotalVert,    dtsi->si_TotVert,
		    DTA_TopHoriz,     dtsi->si_TopHoriz,
		    DTA_VisibleHoriz, dtsi->si_VisHoriz,
		    DTA_TotalHoriz,   dtsi->si_TotHoriz,
		    DTA_Sync,         TRUE,
		    DTA_Busy,         FALSE,
		    TAG_DONE);
   
    Forbid();
    
    dtsi->si_Flags &= ~DTSIF_LAYOUTPROC;
   
    FreeVec(lm);
}


/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH2(ULONG, DoAsyncLayout,

/*  SYNOPSIS */
	AROS_LHA(Object *         , object, A0),
	AROS_LHA(struct gpLayout *, gpl   , A1),

/*  LOCATION */
	struct Library *, DataTypesBase, 14, DataTypes)

/*  FUNCTION

    Perform an object's DTM_ASYNCLAYOUT method -- doing it asynchronously
    off loads the input.device. The method should exit when a SIGBREAK_CTRL_C
    is received; this signal means that the data is obsolete and the the
    method will be called again.

    INPUTS

    object  --  pointer to data type object
    gpl     --  gpLayout message pointer

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG retval = FALSE;
    struct DTSpecialInfo *dtsi=((struct Gadget *)object)->SpecialInfo;
    struct Process *LayoutProc;
    
    ObtainSemaphore(&(GPB(DataTypesBase)->dtb_Semaphores[SEM_ASYNC]));
    
    if(dtsi->si_Flags & DTSIF_LAYOUT)
    {
	dtsi->si_Flags |= DTSIF_NEWSIZE;
	
	if(GetAttr(DTA_LayoutProc, object, (ULONG *)&LayoutProc))
	{
	    if(LayoutProc != NULL)
		Signal((struct Task *)LayoutProc, SIGBREAKF_CTRL_C);
	}
      
	retval = TRUE;
    }
    else
    {
	struct LayoutMessage *lm;
	
	dtsi->si_Flags |= (DTSIF_LAYOUT | DTSIF_LAYOUTPROC);
	
	Do_OM_NOTIFY(DataTypesBase, object, gpl->gpl_GInfo, 0, DTA_Data, NULL,
		     TAG_DONE);
	
	if((lm = AllocVec(sizeof(struct LayoutMessage),
			 MEMF_PUBLIC | MEMF_CLEAR)))
	{
	    struct TagItem Tags[5];
	    
	    lm->lm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	    lm->lm_Msg.mn_Length = sizeof(struct LayoutMessage);
	    lm->lm_dtb = (struct DataTypesBase *)DataTypesBase;
	    lm->lm_object = object;
	    lm->lm_window = gpl->gpl_GInfo->gi_Window;
	    lm->lm_gplayout = *gpl;
	    lm->lm_ginfo= *gpl->gpl_GInfo;
	    lm->lm_gplayout.gpl_GInfo= &lm->lm_ginfo;
	    
	    Tags[0].ti_Tag  = NP_StackSize;
	    Tags[0].ti_Data = AROS_STACKSIZE;
	    Tags[1].ti_Tag  = NP_Entry;
	    Tags[1].ti_Data = (ULONG)&AsyncLayouter;
	    Tags[2].ti_Tag  = NP_Priority;
	    Tags[2].ti_Data = 0;
	    Tags[3].ti_Tag  = NP_Name;
	    Tags[3].ti_Data = (ULONG)"AsyncLayoutDaemon";
	    Tags[4].ti_Tag  = TAG_DONE;
	 
	    if((LayoutProc = CreateNewProc(Tags)))
	    {
		PutMsg(&LayoutProc->pr_MsgPort, &lm->lm_Msg);
	    
		setattrs(DataTypesBase, object, DTA_LayoutProc, LayoutProc, TAG_DONE);
		retval = TRUE;
	    }
	    else
		FreeVec(lm);
	}
	
	if (!retval) dtsi->si_Flags &= ~(DTSIF_LAYOUT | DTSIF_LAYOUTPROC);
    }
    
    ReleaseSemaphore(&(GPB(DataTypesBase)->dtb_Semaphores[SEM_ASYNC]));
    
    return retval;

    AROS_LIBFUNC_EXIT
    
} /* DoAsyncLayout */
