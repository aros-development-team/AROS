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
#include <oop/oop.h>
#include <oop/root.h>
#include <oop/meta.h>
#include <oop/hiddmeta.h>
#include <hidd/hidd.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#ifdef _AROS
#include <aros/asmcall.h>
#endif /* _AROS */

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

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
    90, /* Has to be after OOP */
    (UBYTE *)name,
    (UBYTE *)version,
    (APTR)&AROS_SLIB_ENTRY(init,HIDD)
};

static const UBYTE name[] = "hiddclass";
static const UBYTE version[] = "hiddclass 41.1 (23.10.1997)\r\n";

static const char unknown[] = "--unknown device--";


static ULONG __HIDD_AttrBase;

#define ISHIDDATTR(attr, idx) ((idx = attr - __HIDD_AttrBase) < NUM_A_HIDD)

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
    struct Library		*UtilityBase;
    struct Library		*OOPBase;
    struct MinList		 hiddList;
    struct SignalSemaphore	 listLock;
};

#define OOPBase	(((struct HCD *)cl->UserData)->OOPBase)
#define UtilityBase	(((struct HCD *)cl->UserData)->UtilityBase)

/* Implementation of root HIDD class methods. */
static VOID hidd_set(Class *cl, Object *o, struct P_Root_Set *msg);


/******************
**  HIDD::New()  **
******************/
static Object *hidd_new(Class *cl, Object *o, struct P_Root_New *msg)
{
    EnterFunc(bug("HIDD::New(cl=%s)\n", cl->ClassNode.ln_Name));
    D(bug("DoSuperMethod:%p\n", cl->DoSuperMethod));
    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if(o)
    {
    	struct HIDDData *hd;
        struct TagItem *list = msg->AttrList;
	struct P_Root_Set set_msg;
	
	hd = INST_DATA(cl, o);

	/*  Initialise the HIDD class. These fields are publicly described
	    as not being settable at Init time, however it is the only way to
	    get proper abstraction if you ask me. Plus it does reuse code
	    in a nice way.

	    To pass these into the init code I would recommend that your
	    pass in a TagList of your tags, which is linked to the user's
	    tags by a TAG_MORE. This way you will prevent them from setting
	    these values.
	*/

	hd->hd_Type 	= GetTagData(HIDDA_Type, 	0, list);
	hd->hd_SubType 	= GetTagData(HIDDA_SubType, 	0, list);
	hd->hd_Producer = GetTagData(HIDDA_Producer, 	0, list);
	
	hd->hd_Name 	= (STRPTR)GetTagData(HIDDA_Name, 	(IPTR)unknown,	list);
	hd->hd_HWName 	= (STRPTR)GetTagData(HIDDA_HardwareName,(IPTR)unknown,	list);
	
	hd->hd_Status 	= GetTagData(HIDDA_Status, 	HIDDV_StatusUnknown, 	list);
	hd->hd_Locking 	= GetTagData(HIDDA_Locking, 	HIDDV_LockShared, 	list);
	hd->hd_ErrorCode= GetTagData(HIDDA_ErrorCode, 	0, list);

	hd->hd_Active 	= TRUE; /* Set default, GetTagData() comes later */
	
	/* Use OM_SET to set the rest */


	set_msg.AttrList = msg->AttrList;
	hidd_set(cl, o, &set_msg);
	
	
    }
    
    ReturnPtr("HIDD::New", Object *, o);
}


/******************
**  HIDD::Set()  **
******************/

static VOID hidd_set(Class *cl, Object *o, struct P_Root_Set *msg)
{

    struct TagItem *tstate = msg->AttrList;
    struct TagItem *tag;
    struct HIDDData *hd = INST_DATA(cl, o);

    while((tag = NextTagItem(&tstate)))
    {
    	ULONG idx;
	
    	if (ISHIDDATTR(tag->ti_Tag, idx))
	{
	    switch(idx)
	    {
		case HIDDAO_Active:
	    	    hd->hd_Active = tag->ti_Data;
	    	    break;
		    
	    }
	}
    }
    return;
}

/******************
**  HIDD::Get()  **
******************/
static VOID hidd_get(Class *cl, Object *o, struct P_Root_Get *msg)
{
    struct HIDDData *hd = INST_DATA(cl, o);
    ULONG idx;
    

    if (ISHIDDATTR(msg->AttrID, idx))
    {
    	switch (idx)
	{
	case HIDDAO_Type:
	    *msg->Storage = hd->hd_Type;
	    break;

	case HIDDAO_SubType:
	    *msg->Storage = hd->hd_SubType;
	    break;

	case HIDDAO_Producer:
	    *msg->Storage = hd->hd_Producer;
	    break;

	case HIDDAO_Name:
	    *msg->Storage = (IPTR)hd->hd_Name;
	    break;

	case HIDDAO_HardwareName:
	    *msg->Storage = (IPTR)hd->hd_HWName;
	    break;

	case HIDDAO_Active:
	    *msg->Storage = hd->hd_Active;
	    break;

	case HIDDAO_Status:
	    *msg->Storage = hd->hd_Status;
	    break;

	case HIDDAO_ErrorCode:
	    *msg->Storage = hd->hd_ErrorCode;
	    break;

	case HIDDAO_Locking:
	    *msg->Storage = hd->hd_Locking;
	    break;
	
	}
    }
    
    return;

}


