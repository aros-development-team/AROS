/*
    (C) 1999 AROS - The Amiga Research OS
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
#include <aros/machine.h>
#include <aros/asmcall.h>

#include "irq.h"

#define DEBUG 1
#include <aros/debug.h>

static AttrBase HiddIRQAttrBase	= 0;

static struct ABDescr attrbases[] =
{
    { IID_Hidd_IRQ,	&HiddIRQAttrBase	},
    { NULL, NULL }
};

struct irq_data
{
    int	dummy;	//dummy!!!!!!!!!
};

/*** HIDDIRQ::AddHandler() ***************************************/

static BOOL irq_addhandler(Class *cl, Object *obj, struct pHidd_IRQ_AddHandler *msg)
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

static VOID irq_remhandler(Class *cl, Object *obj, struct pHidd_IRQ_RemHandler *msg)
{
    EnterFunc(bug("HIDDIRQ::RemHandler()\n"));
    D(bug("Removing handler %s\n",msg->handlerinfo->h_Node.ln_Name));

    Disable();
    Remove((struct Node *)msg->handlerinfo);
    Enable();

    ReturnVoid("HIDDIRQ::RemHandler");
}

/*** HIDDIRQ::CauseIRQ() *****************************************/

static VOID irq_causeirq(Class *cl, Object *obj, struct pHidd_CauseIRQ *msg)
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

Class *init_irqclass (struct irq_staticdata *isd)
{
    Class *cl = NULL;
    BOOL  ok  = FALSE;
    
    struct MethodDescr irqhidd_descr[NUM_IRQ_METHODS + 1] = 
    {
        {(IPTR (*)())irq_addhandler,    moHidd_IRQ_AddHandler},
        {(IPTR (*)())irq_remhandler,    moHidd_IRQ_RemHandler},
        {(IPTR (*)())irq_causeirq,      moHidd_CauseIRQ},
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {irqhidd_descr, IID_Hidd_IRQ, NUM_IRQ_METHODS},
        {NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
        { aMeta_SuperID,                (IPTR)CLID_Root},

        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_IRQ},
        { aMeta_InstSize,               (IPTR)sizeof (struct irq_data) },
        {TAG_DONE, 0UL}
    };
    
    EnterFunc(bug("    init_irqclass(isd=%p)\n", isd));

    cl = NewObject(NULL, CLID_HiddMeta, tags);
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
        AddClass(cl);
    }

    ReturnPtr("init_irqclass", Class *, cl);
}

void free_irqclass(struct irq_staticdata *isd)
{
    EnterFunc(bug("free_irqclass(isd=%p)\n", isd));

    if(isd)
    {
        RemoveClass(isd->irqclass);
	
        if(isd->irqclass) DisposeObject((Object *) isd->irqclass);
        isd->irqclass = NULL;
    }

    ReturnVoid("free_irqclass");
}


