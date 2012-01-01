/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Implementation of datatype rootclass
    Lang: English
*/

#define USE_BOOPSI_STUBS
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <intuition/cghooks.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <string.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/datatypes.h>
#include "datatypes_intern.h"

#include <clib/boopsistubs.h>

#include <aros/debug.h>

/* #include <devices/printer.h>  --  No printer stuff yet... */

/*****************************************************************************/

void DrawBox(struct Library *DataTypesBase, struct RastPort *rp,
	     LONG x1, LONG y1, LONG x2, LONG y2);


/****** datatypes.library/SetAttributes ***************************************
*
*   NAME
*       SetAttributes - set a DTObject's attributes given an opSet structure
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

IPTR SetAttributes(struct Library *DataTypesBase, Class *class, Object *object,
		    Msg msg)
{
    IPTR result = 0;
    struct DTObject *dto = INST_DATA(class, object);
    struct DTSpecialInfo *dtsi = ((struct Gadget *)object)->SpecialInfo;
   
    LONG TopVert  = dtsi->si_TopVert;
    LONG VisVert  = dtsi->si_VisVert;
    LONG TotVert  = dtsi->si_TotVert;
    LONG TopHoriz = dtsi->si_TopHoriz;
    LONG VisHoriz = dtsi->si_VisHoriz;
    LONG TotHoriz = dtsi->si_TotHoriz;
   
    struct TagItem *tstate = ((struct opSet *)msg)->ops_AttrList;
    struct TagItem *tag;
   
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
	SIPTR data = tag->ti_Data;
	
	switch(tag->ti_Tag)
	{
	case DTA_TopVert:       TopVert = data;            break;
	case DTA_VisibleVert:   VisVert = data;            break;
	case DTA_TotalVert:     TotVert = data;            break;
	case DTA_VertUnit:      dtsi->si_VertUnit = data;  break;
	    
	case DTA_TopHoriz:      TopHoriz = data;           break;
	case DTA_VisibleHoriz:  VisHoriz = data;           break;
	case DTA_TotalHoriz:    TotHoriz = data;           break;
	case DTA_HorizUnit:     dtsi->si_HorizUnit = data; break;
	    
	case DTA_PrinterProc:   dto->dto_PrinterProc = (struct Process *)data;
	                                                   break;
	case DTA_LayoutProc:    dto->dto_LayoutProc = (struct Process *)data;
	                                                   break;
	    
	case DTA_ObjName:
	    if(dto->dto_ObjName)
	    {
		FreeVec(dto->dto_ObjName); 
		dto->dto_ObjName = NULL;
	    }

	    if((APTR)data != NULL)
		if((dto->dto_ObjName = AllocVec((ULONG)strlen((UBYTE *)data) + 1,
						MEMF_PUBLIC | MEMF_CLEAR)))
		    strcpy(dto->dto_ObjName,(UBYTE *)data);

	    break;
	 
	case DTA_ObjAuthor:
	    if(dto->dto_ObjAuthor != NULL)
	    {
		FreeVec(dto->dto_ObjAuthor);
		dto->dto_ObjAuthor = NULL;
	    }

	    if((APTR)data != NULL)
		if((dto->dto_ObjAuthor = AllocVec((ULONG)strlen((UBYTE *)data)+1,
						  MEMF_PUBLIC | MEMF_CLEAR)))
		    strcpy(dto->dto_ObjAuthor, (UBYTE *)data);

	    break;
	 
	case DTA_ObjAnnotation:
	    if(dto->dto_ObjAnnotation != NULL)
	    {
		FreeVec(dto->dto_ObjAnnotation);
		dto->dto_ObjAnnotation = NULL;
	    }

	    if((APTR)data != NULL)
		if((dto->dto_ObjAnnotation = AllocVec((ULONG)strlen((UBYTE*)data) + 1,
						     MEMF_PUBLIC | MEMF_CLEAR)))
		    strcpy(dto->dto_ObjAnnotation, (UBYTE *)data);

	    break;
	 
	case DTA_ObjCopyright:
	    if(dto->dto_ObjCopyright != NULL)
	    {
		FreeVec(dto->dto_ObjCopyright);
		dto->dto_ObjCopyright=NULL;
	    }

	    if((APTR)data != NULL)
		if((dto->dto_ObjCopyright = AllocVec((ULONG)strlen((UBYTE*)data) + 1,
						     MEMF_PUBLIC | MEMF_CLEAR)))
		    strcpy(dto->dto_ObjCopyright, (UBYTE *)data);

	    break;
	    
	case DTA_ObjVersion:
	    if(dto->dto_ObjVersion != NULL)
	    {
		FreeVec(dto->dto_ObjVersion);
		dto->dto_ObjVersion = NULL;
	    }
	    
	    if((APTR)data != NULL)
		if((dto->dto_ObjVersion = AllocVec((ULONG)strlen((UBYTE*)data) + 1,
						  MEMF_PUBLIC | MEMF_CLEAR)))
		    strcpy(dto->dto_ObjVersion, (UBYTE *)data);

	    break;
	    
	case DTA_ObjectID:
	    dto->dto_ObjectID = data;
	    break;

	case DTA_UserData:
	    dto->dto_UserData = data;
	    break;
	 
	case DTA_SelectDomain:
	    dto->dto_SelectDomain = *((struct IBox *)data);
	    break;
	     
	case DTA_NominalVert:
	    dto->dto_NominalVert = data;
	    break;

	case DTA_NominalHoriz:
	    dto->dto_NominalHoriz = data;
	    break;
	}
    }
   
    if(TopVert < 0)
	TopVert = 0;

    if(VisVert > TotVert)
	TopVert = 0;
    else if(TopVert + VisVert > TotVert)
	TopVert = TotVert-VisVert;
   
    if(TopVert != dtsi->si_TopVert)
    {
	dtsi->si_TopVert = TopVert;
	result = TRUE;
    }

    if(VisVert != dtsi->si_VisVert)
    {
	dtsi->si_VisVert = VisVert;
	result = TRUE;
    }

    if(TotVert != dtsi->si_TotVert)
    {
	dtsi->si_TotVert = TotVert;
	result = TRUE;
    }
   
    if(TopHoriz < 0)
	TopHoriz = 0;

    if(VisHoriz > TotHoriz)
	TopHoriz = 0;
    else if(TopHoriz + VisHoriz > TotHoriz)
	TopHoriz = TotHoriz - VisHoriz;
   
    if(TopHoriz != dtsi->si_TopHoriz)
    {
	dtsi->si_TopHoriz = TopHoriz;
	result = TRUE;
    }

    if(VisHoriz != dtsi->si_VisHoriz)
    {
	dtsi->si_VisHoriz = VisHoriz;
	result = TRUE;
    }

    if(TotHoriz != dtsi->si_TotHoriz)
    {
	dtsi->si_TotHoriz = TotHoriz;
	result = TRUE;
    }
   
    return result;
}


/****** datatypes.library/Dispatcher ******************************************
*
*   NAME
*       Dispatcher - datatypesclass dispatcher code
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

AROS_UFH3(IPTR, Dispatcher,
	  AROS_UFHA(Class *, class, A0),
	  AROS_UFHA(Object *, object, A2),
	  AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    struct DataTypesBase *DataTypesBase = (struct DataTypesBase *)class->cl_UserData;
    struct DTObject *dto = INST_DATA(class,object);
    struct DTSpecialInfo *dtsi = ((struct Gadget *)object)->SpecialInfo;
    
    IPTR retval = 0;
    
    switch(msg->MethodID)
    {
    case OM_NEW:
	{
	    struct Gadget   *newobject;
	    struct DTObject *newdto;
	    
	    D(bug("datatypes.library/class/OM_NEW\n"));

	    if(!(newobject = (struct Gadget *)DoSuperMethodA(class, object,
							     msg)))
		SetIoErr(ERROR_NO_FREE_STORE);
	    else
	    {
		struct TagItem *attrs = ((struct opSet *)msg)->ops_AttrList;
		struct TagItem *nametag;
		BOOL Success = FALSE;
		APTR handle;

	        D(bug("datatypes.library/class/OM_NEW: DoSuperMethod succeeded\n"));
		
		newdto = INST_DATA(class, newobject);
		
		newobject->Flags |= GFLG_RELSPECIAL;
		newobject->SpecialInfo = &newdto->dto_DTSpecialInfo;
		
		InitSemaphore(&newdto->dto_DTSpecialInfo.si_Lock);
		
		newdto->dto_SourceType = GetTagData(DTA_SourceType, DTST_FILE,
						    attrs);
		handle = (APTR)GetTagData(DTA_Handle, (IPTR)NULL, attrs);
		
		if(!(nametag = FindTagItem(DTA_Name, attrs)))
		    SetIoErr(ERROR_REQUIRED_ARG_MISSING);
		else
		{
		    LONG namelen = 2;

	    	    D(bug("datatypes.library/class/OM_NEW: DTA_Name tag found\n"));
		    
		    if (newdto->dto_SourceType == DTST_FILE)
		    {
		        namelen = (ULONG)strlen((UBYTE *)nametag->ti_Data) + 1;
		    }
		    
		    if (!(newdto->dto_Name = AllocVec(namelen, MEMF_PUBLIC | MEMF_CLEAR)))
			SetIoErr(ERROR_NO_FREE_STORE);
		    else
		    {
	   		D(bug("datatypes.library/class/OM_NEW: Namelen allocation succeeded\n"));
			
			switch(newdto->dto_SourceType)
			{
			    case DTST_FILE:
			        strcpy(newdto->dto_Name, (UBYTE *)nametag->ti_Data);
				break;
			
			    case DTST_CLIPBOARD:
			    	newdto->dto_Name[0] = '0' + (UBYTE)nametag->ti_Data;
				break;				
			}
			    
			if(!(newdto->dto_DataType = (struct DataType *)GetTagData(DTA_DataType, (IPTR)NULL, attrs)))
			    Success = TRUE;
			else
			{
	   		    D(bug("datatypes.library/class/OM_NEW: DTA_DataType tag value okay\n"));
			    
			    switch(newdto->dto_SourceType)
			    {
			    case DTST_FILE:
			    	D(bug("datatypes.library/class/OM_NEW: SourceType = DTST_FILE\n"));

				switch(newdto->dto_DataType->dtn_Header->dth_Flags & DTF_TYPE_MASK)
				{
				case DTF_IFF:
				    if((newdto->dto_Handle = (APTR)AllocIFF()))
				    {
					if((((struct IFFHandle *)newdto->dto_Handle)->iff_Stream = (IPTR)NewOpen((struct Library *)DataTypesBase, newdto->dto_Name, DTST_FILE, 0)))
					{
					    InitIFFasDOS((struct IFFHandle*)newdto->dto_Handle);
					    
					    if(!OpenIFF((struct IFFHandle *)newdto->dto_Handle, IFFF_READ))
						Success = TRUE;
					}
				    }
				    
				    UnLock((BPTR)handle);
				    break;

				case DTF_MISC:
				    newdto->dto_Handle = handle;
				    Success = TRUE;
				    break;
				    
				default:
	   			    D(bug("datatypes.library/class/OM_NEW: calling NewOpen()\n"));

				    if((newdto->dto_Handle = (APTR)NewOpen((struct Library *)DataTypesBase, newdto->dto_Name, DTST_FILE, 0)))
					Success = TRUE;

	    			    D(bug("datatypes.library/class/OM_NEW: NewOpened() returned %s\n", Success ? "success" : "failure"));
				    
				    UnLock((BPTR)handle);
				    break;
				} /*  switch(... & DTF_TYPE_MASK) */
				break;
				
			    case DTST_CLIPBOARD:
			    	D(bug("datatypes.library/class/OM_NEW: SourceType = DTST_CLIPBOARD\n"));

				newdto->dto_Handle = handle;
				Success = TRUE;
				break;
			    } /* switch(sourcetype) */
			}
		    }
		}
		
		if(Success)
		{
		    SetAttributes((struct Library *)DataTypesBase, class,
				  (Object *)newobject, msg);
		    retval = (IPTR)newobject;
		}
		else
		    CoerceMethod(class, (Object *)newobject, OM_DISPOSE);
	    }

	    D(bug("datatypes.library/class/OM_NEW: returning %x handle = %x\n", retval, newdto->dto_Handle));

	    break;
	} /* case OM_NEW: */

    case OM_UPDATE:
	/* Avoid update loops */
