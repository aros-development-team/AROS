/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: The main mouse class.
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>

#include <devices/inputevent.h>

#include "mouse.h"

#define DEBUG 0
#include <aros/debug.h>

/* !!!!!!!!!! Remove all .data from file
*/
#define HiddMouseAB	(MSD(cl)->hiddMouseAB)
/*
static OOP_AttrBase HiddMouseAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB },
    { NULL, NULL }
};

*/

struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
    
    OOP_Object * Ser;
    OOP_Object * Unit;
    
    UWORD buttonstate;
};

/* defines for buttonstate */

#define LEFT_BUTTON 	1
#define RIGHT_BUTTON 	2
#define MIDDLE_BUTTON	4

/***** Mouse::New()  ***************************************/
static OOP_Object * _mouse_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;
   
    EnterFunc(bug("_Mouse::New()\n"));
 
    ObtainSemaphoreShared( &MSD(cl)->sema);
 
    if (MSD(cl)->mousehidd)
        has_mouse_hidd = TRUE;

    ReleaseSemaphore( &MSD(cl)->sema);
 
    if (has_mouse_hidd) /* Cannot open twice */
        ReturnPtr("_Mouse::New", Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct mouse_data *data = OOP_INST_DATA(cl, o);
        struct TagItem *tag, *tstate;

	tstate = msg->attrList;
        while ((tag = NextTagItem((const struct TagItem **)&tstate)))
        {
            ULONG idx;

            if (IS_HIDDMOUSE_ATTR(tag->ti_Tag, idx))
            {
                switch (idx)
		{
                    case aoHidd_Mouse_IrqHandler:
                        data->mouse_callback = (APTR)tag->ti_Data;
		        break;

                    case aoHidd_Mouse_IrqHandlerData:
                        data->callbackdata = (APTR)tag->ti_Data;
                        break;
		}
	    }
	} /* while (tags to process) */

        ObtainSemaphore( &MSD(cl)->sema);
        MSD(cl)->mousehidd = o;
        ReleaseSemaphore( &MSD(cl)->sema);
    }
    return o;
}

/***** Mouse::HandleEvent()  ***************************************/

static VOID _mouse_handleevent(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_HandleEvent *msg)
{
    struct mouse_data * data;

    EnterFunc(bug("_mouse_handleevent()\n"));

    data = OOP_INST_DATA(cl, o);

/* Nothing done yet */

    ReturnVoid("_Mouse::HandleEvent");
}

#undef MSD
#define MSD(cl) msd

/********************  init_kbdclass()  *********************************/

#define NUM_ROOT_METHODS 1
#define NUM_MOUSE_METHODS 1

OOP_Class *_init_mouseclass (struct mouse_staticdata *msd)
{
    OOP_Class *cl = NULL;

    struct OOP_ABDescr attrbases[] =
    {
	{ IID_Hidd_Mouse, &msd->hiddMouseAB },
        { NULL, NULL }
    };
	
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
        {OOP_METHODDEF(_mouse_new),  moRoot_New},
        {NULL, 0UL}
    };
    
    struct OOP_MethodDescr mousehidd_descr[NUM_MOUSE_METHODS + 1] = 
    {
        {OOP_METHODDEF(_mouse_handleevent),  moHidd_Mouse_HandleEvent},
        {NULL, 0UL}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr, IID_Root,  NUM_ROOT_METHODS},
        {mousehidd_descr,       IID_Hidd_PCmouse, NUM_MOUSE_METHODS},
        {NULL, NULL, 0}
    };
    
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,         (IPTR)CLID_Hidd },
        {aMeta_InterfaceDescr,  (IPTR)ifdescr},
        {aMeta_InstSize,        (IPTR)sizeof (struct mouse_data) },
        {aMeta_ID,              (IPTR)CLID_Hidd_PCmouse },
        {TAG_DONE, 0UL}
    };

    EnterFunc(bug("_MouseHiddClass init\n"));
    
    if (MetaAttrBase)
    {
        cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            cl->UserData = (APTR)msd;
            msd->mouseclass = cl;
    
            if (OOP_ObtainAttrBases(attrbases))
            {
                D(bug("_MouseHiddClass ok\n"));

                OOP_AddClass(cl);
            }
            else
            {
                _free_mouseclass(msd);
                cl = NULL;
            }
        }
        /* Don't need this anymore */
        OOP_ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("_init_mouseclass", Class *, cl);
}

/*************** free_kbdclass()  **********************************/
VOID _free_mouseclass(struct mouse_staticdata *msd)
{
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd_Mouse, &msd->hiddMouseAB },
        { NULL, NULL }
    };
    
    EnterFunc(bug("_free_mouseclass(msd=%p)\n", msd));

    if(msd)
    {
        OOP_RemoveClass(msd->mouseclass);

        if(msd->mouseclass) OOP_DisposeObject((OOP_Object *) msd->mouseclass);
        msd->mouseclass = NULL;

        OOP_ReleaseAttrBases(attrbases);
    }
    ReturnVoid("_free_mouseclass");
}
