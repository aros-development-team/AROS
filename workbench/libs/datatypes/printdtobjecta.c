/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#define USE_BOOPSI_STUBS
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <utility/tagitem.h>
#include <clib/boopsistubs.h>
#include "datatypes_intern.h"


struct PrintMessage
{
    struct Message        pm_Msg;
    struct DataTypesBase *pm_dtb;
    Object               *pm_object;
    struct Window        *pm_window;
    struct Requester     *pm_requester;
    struct dtPrint        pm_dtprint;
    struct GadgetInfo     pm_ginfo;
};



void AsyncPrinter(void)
{
    struct DataTypesBase *DataTypesBase;
    struct PrintMessage  *pm;
    struct DTSpecialInfo *dtsi;
    Object               *object;
    struct MsgPort       *ReplyPort;
    ULONG                 result = 0;
    
    struct Process *MyProc = (struct Process *)FindTask(NULL);
    
    WaitPort(&MyProc->pr_MsgPort);
    pm = (struct PrintMessage *)GetMsg(&MyProc->pr_MsgPort);
   
    DataTypesBase = pm->pm_dtb;
   
    object = pm->pm_object;
    dtsi = ((struct Gadget *)object)->SpecialInfo;
   
    if((ReplyPort = CreateMsgPort()))
    {
	struct IORequest *IORequest;
	
	if((IORequest = CreateIORequest(ReplyPort, sizeof(union printerIO))))
	{
	    union printerIO *oldPIO;
	    struct Window *window;
	    
	    CopyMem(pm->pm_dtprint.dtp_PIO,IORequest,sizeof(union printerIO));
	    
	    IORequest->io_Message.mn_ReplyPort=ReplyPort;
	    
	    oldPIO = pm->pm_dtprint.dtp_PIO;
	    pm->pm_dtprint.dtp_PIO = (union printerIO *)IORequest;
	    pm->pm_dtprint.dtp_GInfo=NULL;
	    
	    if((window = pm->pm_window))
	    {
		pm->pm_dtprint.dtp_GInfo  = &pm->pm_ginfo;
		pm->pm_ginfo.gi_Screen    = window->WScreen;
		pm->pm_ginfo.gi_Window    = window;
		pm->pm_ginfo.gi_Requester = pm->pm_requester;
		pm->pm_ginfo.gi_RastPort  = window->RPort;

		if(pm->pm_requester != NULL)
		    pm->pm_ginfo.gi_Layer = pm->pm_requester->ReqLayer;
		else
		    pm->pm_ginfo.gi_Layer=window->WLayer;

		pm->pm_ginfo.gi_DrInfo = GetScreenDrawInfo(window->WScreen);
	    }

	    result = DoMethodA(object, (Msg)&pm->pm_dtprint);
	    
	    if (pm->pm_ginfo.gi_DrInfo)
		FreeScreenDrawInfo(window->WScreen, pm->pm_ginfo.gi_DrInfo);
	 
	    pm->pm_dtprint.dtp_PIO = oldPIO;
	 
	    DeleteIORequest(IORequest);
	}

	DeleteMsgPort(ReplyPort);
    }

    setattrs((struct Library *)DataTypesBase, object, DTA_PrinterProc, NULL,
	     TAG_DONE);
   
    DoGad_OM_NOTIFY((struct Library *)DataTypesBase, object, pm->pm_window,
		    pm->pm_requester, 0, GA_ID, 
		    (ULONG)((struct Gadget*)object)->GadgetID,
		    DTA_PrinterStatus, result, TAG_DONE);
   
    Delay(50);
    
    Forbid();
    
    dtsi->si_Flags &= ~DTSIF_PRINTING;
    
    FreeVec(pm);
}


/*****************************************************************************

    NAME */
#include <proto/datatypes.h>


	AROS_LH4(ULONG, PrintDTObjectA,

/*  SYNOPSIS */
	AROS_LHA(Object           *, object   , A0),
	AROS_LHA(struct Window    *, window   , A1),
	AROS_LHA(struct Requester *, requester, A2),
	AROS_LHA(struct dtPrint   *, msg      , A3),

/*  LOCATION */
	struct Library *, DataTypesBase, 19, DataTypes)

/*  FUNCTION

    Perform an object's DTM_PRINT method in an asynchronous manner.

    INPUTS

    object     --  pointer to the data type object
    window     --  pointer to the window the object has been added to
    requester  --  pointer to the requester the object has been added to

    RESULT

    TRUE on success, FALSE otherwise.

    NOTES

    When an application has called PrintDTObjectA() it must not touch
    the printerIO union until a IDCMP_IDCMPUPDATE is received which
    contains the DTA_PrinterStatus tag.
        To abort a print, send the DTM_ABORTPRINT method to the object.
    This will signal the print process with a SIGBREAK_CTRL_C.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG                 retval = 0;
    struct DTSpecialInfo *dtsi = ((struct Gadget *)object)->SpecialInfo;
    struct PrintMessage  *pm;
   
    ObtainSemaphore(&(GPB(DataTypesBase)->dtb_Semaphores[SEM_ASYNC]));
   
    if(!(dtsi->si_Flags & DTSIF_PRINTING))
    {
	dtsi->si_Flags |= DTSIF_PRINTING;
      
	if((pm = AllocVec(sizeof(struct PrintMessage),
			 MEMF_PUBLIC | MEMF_CLEAR)))
	{
	    struct TagItem Tags[5];
	    struct Process *PrintProcess;
	    
	    pm->pm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	    pm->pm_Msg.mn_Length = sizeof(struct PrintMessage);
	    pm->pm_dtb = (struct DataTypesBase *)DataTypesBase;
	    pm->pm_object = object;
	    pm->pm_window = window;
	    pm->pm_requester = requester;
	    pm->pm_dtprint = *msg;
	 
	    Tags[0].ti_Tag  = NP_StackSize;
	    Tags[0].ti_Data = 4096;
	    Tags[1].ti_Tag  = NP_Entry;
	    Tags[1].ti_Data = (ULONG)&AsyncPrinter;
	    Tags[2].ti_Tag  = NP_Priority;
	    Tags[2].ti_Data = 0;
	    Tags[3].ti_Tag  = NP_Name;
	    Tags[3].ti_Data = (ULONG)"AsyncPrintDaemon";
	    Tags[4].ti_Tag  = TAG_DONE;
	 
	    if((PrintProcess = CreateNewProc(Tags)))
	    {
		PutMsg(&PrintProcess->pr_MsgPort, &pm->pm_Msg);
		
		setattrs(DataTypesBase, object, DTA_PrinterProc, PrintProcess,
			 TAG_DONE);
		
		retval = TRUE;
	    }
	    else
		FreeVec(pm);
	}
    }
    
    ReleaseSemaphore(&(GPB(DataTypesBase)->dtb_Semaphores[SEM_ASYNC]));
    
    return retval;

    AROS_LIBFUNC_EXIT
} /* PrintDTObjectA */
