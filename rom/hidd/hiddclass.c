/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Main class for HIDD.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <intuition/classusr.h>
#include <intuition/classes.h>
#include <hidd/hidd.h>

#include <proto/exec.h>
#include <proto/intuition.h>	/* for DoSuperMethodA() */
#include <proto/boopsi.h>
#include <proto/utility.h>

#ifdef _AROS
#include <aros/asmcall.h>
#endif /* _AROS */

static const UBYTE name[];
static const UBYTE version[];
static ULONG AROS_SLIB_ENTRY(init,HIDD)();
extern const char HIDD_End;

int entry(void)
{
    return -1;
}

const struct Resident HIDD_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&HIDD_resident,
    (APTR)&HIDD_End,
    RTF_COLDSTART,
    41,
    NT_UNKNOWN,
    90,	/* Has to be after BOOPSI */
    (UBYTE *)name,
    (UBYTE *)version,
    (APTR)&AROS_SLIB_ENTRY(init,HIDD)
};

static const UBYTE name[] = "hiddclass";
static const UBYTE version[] = "hiddclass 41.1 (23.10.1997)\r\n";

static const char unknown[] = "--unknown device--";

/************************************************************************/

struct HIDDData
{
    UWORD	hd_Type;
    UWORD	hd_SubType;
    ULONG	hd_Producer;
    STRPTR	hd_Name;
    STRPTR	hd_HWName;
    BOOL	hd_Active;
    UWORD	hd_Locking;
    ULONG	hd_Status;
    ULONG	hd_ErrorCode;
};

/* Static Data for the hiddclass. */
struct HCD
{
    struct Library 		*UtilityBase;
    struct Library 		*BOOPSIBase;
    struct MinList 		 hiddList;
    struct SignalSemaphore	 listLock;
};

#define BOOPSIBase	(((struct HCD *)cl->cl_UserData)->BOOPSIBase)
#define UtilityBase	(((struct HCD *)cl->cl_UserData)->UtilityBase)

/* This is the dispatcher for the root HIDD class. */

AROS_UFH3(static IPTR, dispatch_hiddclass,
    AROS_UFHA(Class *, 	cl, 	A0),
    AROS_UFHA(Object *, o,  	A2),
    AROS_UFHA(Msg,	msg,	A1)
)
{
    IPTR retval = 0UL;
    struct HIDDData *hd = NULL;

    /* Don't try and get instance data if we don't have an object. */
    if(	   msg->MethodID != OM_NEW
	&& msg->MethodID != HIDDM_Class_Get 
	&& msg->MethodID != HIDDM_Class_MGet
      )
	hd = INST_DATA(cl, o);

    /* We now dispatch the actual methods */
    switch(msg->MethodID)
    {
    case OM_NEW:
	retval = DoSuperMethodA(cl, o, msg);
	if(!retval)
	    break;

	hd = INST_DATA(cl, retval);
	
	/*  Initialise the HIDD class. These fields are publicly described
	    as not being settable at Init time, however it is the only way to
	    get proper abstraction if you ask me. Plus it does reuse code
	    in a nice way.
	
	    To pass these into the init code I would recommend that your
	    pass in a TagList of your tags, which is linked to the user's
	    tags by a TAG_MORE. This way you will prevent them from setting
	    these values.
	*/
	if( hd != NULL)
	{
	    struct TagItem *list = ((struct opSet *)msg)->ops_AttrList;
	    hd->hd_Type = GetTagData(HIDDA_Type, 0, list);
	    hd->hd_SubType = GetTagData(HIDDA_SubType, 0, list);
	    hd->hd_Producer = GetTagData(HIDDA_Producer, 0, list);
	    hd->hd_Name = (STRPTR)GetTagData(HIDDA_Name, (IPTR)unknown, list);
	    hd->hd_HWName = (STRPTR)GetTagData(HIDDA_HardwareName, (IPTR)unknown, list);
	    hd->hd_Active = TRUE; /* Set default, GetTagData() comes later */
	    hd->hd_Status = GetTagData(HIDDA_Status, HIDDV_StatusUnknown, list);
	    hd->hd_ErrorCode = GetTagData(HIDDA_ErrorCode, 0, list);
	    hd->hd_Locking = GetTagData(HIDDA_Locking, HIDDV_LockShared, list);
	}
	
	/* Fall through */
    case OM_SET:
    {
	struct TagItem *tstate = ((struct opSet *)msg)->ops_AttrList;
	struct TagItem *tag;

	while((tag = NextTagItem(&tstate)))
	{
	    switch(tag->ti_Tag)
	    {
	    case HIDDA_Active:
		hd->hd_Active = tag->ti_Data;
		break;
	    }
	}
	break;
    }

    case OM_NOTIFY:
	break;

    case OM_GET:
    {
	switch(((struct opGet *)msg)->opg_AttrID)
	{
	case HIDDA_Type:
	    *((struct opGet *)msg)->opg_Storage = hd->hd_Type;
	    break;

	case HIDDA_SubType:
	    *((struct opGet *)msg)->opg_Storage = hd->hd_SubType;
	    break;

	case HIDDA_Producer:
	    *((struct opGet *)msg)->opg_Storage = hd->hd_Producer;
	    break;

	case HIDDA_Name:
	    *((struct opGet *)msg)->opg_Storage = (IPTR)hd->hd_Name;
	    break;

	case HIDDA_HardwareName:
	    *((struct opGet *)msg)->opg_Storage = (IPTR)hd->hd_HWName;
	    break;

	case HIDDA_Active:
	    *((struct opGet *)msg)->opg_Storage = hd->hd_Active;
	    break;

	case HIDDA_Status:
	    *((struct opGet *)msg)->opg_Storage = hd->hd_Status;
	    break;

	case HIDDA_ErrorCode:
	    *((struct opGet *)msg)->opg_Storage = hd->hd_ErrorCode;
	    break;
    
	case HIDDA_Locking:
	    *((struct opGet *)msg)->opg_Storage = hd->hd_Locking;
	    break;
	} /* switch(msg->opg_AttrID) */
    } /* OM_GET */

    /* These are the "hiddclass" methods. */

    /*  These two are invalid, since we don't have anything to get 
	from a class, so the superclass should handle these.

	This is especially the case since the only place that we can
	get the information for these methods is from an object, but
	we don't have any objects if this method is called.
    */
    case HIDDM_Class_Get:
    case HIDDM_Class_MGet:
	retval = 0;
	break;

    /* 	Yet to determine the semantics of these so we just let
	them return 0 for now.
    */
    case HIDDM_BeginIO:
    case HIDDM_AbortIO:
	retval = 0;
	break;

    /* Return NULL for failure. */
    case HIDDM_LoadConfigPlugin:
    case HIDDM_Lock:
    case HIDDM_Unlock:			    
	retval = NULL;
	break;

    case HIDDM_AddHIDD:
    {
	/* We add a class to the list of HIDD's */
	struct IClass *hc = ((hmAdd *)msg)->hma_Class;

	if( (hc->cl_Flags & CLF_INLIST) == 0 )
	{
	    /* A class structure is really a MinNode */
	    ObtainSemaphore(&((struct HCD *)cl->cl_UserData)->listLock);
	    AddTail(
		(struct List *)&((struct HCD *)cl->cl_UserData)->hiddList, 
		(struct Node *)hc
	    );
	    ReleaseSemaphore(&((struct HCD *)cl->cl_UserData)->listLock);

	    hc->cl_Flags |= CLF_INLIST;
	    retval = TRUE;
	}
	break;
    }

    case HIDDM_RemoveHIDD:
    {
	struct IClass *hc = ((hmAdd *)msg)->hma_Class;

	if( hc->cl_Flags & CLF_INLIST )
	{
	    ObtainSemaphore(&((struct HCD *)cl->cl_UserData)->listLock);
	    Remove((struct Node *)hc);
	    ReleaseSemaphore(&((struct HCD *)cl->cl_UserData)->listLock);
	    hc->cl_Flags &= ~CLF_INLIST;
	}
    }

    case OM_DISPOSE:
	/* We don't actually have anything to free - fall through. */
    
    default:
	/* No idea, send it to the superclass */
	retval = DoSuperMethodA(cl, o, msg);
    } /* switch(msg->MethodID) */

    return retval;
}