/*	if(DoMethod(object, ICM_CHECKLOOP))
	    break; */

	/* Fall through */
    case OM_SET:
	/* Let superclass see the new attributes... */
	retval  = DoSuperMethodA(class, object, msg);
	/* ...and set our own new attributes. */
	retval += SetAttributes((struct Library *)DataTypesBase, class, object, msg);
	break;
	
    case OM_GET:
	D(bug("datatypes.library/class/OM_GET: tag = %x\n", ((struct opGet*)msg)->opg_AttrID));
	{
	    IPTR *store = ((struct opGet *)msg)->opg_Storage;
	    retval = 1;
	    
	    switch(((struct opGet*)msg)->opg_AttrID)
	    {
	    case DTA_TopVert:       *store = dtsi->si_TopVert;   break;
	    case DTA_VisibleVert:   *store = dtsi->si_VisVert;   break;
	    case DTA_TotalVert:     *store = dtsi->si_TotVert;   break;
	    case DTA_VertUnit:      *store = dtsi->si_VertUnit;  break;
		
	    case DTA_TopHoriz:      *store = dtsi->si_TopHoriz;  break;
	    case DTA_VisibleHoriz:  *store = dtsi->si_VisHoriz;  break;
	    case DTA_TotalHoriz:    *store = dtsi->si_TotHoriz;  break;
	    case DTA_HorizUnit:     *store = dtsi->si_HorizUnit; break;
		
	    case DTA_PrinterProc:   *store = (IPTR)dto->dto_PrinterProc; break;
	    case DTA_LayoutProc:    *store = (IPTR)dto->dto_LayoutProc;  break;
		
	    case DTA_Name:          *store = (IPTR)dto->dto_Name;     break;
	    case DTA_SourceType:    *store = dto->dto_SourceType;      break;
	    case DTA_Handle:	    *store = (IPTR)dto->dto_Handle;   break;
	    case DTA_DataType:      *store = (IPTR)dto->dto_DataType; break;
	    case DTA_Domain:        *store = (IPTR)&dto->dto_Domain;  break;
		
	    case DTA_ObjName:       *store = (IPTR)dto->dto_ObjName;       break;
	    case DTA_ObjAuthor:     *store = (IPTR)dto->dto_ObjAuthor;     break;
	    case DTA_ObjAnnotation: *store = (IPTR)dto->dto_ObjAnnotation; break;
	    case DTA_ObjCopyright:  *store = (IPTR)dto->dto_ObjCopyright;  break;
	    case DTA_ObjVersion:    *store = (IPTR)dto->dto_ObjVersion;    break;
	    case DTA_ObjectID:      *store = dto->dto_ObjectID;             break;
	    case DTA_UserData:      *store = dto->dto_UserData;             break;
	    case DTA_FrameInfo:     *store = (IPTR)&dto->dto_FrameInfo;    break;
		
	    case DTA_SelectDomain:
		if (dtsi->si_Flags & DTSIF_HIGHLIGHT)
		    *store = (IPTR)&dto->dto_SelectDomain;
		else
		    *store = (IPTR)NULL;
		break;
		
	    case DTA_TotalPVert:    *store = dto->dto_TotalPVert;   break;
	    case DTA_TotalPHoriz:   *store = dto->dto_TotalPHoriz;  break;
	    case DTA_NominalVert:   *store = dto->dto_NominalVert;  break;
	    case DTA_NominalHoriz:  *store = dto->dto_NominalHoriz; break;
	    case DTA_Data:          *store = (IPTR)object;          break;

	    default:
		retval = DoSuperMethodA(class, object, msg);
		break;
	    } /* switch(AttrId) */
	} /* case OM_GET: */
	break;
	
    case GM_HITTEST:
	/* A datatypes gadget is hit everywhere */
	retval = GMR_GADGETHIT;
	break;
	
    case GM_GOACTIVE:
	/* Calculate printable dimensions */
	dto->dto_TotalPHoriz = (dtsi->si_HorizUnit) ?
	    dtsi->si_HorizUnit*dtsi->si_TotHoriz : dto->dto_Domain.Width;
	
	dto->dto_TotalPVert  = (dtsi->si_VertUnit ) ?
	    dtsi->si_VertUnit*dtsi->si_TotVert : dto->dto_Domain.Height;
	
	/* Fall through */
	
    case GM_HANDLEINPUT:
	{
	    struct InputEvent *ievent = ((struct gpInput *)msg)->gpi_IEvent;
	    
	    if((msg->MethodID == GM_GOACTIVE && (!ievent) ) ||
	       (dtsi->si_Flags & DTSIF_LAYOUT))
		retval=GMR_NOREUSE;
	    else
	    {
		if(!AttemptSemaphoreShared(&dtsi->si_Lock))
		    retval = GMR_NOREUSE;
		else
		{
		    struct RastPort *rp;
		    struct IBox *domain;
		    ULONG hunit;
		    ULONG vunit;
		    
		    if((rp = ObtainGIRPort(((struct gpInput *)msg)->gpi_GInfo)))
		    {
			SetDrMd(rp, COMPLEMENT);
			SetAPen(rp, -1);
			
			GetAttr(DTA_Domain, object, (IPTR *)&domain);
			
			hunit = (dtsi->si_HorizUnit) ? (dtsi->si_HorizUnit) : 1;
			vunit = (dtsi->si_VertUnit ) ? (dtsi->si_VertUnit ) : 1;
			
			switch(ievent->ie_Class)
			{
			case IECLASS_RAWMOUSE:
			    {
				switch(ievent->ie_Code)
				{
				case IECODE_LBUTTON:
				    {
					((struct Gadget *)object)->Flags |= GFLG_SELECTED;
					
					dto->dto_OldTopHoriz = dtsi->si_TopHoriz;
					dto->dto_OldTopVert  = dtsi->si_TopVert;
					
					if(dtsi->si_Flags & DTSIF_DRAGSELECT)
					{
					    struct IBox *sdomain;
					    
					    if(GetAttr(DTA_SelectDomain, object, (IPTR *)&sdomain))
					    {
						if(sdomain)
						{
						    dtsi->si_Flags &= ~DTSIF_HIGHLIGHT;
						    
						    DoMethod(object, DTM_CLEARSELECTED, 
							     (IPTR)((struct gpInput *)msg)->gpi_GInfo);
						}
					    }
					    
					    dtsi->si_Flags |= (DTSIF_DRAGGING | DTSIF_DRAGSELECT);
					    
					    dto->dto_MouseX = ((struct gpInput *)msg)->gpi_Mouse.X;
					    dto->dto_MouseY = ((struct gpInput *)msg)->gpi_Mouse.Y;
					    
					    if(dto->dto_MouseX >= dto->dto_TotalPHoriz)
						dto->dto_MouseX = dto->dto_TotalPHoriz - 1;

					    if(hunit>1)
						dto->dto_MouseX = dto->dto_MouseX / hunit * hunit;
					    
					    dto->dto_SelectRect.MinX = dto->dto_MouseX + dtsi->si_TopHoriz*hunit;
					    dto->dto_StartX = dto->dto_MouseX = dto->dto_MouseX + ((struct Gadget *)object)->LeftEdge;
					    
					    if(dto->dto_MouseY >= dto->dto_TotalPVert)
						dto->dto_MouseY = dto->dto_TotalPVert - 1;
					    
					    if(vunit > 1)
						dto->dto_MouseY = dto->dto_MouseY/vunit*vunit;
					    dto->dto_SelectRect.MinY = dto->dto_MouseY + dtsi->si_TopVert*vunit;
					    dto->dto_StartY = dto->dto_MouseY = dto->dto_MouseY + ((struct Gadget *)object)->TopEdge;
					    
					    dto->dto_LinePtrn = rp->LinePtrn = 0xff00;
					    DrawBox((struct Library *)DataTypesBase, rp, (LONG)dto->dto_StartX, (LONG)dto->dto_StartY, (LONG)dto->dto_MouseX, (LONG)dto->dto_MouseY);
					} /* if(DRAGSELECT) */
				    } /* if(IECODE_LBUTTON) */
				    break;
				    
				case (IECODE_LBUTTON | IECODE_UP_PREFIX):
				    retval = GMR_VERIFY | GMR_NOREUSE;
				    break;

				case IECODE_RBUTTON:
				    {
					((struct Gadget *)object)->Flags &= ~GFLG_SELECTED;
					
					dtsi->si_TopHoriz = dto->dto_OldTopHoriz;
					dtsi->si_TopVert  = dto->dto_OldTopVert;
					
					retval = GMR_NOREUSE;
				    }
				    break;
				    
				default:
				    if(dtsi->si_Flags & DTSIF_DRAGSELECT)
				    {
					rp->LinePtrn = dto->dto_LinePtrn;
					DrawBox((struct Library *)DataTypesBase, rp, (LONG)dto->dto_StartX, (LONG)dto->dto_StartY, (LONG)dto->dto_MouseX, (LONG)dto->dto_MouseY);
					
					dto->dto_MouseX=((struct gpInput *)msg)->gpi_Mouse.X;
					dto->dto_MouseY=((struct gpInput *)msg)->gpi_Mouse.Y;
					
					if(dto->dto_MouseX < 0)
					    dto->dto_MouseX = 0;
					
					if(dto->dto_MouseX >= domain->Width)
					    dto->dto_MouseX = domain->Width - 1;
					
					if(dto->dto_MouseX >= dto->dto_TotalPHoriz)
					    dto->dto_MouseX = dto->dto_TotalPHoriz - 1;
					
					if(hunit > 1)
					    dto->dto_MouseX = dto->dto_MouseX / hunit * hunit - 1;
					
					dto->dto_SelectRect.MaxX = dto->dto_MouseX + dtsi->si_TopHoriz * hunit;
					dto->dto_MouseX = dto->dto_MouseX + ((struct Gadget*)object)->LeftEdge;
					
					if(dto->dto_MouseY < 0)
					    dto->dto_MouseY = 0;
					
					if(dto->dto_MouseY >= domain->Height)
					    dto->dto_MouseY = domain->Height - 1;
					
					if(dto->dto_MouseY >= dto->dto_TotalPVert)
					    dto->dto_MouseY = dto->dto_TotalPVert - 1;
					
					if(vunit > 1)
					    dto->dto_MouseY = dto->dto_MouseY / vunit * vunit - 1;
					
					dto->dto_SelectRect.MaxY = dto->dto_MouseY + dtsi->si_TopVert * vunit;
					dto->dto_MouseY = dto->dto_MouseY + ((struct Gadget*)object)->TopEdge;
					
					DrawBox((struct Library *)DataTypesBase, rp, (LONG)dto->dto_StartX, (LONG)dto->dto_StartY, (LONG)dto->dto_MouseX, (LONG)dto->dto_MouseY);
				    } /* if(DRAGSELECT) */
				    break;
				} /* switch(ievent->ie_Code) */
			    } /* case IECLASS_RAWMOUSE */
			    break;
			    
			case IECLASS_TIMER:
			    {
				LONG NewTopHoriz, NewTopVert;
				LONG OldTopHoriz, OldTopVert;
				WORD MouseX, MouseY;
				
				NewTopHoriz = OldTopHoriz = dtsi->si_TopHoriz;
				NewTopVert  = OldTopVert  = dtsi->si_TopVert;
				
				MouseX = ((struct gpInput *)msg)->gpi_Mouse.X;
				MouseY = ((struct gpInput *)msg)->gpi_Mouse.Y;
				
				if(MouseX < 0)
				    NewTopHoriz += MouseX / hunit;
				else if(MouseX >= domain->Width )
				    NewTopHoriz += (MouseX - domain->Width - 1) / hunit;
				
				if(MouseY < 0)
				    NewTopVert  += MouseY / vunit;
				else if(MouseY >= domain->Height)
				    NewTopVert  += (MouseY - domain->Height - 1) / vunit;
				
				if(NewTopHoriz > (dtsi->si_TotHoriz-dtsi->si_VisHoriz))
				    NewTopHoriz = dtsi->si_TotHoriz-dtsi->si_VisHoriz;
				
				if(NewTopHoriz < 0)
				    NewTopHoriz = 0;
				
				if(NewTopVert  > (dtsi->si_TotVert - dtsi->si_VisVert ))
				    NewTopVert  = dtsi->si_TotVert - dtsi->si_VisVert;
				
				if(NewTopVert  < 0)
				    NewTopVert  = 0;
				
				if((NewTopVert != OldTopVert) || (NewTopHoriz != OldTopHoriz))
				{
				    dto->dto_Flags |= DTOFLGF_HAS_MOVED;
				    
				    if (dtsi->si_Flags & DTSIF_DRAGSELECT)
				    {
					rp->LinePtrn = dto->dto_LinePtrn;
					DrawBox((struct Library *)DataTypesBase, rp, (LONG)dto->dto_StartX, (LONG)dto->dto_StartY, (LONG)dto->dto_MouseX, (LONG)dto->dto_MouseY);
				    }
				    
				    dtsi->si_TopHoriz = NewTopHoriz;
				    dtsi->si_TopVert  = NewTopVert;
				    
				    DoMethod(object, GM_RENDER, (IPTR)((struct gpInput *)msg)->gpi_GInfo, (IPTR)rp, GREDRAW_UPDATE);
				    
				    if (dtsi->si_Flags & DTSIF_DRAGSELECT)
				    {
					dto->dto_StartX = domain->Left + dto->dto_SelectRect.MinX - dtsi->si_TopHoriz;
					
					if(dto->dto_StartX < domain->Left)
					    dto->dto_StartX = domain->Left;
					else if(dto->dto_StartX >= (domain->Left + domain->Width))
					    dto->dto_StartX = domain->Left + domain->Width - 1;
					
					dto->dto_StartY = domain->Top  + dto->dto_SelectRect.MinY - dtsi->si_TopVert;
					
					if(dto->dto_StartY < domain->Top )
					    dto->dto_StartY = domain->Top;
					else if(dto->dto_StartY >= (domain->Top  + domain->Height))
					    dto->dto_StartY = domain->Top  + domain->Height - 1;
					
					DrawBox((struct Library *)DataTypesBase, rp, (LONG)dto->dto_StartX, (LONG)dto->dto_StartY, (LONG)dto->dto_MouseX, (LONG)dto->dto_MouseY);
				    } /* if(DRAGSELECT) */
				} /* if(vert or horiz have changed) */
				
				if(dtsi->si_Flags & DTSIF_DRAGSELECT)
				{
				    UWORD pattern = (dto->dto_LinePtrn << 4) | ((dto->dto_LinePtrn & 0xf000) >> 12);
				    rp->LinePtrn = dto->dto_LinePtrn ^ pattern;
				    DrawBox((struct Library *)DataTypesBase, rp, (LONG)dto->dto_StartX, (LONG)dto->dto_StartY, (LONG)dto->dto_MouseX, (LONG)dto->dto_MouseY);
				    dto->dto_LinePtrn = pattern;
				}
			    } /* case IECLASS_TIMER: */
			    break;
			} /* switch(ievent->ie_Class) */
			
			ReleaseGIRPort(rp);
		    } /* if(we got GIRPort) */

		    ReleaseSemaphore(&dtsi->si_Lock);
		}
	    }
	} /* case GM_HANDLEINPUT */
	break;
	
    case GM_GOINACTIVE:
	{
	    struct RastPort *rp;
	    
	    if((rp = ObtainGIRPort(((struct gpInput *)msg)->gpi_GInfo)))
	    {
		if(dtsi->si_Flags & DTSIF_DRAGSELECT)
		{
		    SetDrMd(rp, COMPLEMENT);
		    SetAPen(rp, -1);
		    
		    rp->LinePtrn = dto->dto_LinePtrn;
		    DrawBox((struct Library *)DataTypesBase, rp, 
			    (LONG)dto->dto_StartX, (LONG)dto->dto_StartY,
			    (LONG)dto->dto_MouseX, (LONG)dto->dto_MouseY);
		    
		    if(((struct Gadget *)object)->Flags & GFLG_SELECTED)
		    {
			struct dtSelect dts;
			WORD temp;
			
			if (dto->dto_SelectRect.MinX > dto->dto_SelectRect.MaxX)
			{
			    temp = dto->dto_SelectRect.MinX;
			    dto->dto_SelectRect.MinX = dto->dto_SelectRect.MaxX;
			    dto->dto_SelectRect.MaxX = temp;
			}
			
			if (dto->dto_SelectRect.MinY > dto->dto_SelectRect.MaxY)
			{
			    temp = dto->dto_SelectRect.MinY;
			    dto->dto_SelectRect.MinY = dto->dto_SelectRect.MaxY;
			    dto->dto_SelectRect.MaxY = temp;
			}
			
			dto->dto_SelectDomain.Left   = dto->dto_SelectRect.MinX;
			dto->dto_SelectDomain.Top    = dto->dto_SelectRect.MinY;
			dto->dto_SelectDomain.Width  = dto->dto_SelectRect.MaxX - dto->dto_SelectRect.MinX + 1;
			dto->dto_SelectDomain.Height = dto->dto_SelectRect.MaxY - dto->dto_SelectRect.MinY + 1;
			
			dtsi->si_Flags |= DTSIF_HIGHLIGHT;
			
			dts.MethodID = DTM_SELECT;
			dts.dts_GInfo = ((struct gpInput *)msg)->gpi_GInfo;
			dts.dts_Select = dto->dto_SelectRect;
			DoMethodA(object, (Msg)&dts);
		    }
		    
		    Do_OM_NOTIFY((struct Library *)DataTypesBase, object,
				 ((struct gpInput *)msg)->gpi_GInfo, 0, GA_ID,
				 (ULONG)((struct Gadget *)object)->GadgetID,
				 DTA_Busy, FALSE, TAG_DONE );
		} /* if(DRAGSELECT) */
		
		if (dto->dto_Flags & DTOFLGF_HAS_MOVED)
		{
		    Do_OM_NOTIFY((struct Library *)DataTypesBase, object,
				 ((struct gpInput *)msg)->gpi_GInfo, 0, GA_ID,
				 (ULONG)((struct Gadget *)object)->GadgetID,
				 DTA_Sync, TRUE,
				 DTA_TopVert, dtsi->si_TopVert,
				 DTA_TopHoriz, dtsi->si_TopHoriz,
				 TAG_DONE );
		    
		    dto->dto_Flags &= ~DTOFLGF_HAS_MOVED;
		}
		
		ReleaseGIRPort(rp);
	    }

	    dtsi->si_Flags &= ~(DTSIF_DRAGGING | DTSIF_DRAGSELECT);
	    ((struct Gadget *)object)->Flags &= ~(GFLG_SELECTED | GFLG_RELSPECIAL);
	} /* case GM_GOINACTIVE */
	break;
	
    case GM_DOMAIN:
	if(object == NULL)
	    break;

	if(dtsi != NULL)
	{
	    ; /* TODO */
	}

	break;

    /* GM_LAYOUT and DTM_PROCLAYOUT have the same semantics; DTM_PROCLAYOUT
       must be in the process context, GM_LAYOUT must not. */
    case GM_LAYOUT:
    case DTM_PROCLAYOUT:
	{
	    struct GadgetInfo *ginfo;

	    dto->dto_Domain= *((struct IBox *)(&((struct Gadget*)object)->LeftEdge));
	    
	    if((ginfo = ((struct gpLayout *)msg)->gpl_GInfo))
	    {
		struct Window *window;
		
		if((window = ginfo->gi_Window))
		{
		    if(dto->dto_Domain.Left < 0)
			dto->dto_Domain.Left += window->Width - 1;
		    
		    if(dto->dto_Domain.Top < 0)
			dto->dto_Domain.Top += window->Height - 1;
		    
		    if(dto->dto_Domain.Width < 0)
			dto->dto_Domain.Width += window->Width;
		    
		    if(dto->dto_Domain.Height < 0)
			dto->dto_Domain.Height += window->Height;
		}
	    }
	    
	    /* Calculate printable dimensions */
	    dto->dto_TotalPHoriz = (dtsi->si_HorizUnit) ?
		dtsi->si_HorizUnit*dtsi->si_TotHoriz : dto->dto_Domain.Width;
	    dto->dto_TotalPVert  = (dtsi->si_VertUnit ) ?
		dtsi->si_VertUnit*dtsi->si_TotVert : dto->dto_Domain.Height;
	 
	    retval = TRUE;
	    
	}  /* Fall through */ /* case DTM_PROCLAYOUT: */
	
    case DTM_FRAMEBOX:
	if(dto->dto_SourceType == DTST_CLIPBOARD)
	{
	    if(dto->dto_Handle != NULL)
	    {
		CloseIFF((struct IFFHandle *)dto->dto_Handle);
		CloseClipboard((struct ClipboardHandle *)((struct IFFHandle *)dto->dto_Handle)->iff_Stream);
		FreeIFF((struct IFFHandle *)dto->dto_Handle);
		dto->dto_Handle = NULL;
	    }
	}
	
	break;
	
    case DTM_ABORTPRINT:
	Forbid();

	if(dto->dto_PrinterProc != NULL)
	    Signal((struct Task *)dto->dto_PrinterProc, SIGBREAKF_CTRL_C);
	
	Permit();
	
	break;

    case DTM_PRINT:
	/*	retval = PDERR_CANCEL;  --  No printer stuff yet... */
	SetIoErr(ERROR_NOT_IMPLEMENTED);
	break;
	
    case DTM_REMOVEDTOBJECT:	/* TODO... */
	break;

    case DTM_WRITE:
	retval = 0;
	SetIoErr(ERROR_NOT_IMPLEMENTED);
	break;

    case OM_DISPOSE:
	retval = IoErr();
	
	if(dto->dto_DataType != NULL)
	{
	    APTR handle;
	    
	    if((handle = dto->dto_Handle))
	    {
		switch(dto->dto_SourceType)
		{
		case DTST_FILE:
		    switch(dto->dto_DataType->dtn_Header->dth_Flags & DTF_TYPE_MASK)
		    {
		    case DTF_IFF:
			if(((struct IFFHandle*)handle)->iff_Stream != (IPTR)NULL)
			{
			    CloseIFF((struct IFFHandle *)handle);
			    Close((BPTR)((struct IFFHandle *)handle)->iff_Stream);
			}
			
			FreeIFF((struct IFFHandle *)handle);
			
			break;
			
		    case DTF_MISC:
			UnLock((BPTR)handle);
			break;
			
		    default:
			Close((BPTR)handle);
			break;
		    }
		    break;
		    
		case DTST_CLIPBOARD:
		    if(dto->dto_Handle != NULL)
		    {
			CloseIFF((struct IFFHandle *)handle);
			CloseClipboard((struct ClipboardHandle *)((struct IFFHandle *)handle)->iff_Stream);
			FreeIFF((struct IFFHandle *)handle);
		    }
		    
		    break;
		} /* switch(sourcetype) */
	    } /* if(got handle) */
	} /* if(datatype != NULL) */
	
	if(dto->dto_Name)
	    FreeVec(dto->dto_Name);
	
	if(dto->dto_ObjName)
	    FreeVec(dto->dto_ObjName);
	
	if(dto->dto_ObjAuthor)
	    FreeVec(dto->dto_ObjAuthor);
	
	if(dto->dto_ObjAnnotation)
	    FreeVec(dto->dto_ObjAnnotation);
	
	if(dto->dto_ObjCopyright)
	    FreeVec(dto->dto_ObjCopyright);
	
	if(dto->dto_ObjVersion)
	    FreeVec(dto->dto_ObjVersion);
	
	/* Redirect error to the DataTypes error codes */
	if(retval == ERROR_OBJECT_NOT_FOUND)
	    retval = DTERROR_COULDNT_OPEN;
	
	SetIoErr(retval);
	
        /* Fall through so the superclass can free its resources */
    default:
	retval = DoSuperMethodA(class, object, msg);
	break;
    }

    return retval;

    AROS_USERFUNC_EXIT
}


/****** datatypes.library/DrawBox *********************************************
*
*   NAME
*       DrawBox - draw a box for area marking
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RETURNS
*
*   EXAMPLE
*
*   SEE ALSO
*
******************************************************************************
*
*/

void DrawBox(struct Library *DataTypesBase, struct RastPort *rp,
	     LONG x1, LONG y1, LONG x2, LONG y2)
{
    rp->Flags |= FRST_DOT;
    rp->linpatcnt = 15;
    
    SetWriteMask(rp, 0x03);
    
    if(x1 != x2)
    {
	Move(rp,x1,y1);
	Draw(rp,x2,y1);
	
	Move(rp,x2,y2);
	Draw(rp,x1,y2);
    }
    
    if(y1 != y2)
    {
	Move(rp,x2,y1);
	Draw(rp,x2,y2);
	
	Move(rp,x1,y2);
	Draw(rp,x1,y1);
    }
    
    SetWriteMask(rp, 0xff);
}
