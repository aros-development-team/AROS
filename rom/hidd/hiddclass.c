/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Main class for HIDD.
    Lang: english
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/alerts.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <oop/oop.h>
#include <hidd/hidd.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "hiddclass_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

static const char unknown[]  = "--unknown device--";

#define IS_HIDD_ATTR(attr, idx) ((idx = attr - HiddAttrBase) < num_Hidd_Attrs)

/* Implementation of root HIDD class methods. */
static VOID hidd_set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg);


/*** HIDD::New() **************************************************************/

static OOP_Object *hidd_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("HIDD::New(cl=%s)\n", cl->ClassNode.ln_Name));
    D(bug("DoSuperMethod:%p\n", cl->DoSuperMethod));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if(o)
    {
        struct HIDDData *hd;
        struct TagItem *list = msg->attrList;
        struct pRoot_Set set_msg;
        
        hd = OOP_INST_DATA(cl, o);

        /*  Initialise the HIDD class. These fields are publicly described
            as not being settable at Init time, however it is the only way to
            get proper abstraction if you ask me. Plus it does reuse code
            in a nice way.

            To pass these into the init code I would recommend that your
            pass in a TagList of your tags, which is linked to the user's
            tags by a TAG_MORE. This way you will prevent them from setting
            these values.
        */

        hd->hd_Type     = GetTagData(aHidd_Type,        0, list);
        hd->hd_SubType  = GetTagData(aHidd_SubType,     0, list);
        hd->hd_Producer = GetTagData(aHidd_Producer,    0, list);
        
        hd->hd_Name     = (STRPTR)GetTagData(aHidd_Name,        (IPTR)unknown,  list);
        hd->hd_HWName   = (STRPTR)GetTagData(aHidd_HardwareName,(IPTR)unknown,  list);
        
        hd->hd_Status   = GetTagData(aHidd_Status,      vHidd_StatusUnknown,    list);
        hd->hd_Locking  = GetTagData(aHidd_Locking,     vHidd_LockShared,       list);
        hd->hd_ErrorCode= GetTagData(aHidd_ErrorCode,   0, list);

        hd->hd_Active   = TRUE; /* Set default, GetTagData() comes later */
        
        /* Use OM_SET to set the rest */


        set_msg.attrList = msg->attrList;
        hidd_set(cl, o, &set_msg);
    }
    
    ReturnPtr("HIDD::New", OOP_Object *, o);
}


/*** HIDD::Set() **************************************************************/

static VOID hidd_set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{

    struct TagItem  *tstate = msg->attrList;
    struct TagItem  *tag;
    struct HIDDData *hd = OOP_INST_DATA(cl, o);

    EnterFunc(bug("HIDD::Set(cl=%s)\n", cl->ClassNode.ln_Name));

    while((tag = NextTagItem((const struct TagItem **)&tstate)))
    {
        ULONG idx;
        
        if (IS_HIDD_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_Active:
                    hd->hd_Active = tag->ti_Data;
                    break;
                    
            }
        }
    }

    ReturnVoid("HIDD::Set");
}


/*** HIDD::Get() **************************************************************/

static VOID hidd_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HIDDData *hd = OOP_INST_DATA(cl, o);
    ULONG idx;
    
    EnterFunc(bug("HIDD::Get(cl=%s)\n", cl->ClassNode.ln_Name));

    if (IS_HIDD_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Type        : *msg->storage = hd->hd_Type;         break;
            case aoHidd_SubType     : *msg->storage = hd->hd_SubType;      break;
            case aoHidd_Producer    : *msg->storage = hd->hd_Producer;     break;
            case aoHidd_Name        : *msg->storage = (IPTR)hd->hd_Name;   break;
            case aoHidd_HardwareName: *msg->storage = (IPTR)hd->hd_HWName; break;
            case aoHidd_Active      : *msg->storage = hd->hd_Active;       break;
            case aoHidd_Status      : *msg->storage = hd->hd_Status;       break;
            case aoHidd_ErrorCode   : *msg->storage = hd->hd_ErrorCode;    break;
            case aoHidd_Locking     : *msg->storage = hd->hd_Locking;      break;
	    default		    : OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);	   break;
        }
    } else {
        OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    }
    
    
    ReturnVoid("HIDD::Get");
}


#if 0
/***********************************
**  Unimplemented methods 
*/