/***********************************
**  Unimplemented methods 
*/




/*    switch(msg->MethodID)
    {
    case OM_NEW:
	retval = DoSuperMethodA(cl, o, msg);
	if(!retval)
	    break;

	hd = INST_DATA(cl, retval);

	if( hd != NULL)
	{
	    struct TagItem *list = ((struct opSet *)msg)->ops_AttrList;
	    hd->hd_Type = GetTagData(HIDDA_Type, 0, list);
	    hd->hd_SubType = GetTagData(HIDDA_SubType, 0, list);
	    hd->hd_Producer = GetTagData(HIDDA_Producer, 0, list);
	    hd->hd_Name = (STRPTR)GetTagData(HIDDA_Name, (IPTR)unknown, list);
	    hd->hd_HWName = (STRPTR)GetTagData(HIDDA_HardwareName, (IPTR)unknown, list);
	    hd->hd_Active = TRUE; 
	    hd->hd_Status = GetTagData(HIDDA_Status, HIDDV_StatusUnknown, list);
	    hd->hd_ErrorCode = GetTagData(HIDDA_ErrorCode, 0, list);
	    hd->hd_Locking = GetTagData(HIDDA_Locking, HIDDV_LockShared, list);
	}

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
	}
    }

*/


    /* These are the "hiddclass" methods. */

    /*	These two are invalid, since we don't have anything to get
	from a class, so the superclass should handle these.

	This is especially the case since the only place that we can
	get the information for these methods is from an object, but
	we don't have any objects if this method is called.
    */
/*    case HIDDM_Meta_Get:
    case HIDDM_Meta_MGet:
	retval = 0;
	break;
*/
    /*	Yet to determine the semantics of these so we just let
	them return 0 for now.
    */
/*    case HIDDM_BeginIO:
    case HIDDM_AbortIO:
	retval = 0;
	break;

    case HIDDM_LoadConfigPlugin:
    case HIDDM_Lock:
    case HIDDM_Unlock:
	retval = NULL;
	break;

    case HIDDM_AddHIDD:
    {

	Class *hc = ((hmAdd *)msg)->hma_Class;

	if( (hc->cl_Flags & CLF_INLIST) == 0 )
	{

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

    default:
	retval = DoSuperMethodA(cl, o, msg);
    }

    return retval;
}
*/
/* This is the initialisation code for the HIDD class itself. */
#undef OOPBase
#undef UtilityBase

#define NUM_ROOT_METHODS 3
#define NUM_HIDD_METHODS 0

AROS_UFH3(static ULONG, AROS_SLIB_ENTRY(init, HIDD),
    AROS_UFHA(ULONG, dummy1, D0),
    AROS_UFHA(ULONG, dummy2, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    struct Library *OOPBase;
    Class *cl;
    struct HCD *hcd;
    
    
    struct MethodDescr root_mdescr[NUM_ROOT_METHODS + 1] =
    {
    	{ (IPTR (*)())hidd_new,		MO_Root_New		},
    	{ (IPTR (*)())hidd_set,		MO_Root_Set		},
    	{ (IPTR (*)())hidd_get,		MO_Root_Get		},
    	{ NULL, 0UL }
    };

    
    struct MethodDescr hidd_mdescr[NUM_HIDD_METHODS + 1] =
    {
    	{ NULL, 0UL }
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_mdescr, IID_Root, NUM_ROOT_METHODS},
	{hidd_mdescr, IID_Hidd, NUM_HIDD_METHODS},
	{NULL, NULL, 0UL}
    
    };
    
    

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

    OOPBase = hcd->OOPBase = OpenLibrary("oop.library", 0);
    if(hcd->OOPBase == NULL)
    {
	FreeMem(hcd, sizeof(struct HCD));
	Alert(AT_DeadEnd | AG_OpenLib | AN_Unknown | AO_Unknown);
	return NULL;
    }

    hcd->UtilityBase = OpenLibrary("utility.library",0);
    if(hcd->UtilityBase == NULL)
    {
	CloseLibrary(hcd->OOPBase);
	FreeMem(hcd, sizeof(struct HCD));
	Alert(AT_DeadEnd | AG_OpenLib | AN_Unknown | AO_UtilityLib);
	return NULL;
    }

    /* Create the class structure for the "hiddclass" */
    {
        ULONG __OOPI_Meta = GetAttrBase(IID_Meta);
        struct TagItem tags[] =
    	{
            {A_Meta_SuperID,		(IPTR)CLID_Root},
	    {A_Meta_InterfaceDescr,	(IPTR)ifdescr},
	    {A_Meta_ID,			(IPTR)CLID_Hidd},
	    {A_Meta_InstSize,		(IPTR)sizeof (struct HIDDData) },
	    {TAG_DONE, 0UL}
    	};
    
	cl = NewObject(NULL, CLID_HIDDMeta, tags);
	if (cl == NULL)
	{
	    CloseLibrary(hcd->UtilityBase);
	    CloseLibrary(hcd->OOPBase);
	    FreeMem(hcd, sizeof(struct HCD));
	    Alert(AT_DeadEnd | AG_OpenLib | AN_Unknown | AO_Unknown);
	    return NULL;
        }
	
    }
    
    cl->UserData = hcd;
    __HIDD_AttrBase = GetAttrBase(IID_Hidd);

    AddClass(cl);

    return TRUE;
}