/* This is the initialisation code for the HIDD class itself. */
#undef BOOPSIBase
#undef UtilityBase

AROS_UFH3(static ULONG, AROS_SLIB_ENTRY(init, HIDD),
    AROS_UFHA(ULONG, dummy1, D0),
    AROS_UFHA(ULONG, dummy2, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    struct Library *BOOPSIBase;
    struct IClass *cl;
    struct HCD *hcd;

    /*
	We map the memory into the shared memory space, because it is
	to be accessed by many processes, eg searching for a HIDD etc.

	Well, maybe once we've got MP this might help...:-)
    */
    hcd = AllocMem(sizeof(struct HCD), MEMF_CLEAR|MEMF_PUBLIC);
    if(hcd == NULL)
    {
	/* If you are not running from ROM, don't use Alert() */
	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
	return NULL;
    }

    NEWLIST(&hcd->hiddList);
    InitSemaphore(&hcd->listLock);

    BOOPSIBase = hcd->BOOPSIBase = OpenLibrary("boopsi.library", 0);
    if(hcd->BOOPSIBase == NULL)
    {
	FreeMem(hcd, sizeof(struct HCD));
	Alert(AT_DeadEnd | AG_OpenLib | AN_Unknown | AO_Unknown);
	return NULL;
    }

    hcd->UtilityBase = OpenLibrary("utility.library",0);
    if(hcd->UtilityBase == NULL)
    {
	CloseLibrary(hcd->UtilityBase);
    	FreeMem(hcd, sizeof(struct HCD));
	Alert(AT_DeadEnd | AG_OpenLib | AN_Unknown | AO_UtilityLib);
	return NULL;
    }

    /* Create the class structure for the "hiddclass" */
    if((cl = MakeClass(HIDDCLASS, ROOTCLASS, NULL, sizeof(struct HIDDData), 0)))
    {
	cl->cl_Dispatcher.h_Entry = (APTR)dispatch_hiddclass;
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_Dispatcher.h_Data = cl;
	cl->cl_UserData = (IPTR)hcd;

	AddClass(cl);
    }
    return NULL;
}	