/*    switch(msg->MethodID)
    {
    case OM_NEW:
        retval = OOP_DoSuperMethodA(cl, o, msg);
        if(!retval)
            break;

        hd = OOP_INST_DATA(cl, retval);

        if( hd != NULL)
        {
            struct TagItem *list = ((struct opSet *)msg)->ops_AttrList;
            hd->hd_Type = GetTagData(aHidd_Type, 0, list);
            hd->hd_SubType = GetTagData(aHidd_SubType, 0, list);
            hd->hd_Producer = GetTagData(aHidd_Producer, 0, list);
            hd->hd_Name = (STRPTR)GetTagData(aHidd_Name, (IPTR)unknown, list);
            hd->hd_HWName = (STRPTR)GetTagData(aHidd_HardwareName, (IPTR)unknown, list);
            hd->hd_Active = TRUE; 
            hd->hd_Status = GetTagData(aHidd_Status, HIDDV_StatusUnknown, list);
            hd->hd_ErrorCode = GetTagData(aHidd_ErrorCode, 0, list);
            hd->hd_Locking = GetTagData(aHidd_Locking, HIDDV_LockShared, list);
        }

    case OM_SET:
    {
        struct TagItem *tstate = ((struct opSet *)msg)->ops_AttrList;
        struct TagItem *tag;

        while((tag = NextTagItem(&tstate)))
        {
            switch(tag->ti_Tag)
            {
            case aHidd_Active:
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
        case aHidd_Type:
            *((struct opGet *)msg)->opg_Storage = hd->hd_Type;
            break;

        case aHidd_SubType:
            *((struct opGet *)msg)->opg_Storage = hd->hd_SubType;
            break;

        case aHidd_Producer:
            *((struct opGet *)msg)->opg_Storage = hd->hd_Producer;
            break;

        case aHidd_Name:
            *((struct opGet *)msg)->opg_Storage = (IPTR)hd->hd_Name;
            break;

        case aHidd_HardwareName:
            *((struct opGet *)msg)->opg_Storage = (IPTR)hd->hd_HWName;
            break;

        case aHidd_Active:
            *((struct opGet *)msg)->opg_Storage = hd->hd_Active;
            break;

        case aHidd_Status:
            *((struct opGet *)msg)->opg_Storage = hd->hd_Status;
            break;

        case aHidd_ErrorCode:
            *((struct opGet *)msg)->opg_Storage = hd->hd_ErrorCode;
            break;

        case aHidd_Locking:
            *((struct opGet *)msg)->opg_Storage = hd->hd_Locking;
            break;
        }
    }

*/


    /* These are the "hiddclass" methods. */

    /*  These two are invalid, since we don't have anything to get
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
    /*  Yet to determine the semantics of these so we just let
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
        retval = OOP_DoSuperMethod(cl, o, msg);
    }

    return retval;
}
*/
#endif


/*************************** Classes *****************************/

#undef SysBase
#undef UtilityBase
#undef OOPBase

#define SysBase     (lh->hd_SysBase)
#define UtilityBase (csd->UtilityBase)
#define OOPBase     (csd->OOPBase)

#define NUM_ROOT_METHODS 3
#define NUM_HIDD_METHODS 0

ULONG init_hiddclass(struct IntHIDDClassBase *lh)
{
    OOP_Class  *cl = NULL;
    struct  class_static_data *csd;
    ULONG   alert = AT_DeadEnd | AN_Unknown | AO_Unknown;
    ULONG   ok    = 0;

    struct OOP_MethodDescr root_mdescr[NUM_ROOT_METHODS + 1] =
    {
        { (IPTR (*)())hidd_new,         moRoot_New              },
        { (IPTR (*)())hidd_set,         moRoot_Set              },
        { (IPTR (*)())hidd_get,         moRoot_Get              },
        { NULL, 0UL }
    };

    
    struct OOP_MethodDescr hidd_mdescr[NUM_HIDD_METHODS + 1] =
    {
        { NULL, 0UL }
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
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

    EnterFunc(bug("HIDD::Init()\n"));

    /* If you are not running from ROM, don't use Alert() */

    alert = AT_DeadEnd | AG_NoMemory | AN_Unknown;
    csd = lh->hd_csd = AllocMem(sizeof(struct class_static_data), MEMF_CLEAR|MEMF_PUBLIC);
    if(csd)
    {
    	D(bug("Got CSD\n"));
        NEWLIST(&csd->hiddList);
        InitSemaphore(&csd->listLock);
	
	csd->sysBase = SysBase;

        alert = AT_DeadEnd | AG_OpenLib | AN_Unknown | AO_Unknown;
        OOPBase = OpenLibrary(AROSOOP_NAME, 0);
        if(OOPBase)
        {
	    D(bug("Got OOPBase\n"));
            UtilityBase = OpenLibrary("utility.library", 0);
            if(UtilityBase)
            {
                /* Create the class structure for the "hiddclass" */

                OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);

                struct TagItem tags[] =
                {
                    {aMeta_SuperID,             (IPTR)CLID_Root},
                    {aMeta_InterfaceDescr,      (IPTR)ifdescr},
                    {aMeta_ID,                  (IPTR)CLID_Hidd},
                    {aMeta_InstSize,            (IPTR)sizeof (struct HIDDData) },
                    {TAG_DONE, 0UL}
                };

	    	D(bug("Got UtilityBase\n"));

                alert = AT_DeadEnd | AN_Unknown | AO_Unknown;
                cl = csd->hiddclass = OOP_NewObject(NULL, CLID_HiddMeta, tags);
                if(cl)
                {
		    D(bug("Class created\n"));
                    cl->UserData = csd;

                    HiddAttrBase = OOP_ObtainAttrBase(IID_Hidd);
                    if(HiddAttrBase)
                    {
		    	D(bug("Got HiddAttrBase\n"));
                        OOP_AddClass(cl);
                        ok = 1;
                    } /* if(HiddAttrBase) */

                } /* if(cl) */

            } /* if(UtilityBase) */

        } /* if(OOPBase) */
    }

    if(ok == 0)
    {
        free_hiddclass(lh);

        lh->hd_csd = NULL;

        /* If you are not running from ROM, don't use Alert() */

        Alert(alert);
    }

    ReturnInt("HIDD::Init", ULONG, ok);
}


VOID free_hiddclass(struct IntHIDDClassBase *lh)
{
    struct class_static_data *csd = lh->hd_csd;

    EnterFunc(bug("HIDD::Free()\n"));

    if(csd)
    {
        if(csd->hiddclass)
        {
            OOP_RemoveClass(csd->hiddclass);
            if(csd->hiddAttrBase)
            {
                OOP_ReleaseAttrBase(IID_Hidd);
                csd->hiddAttrBase = 0;
            }

            OOP_DisposeObject((OOP_Object *) csd->hiddclass);
        }


        if(OOPBase)     CloseLibrary(OOPBase);
        if(UtilityBase) CloseLibrary(UtilityBase);

        FreeMem(csd, sizeof(struct class_static_data));

        lh->hd_csd = NULL;
    }

    ReturnVoid("HIDD::Free");
}

