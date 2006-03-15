/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The main touchscreen class. Actually a mouse class but still
          implements a touchscreen.
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hardware/intbits.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <hidd/irq.h>

#include <devices/inputevent.h>
#include <string.h>

#include "touchscreen.h"

#define DEBUG 1
#include <aros/debug.h>

#define HiddMouseAB	(TSD(cl)->hiddMouseAB)

/* Prototypes */

/***** Mouse::New()  ***************************************/
static OOP_Object * _mouse_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
	BOOL has_mouse_hidd = FALSE;
   
	EnterFunc(bug("_Mouse::New()\n"));
 
	ObtainSemaphoreShared( &TSD(cl)->sema);
 
	if (TSD(cl)->mousehidd)
		has_mouse_hidd = TRUE;

	ReleaseSemaphore( &TSD(cl)->sema);
 
	if (has_mouse_hidd) /* Cannot open twice */
		ReturnPtr("_Mouse::New", OOP_Object *, NULL); /* Should have some error code here */

	o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
	if (o) {
		struct mouse_data *data = OOP_INST_DATA(cl, o);
		struct TagItem *tag, *tstate;

		tstate = msg->attrList;

		/* Search for all mouse attrs */

		while ((tag = NextTagItem((const struct TagItem **)&tstate))) {
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

		/*
		 * Try to get access to the IRQ hidd.
		 */
		data->irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
		if (NULL != data->irqhidd) {
			HIDDT_IRQ_Handler * irq;
			data->irq = irq = AllocMem(sizeof(HIDDT_IRQ_Handler),
			                           MEMF_CLEAR | MEMF_PUBLIC);
			if (NULL != irq) {
				irq->h_Node.ln_Pri  = 127;
				irq->h_Node.ln_Name = "Mouse class irq";
				irq->h_Code         = touchscreen_int;
				irq->h_Data         = (APTR)data;

				HIDD_IRQ_AddHandler(data->irqhidd, irq, vHidd_IRQ_Mouse);
				D(bug("Added IRQ Handler for TOUCHSCREEN (virq=%d)!\n",vHidd_IRQ_Mouse));
			}
			
		}

		data->VBlank.is_Code         = (APTR)&tsVBlank;
		data->VBlank.is_Data         = (APTR)data;
		data->VBlank.is_Node.ln_Name = "Mouse VBlank server";
		data->VBlank.is_Node.ln_Pri  = 0;
		data->VBlank.is_Node.ln_Type = NT_INTERRUPT;
		AddIntServer(INTB_VERTB, &data->VBlank);
		D(bug("Add INT_VERTB for mouse.\n"));

		data->state = STATE_IDLE;
		data->idlectr = 0;

		ObtainSemaphore( &TSD(cl)->sema);
		TSD(cl)->mousehidd = o;
		ReleaseSemaphore( &TSD(cl)->sema);
	}
	return o;
}




STATIC VOID _mouse_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg) 
{
	struct mouse_data *data = OOP_INST_DATA(cl, o);

	ObtainSemaphore( &TSD(cl)->sema);
	TSD(cl)->mousehidd = NULL;
	ReleaseSemaphore( &TSD(cl)->sema);

	HIDD_IRQ_RemHandler(data->irqhidd, data->irq);
	FreeMem(data->irq, sizeof(HIDDT_IRQ_Handler));

	RemIntServer(INTB_VERTB, &data->VBlank);

	OOP_DisposeObject(data->irqhidd);
	OOP_DoSuperMethod(cl, o, msg);
}

/***** Mouse::Get()  ***************************************/
static VOID _mouse_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
	struct mouse_data *data = OOP_INST_DATA(cl, o);
	ULONG idx;

	if (IS_HIDDMOUSE_ATTR(msg->attrID, idx)) {
		switch (idx) {
			case aoHidd_Mouse_IrqHandler:
				*msg->storage = (IPTR)data->mouse_callback;
			break;

			case aoHidd_Mouse_IrqHandlerData:
				*msg->storage = (IPTR)data->callbackdata;
			break;

			case aoHidd_Mouse_State:
			break;
		}
	}
}

/***** Mouse::HandleEvent()  ***************************************/

static VOID _mouse_handleevent(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_HandleEvent *msg)
{
	struct mouse_data * data;

	EnterFunc(bug("_touechscreen_handleevent()\n"));

	data = OOP_INST_DATA(cl, o);

/* Nothing done yet */

	ReturnVoid("_Mouse::HandleEvent");
}

#undef TSD
#define TSD(cl) tsd

/********************  init_kbdclass()  *********************************/

#define NUM_ROOT_METHODS 3
#define NUM_TOUCHSCREEN_METHODS 1

OOP_Class *_init_mouseclass (struct mouse_staticdata *tsd)
{
	OOP_Class *cl = NULL;

	struct OOP_ABDescr attrbases[] =
	{
		{ IID_Hidd_Mouse, &tsd->hiddMouseAB },
		{ NULL, NULL }
	};
	
	struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
	{
		{OOP_METHODDEF(_mouse_new),     moRoot_New},
		{OOP_METHODDEF(_mouse_dispose), moRoot_Dispose},
		{OOP_METHODDEF(_mouse_get),     moRoot_Get},
		{NULL, 0UL}
	};
	
	struct OOP_MethodDescr mousehidd_descr[NUM_TOUCHSCREEN_METHODS + 1] = 
	{
		{OOP_METHODDEF(_mouse_handleevent),  moHidd_Mouse_HandleEvent},
		{NULL, 0UL}
	};
	
	struct OOP_InterfaceDescr ifdescr[] =
	{
		{root_descr, IID_Root,  NUM_ROOT_METHODS},
		{mousehidd_descr, IID_Hidd_DBmouse, NUM_TOUCHSCREEN_METHODS},
		{NULL, NULL, 0}
	};
	
	OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

	struct TagItem tags[] =
	{
		{aMeta_SuperID,		 (IPTR)CLID_Hidd },
		{aMeta_InterfaceDescr,   (IPTR)ifdescr},
		{aMeta_InstSize,         (IPTR)sizeof (struct mouse_data) },
		{aMeta_ID,               (IPTR)CLID_Hidd_DBmouse },
		{TAG_DONE, 0UL}
	};

	EnterFunc(bug("_MouseHiddClass init\n"));
	
	if (MetaAttrBase) {
		cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
		if(cl) {
			cl->UserData = (APTR)tsd;
			tsd->mouseclass = cl;
	
			if (OOP_ObtainAttrBases(attrbases)) {
				D(bug("_MouseHiddClass ok\n"));

				OOP_AddClass(cl);
			} else {
				_free_mouseclass(tsd);
				cl = NULL;
			}
		}
		/* Don't need this anymore */
		OOP_ReleaseAttrBase(IID_Meta);
	}
	ReturnPtr("_init_mouseclass", OOP_Class *, cl);
}

/*************** free_mouseclass()  **********************************/
VOID _free_mouseclass(struct mouse_staticdata *tsd)
{
	struct OOP_ABDescr attrbases[] =
	{
		{ IID_Hidd_Mouse, &tsd->hiddMouseAB },
		{ NULL, NULL }
	};
	
	EnterFunc(bug("_free_mouseclass(tsd=%p)\n", tsd));

	if(tsd) {
		OOP_RemoveClass(tsd->mouseclass);

		if(tsd->mouseclass) OOP_DisposeObject((OOP_Object *) tsd->mouseclass);
		tsd->mouseclass = NULL;

		OOP_ReleaseAttrBases(attrbases);
	}
	ReturnVoid("_free_mouseclass");
}
