/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.

    Desc: Parallel Unit hidd class implementation.
*/

#define __OOP_NOATTRBASES__

/* the rest are Amiga includes */
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/alib.h>
#include <proto/cia.h>

#include <exec/libraries.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/lists.h>
#include <aros/symbolsets.h>

#include <utility/tagitem.h>
#include <hidd/parallel.h>

#include <hardware/cia.h>

#include "parallel_intern.h"

#include LC_LIBDEFS_FILE

#include <aros/debug.h>

void parallelunit_receive_data();
void parallelunit_write_more_data();

/*************************** Classes *****************************/

static OOP_AttrBase HiddParallelUnitAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_ParallelUnit, &HiddParallelUnitAB },
    { NULL,     NULL }
};

/******* ParallelUnit::New() ***********************************/
OOP_Object *AmigaParUnit__Root__New(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
        struct TagItem *tag, *tstate;
        ULONG unitnum = 0;

        EnterFunc(bug("ParallelUnit::New()\n"));

        tstate = msg->attrList;
        while ((tag = NextTagItem(&tstate))) {
                ULONG idx;

                if (IS_HIDDPARALLELUNIT_ATTR(tag->ti_Tag, idx)) {
                        switch (idx) {
                                case aoHidd_ParallelUnit_Unit:
                                        unitnum = (ULONG)tag->ti_Data;
                                break;
                        }
                }

        } /* while (tags to process) */

        if (unitnum != 0)
            ReturnPtr("ParallelUnit::New()", OOP_Object *, NULL);

        D(bug("!!!!Request for unit number %d\n",unitnum));

        obj = (OOP_Object *)OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);

        if (!obj) {
            D(bug("%s - an error occurred!\n",__FUNCTION__));
        }

        ReturnPtr("ParallelUnit::New()", OOP_Object *, obj);
}

/******* ParallelUnit::Dispose() ***********************************/
OOP_Object *AmigaParUnit__Root__Dispose(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
        struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, obj);
        struct Interrupt *irq = &data->parint;

        EnterFunc(bug("ParallelUnit::Dispose()\n"));

        /* stop all interrupts */
        if (data->ciares)
            RemICRVector(data->ciares, 4, irq);

        OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
        ReturnPtr("ParallelUnit::Dispose()", OOP_Object *, obj);
}

static AROS_INTH1(parallel_interrupt, struct HIDDParallelUnitData *, data)
{
    AROS_INTFUNC_INIT

    data->DataWriteCallBack(0, data->DataWriteUserData);

    return 0;

    AROS_INTFUNC_EXIT
}

/******* ParallelUnit::Init() **********************************/
BOOL AmigaParUnit__Hidd_ParallelUnit__Init(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Init *msg)
{
    struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);
    struct Interrupt *irq = &data->parint;

    EnterFunc(bug("ParallelUnit::Init()\n"));
    data->ciares = OpenResource("ciaa.resource");
    if (!data->ciares)
        ReturnBool("ParallelUnit::Init()", FALSE);

    data->DataReceivedCallBack = msg->DataReceived;
    data->DataReceivedUserData = msg->DataReceivedUserData;
    data->DataWriteCallBack    = msg->WriteData;
    data->DataWriteUserData    = msg->WriteDataUserData;

    irq = &data->parint;
    irq->is_Node.ln_Pri = 0;
    irq->is_Node.ln_Type = NT_INTERRUPT;
    irq->is_Node.ln_Name = "par";
    irq->is_Code = (APTR)parallel_interrupt;
    irq->is_Data = data;

    if (AddICRVector(data->ciares, 4, irq))
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);

    ReturnBool("ParallelUnit::Init()", TRUE);
}

/******* ParallelUnit::Write() **********************************/
ULONG AmigaParUnit__Hidd_ParallelUnit__Write(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Write *msg)
{
        struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);
        volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
        ULONG len = msg->Length;

        EnterFunc(bug("ParallelUnit::Write()\n"));
        /*
         * If the output is currently stopped just don't do anything here.
         */
        if (TRUE == data->stopped || len < 1)
                return 0;

        ciaa->ciaprb = msg->Outbuffer[0];

        ReturnInt("ParallelUnit::Write()",ULONG, 1);
}

/******* ParallelUnit::Start() **********************************/
VOID AmigaParUnit__Hidd_ParallelUnit__Start(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Start *msg)
{
        struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);
        volatile struct CIA *ciaa = (struct CIA*)0xbfe001;

        /* Ensure that the parallel port is in output mode */
        ciaa->ciaddrb = 0xff;

        /*
         * Allow or start feeding the CIA with data. Get the data
         * from upper layer.
         */
        if (TRUE == data->stopped) {
                if (NULL != data->DataWriteCallBack)
                         data->DataWriteCallBack(0, data->DataWriteUserData);
                /*
                 * Also mark the stopped flag as FALSE.
                 */
                data->stopped = FALSE;
        }
}

/******* ParallelUnit::Stop() **********************************/
VOID AmigaParUnit__Hidd_ParallelUnit__Stop(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_Stop *msg)
{
        struct HIDDParallelUnitData * data = OOP_INST_DATA(cl, o);

        /*
         * The next time the interrupt comes along and asks for
         * more data we just don't do anything...
         */
        data->stopped = TRUE;
}

/****** ParallelUnit::GetStatus ********************************/
UWORD AmigaParUnit__Hidd_ParallelUnit__GetStatus(OOP_Class *cl, OOP_Object *o, struct pHidd_ParallelUnit_GetStatus *msg)
{
        volatile struct CIA *ciab = (struct CIA*)0xbfd000;

        return ciab->ciapra & 7;
}


/******* init_parallelunitclass ********************************/

static int AmigaParUnit_Init(LIBBASETYPEPTR LIBBASE)
{
    ReturnInt("AmigaParUnit_Init", ULONG, OOP_ObtainAttrBases(attrbases));
}


static int AmigaParUnit_Expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    ReturnInt("AmigaParUnit_Expunge", int, TRUE);
}

ADD2INITLIB(AmigaParUnit_Init, 0)
ADD2EXPUNGELIB(AmigaParUnit_Expunge, 0)
