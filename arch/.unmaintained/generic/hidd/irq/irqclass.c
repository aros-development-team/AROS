/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IRQ class
    Lang: english
*/

#include <hidd/irq.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>
#include <aros/system.h>
#include <aros/asmcall.h>

#define DEBUG 0
#include <aros/debug.h>

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddIRQAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_IRQ,	&HiddIRQAttrBase	},
    { NULL, NULL }
};

struct irq_data
{
    int	dummy;	//dummy!!!!!!!!!
};

/*** HIDDIRQ::AddHandler() ***************************************/

static BOOL irq_addhandler(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_AddHandler *msg)
{
    EnterFunc(bug("HIDDIRQ::AddHandler()\n"));
    D(bug("Adding handler %s\n",msg->handlerinfo->h_Node.ln_Name));

    if (msg->id < vHidd_IRQ_NumIRQ)
    {
        /* Adding an IRQ handler has to be atomic */
        Disable();
        Enqueue((struct List *)&ISD(cl)->irqlist[msg->id],(struct Node *)msg->handlerinfo);
        Enable();

        ReturnInt("HIDDIRQ::AddHandler", ULONG, TRUE);
    }
    
    ReturnInt("HIDDIRQ::AddHandler", ULONG, FALSE);
}

/*** HIDDIRQ::RemHandler() ***************************************/

static VOID irq_remhandler(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_RemHandler *msg)
{
    EnterFunc(bug("HIDDIRQ::RemHandler()\n"));
    D(bug("Removing handler %s\n",msg->handlerinfo->h_Node.ln_Name));

    Disable();
    Remove((struct Node *)msg->handlerinfo);
    Enable();

    ReturnVoid("HIDDIRQ::RemHandler");
}

/*** HIDDIRQ::CauseIRQ() *****************************************/

static VOID irq_causeirq(OOP_Class *cl, OOP_Object *obj, struct pHidd_CauseIRQ *msg)
{
    EnterFunc(bug("HIDDIRQ::CauseIRQ()\n"));
#warning TODO: Write CauseIRQ method
    ReturnVoid("HIDDIRQ::CauseIRQ");
}

/*************************** Classes *****************************/

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase     (isd->sysbase)
#define OOPBase     (isd->oopbase)
#define UtilityBase (isd->utilitybase)

#define NUM_IRQ_METHODS moHidd_IRQ_NumMethods

OOP_Class *init_irqclass (struct irq_staticdata *isd)
{
    OOP_Class *cl = NULL;
    BOOL  ok  = FALSE;
    
    struct OOP_MethodDescr irqhidd_descr[NUM_IRQ_METHODS + 1] = 
    {
        {(IPTR (*)())irq_addhandler,    moHidd_IRQ_AddHandler},
        {(IPTR (*)())irq_remhandler,    moHidd_IRQ_RemHandler},
        {(IPTR (*)())irq_causeirq,      moHidd_CauseIRQ},
        {NULL, 0UL}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {irqhidd_descr, IID_Hidd_IRQ, NUM_IRQ_METHODS},
        {NULL, NULL, 0}
    };
    
    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
        { aMeta_SuperID,                (IPTR)CLID_Root},

        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_IRQ},
        { aMeta_InstSize,               (IPTR)sizeof (struct irq_data) },
        {TAG_DONE, 0UL}
    };
    
    EnterFunc(bug("    init_irqclass(isd=%p)\n", isd));

    cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
    if(cl)
    {
        D(bug("IRQClass ok\n"));
        cl->UserData = (APTR)isd;
        isd->irqclass = cl;
        
        ok = TRUE;
    }

    if(ok == FALSE)
    {
        free_irqclass(isd);
        cl = NULL;
    }
    else
    {
        OOP_AddClass(cl);
    }

    ReturnPtr("init_irqclass", Class *, cl);
}

void free_irqclass(struct irq_staticdata *isd)
{
    EnterFunc(bug("free_irqclass(isd=%p)\n", isd));

    if(isd)
    {
        OOP_RemoveClass(isd->irqclass);
	
        if(isd->irqclass) OOP_DisposeObject((OOP_Object *) isd->irqclass);
        isd->irqclass = NULL;
    }

    ReturnVoid("free_irqclass");
}
