/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Paula serial unit hidd class implementation.
*/

/*
  This driver talks to Paula's UART: one write register (SERDAT), one read
  register (SERDATR), one baud divisor (SERPER) and two interrupts, TBE and
  RBF, which the kernel dispatches from level 1 and level 5 respectively
  (see arch/m68k-amiga/kernel/amiga_irq.c).

  Write() hands the UART one byte and returns 1; serial.device then keeps the
  request active and feeds the rest from the TBE interrupt. Paula double
  buffers SERDAT against its shift register, so one byte in flight per
  interrupt keeps the line saturated.

  The port stays shared with the kernel's own debug output, which writes SERDAT
  directly from krnPutC(). That works because krnPutC() only writes once TBE
  says the buffer is free, and because every drain raises TBE regardless of who
  filled it: a debug character simply causes one extra callback, which sends
  the next queued byte early rather than losing it. The short spin in Write()
  covers the converse race, where krnPutC() takes the buffer between the
  interrupt firing and us refilling it; without it a lost TBE would strand the
  write queue permanently.
*/

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <aros/symbolsets.h>

#include <utility/tagitem.h>
#include <hidd/serial.h>
#include <hardware/intbits.h>
#include <devices/serial.h>

#include "serial_intern.h"

#include LC_LIBDEFS_FILE

#include <aros/debug.h>

/******* SerialUnit::New() *************************************************/
OOP_Object *AmigaSerUnit__Root__New(OOP_Class *cl, OOP_Object *obj,
                                    struct pRoot_New *msg)
{
    struct class_static_data *csd = CSD(cl->UserData);
    struct TagItem *tag, *tstate;
    ULONG unitnum = 0;

    EnterFunc(bug("SerialUnit::New()\n"));

    tstate = msg->attrList;
    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;

        if (IS_HIDDSERIALUNIT_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
                case aoHidd_SerialUnit_Unit:
                    unitnum = (ULONG)tag->ti_Data;
                break;
            }
        }
    }

    if (unitnum != 0)
        ReturnPtr("SerialUnit::New()", OOP_Object *, NULL);

    obj = (OOP_Object *)OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);

    if (obj)
    {
        struct HIDDSerialUnitData *data = OOP_INST_DATA(cl, obj);

        data->unitnum = unitnum;
    }

    ReturnPtr("SerialUnit::New()", OOP_Object *, obj);
}

/******* SerialUnit::Dispose() *********************************************/
OOP_Object *AmigaSerUnit__Root__Dispose(OOP_Class *cl, OOP_Object *obj,
                                        OOP_Msg msg)
{
    struct HIDDSerialUnitData *data = OOP_INST_DATA(cl, obj);

    EnterFunc(bug("SerialUnit::Dispose()\n"));

    if (data->intsadded)
    {
        RemIntServer(INTB_RBF, &data->rbfint);
        RemIntServer(INTB_TBE, &data->tbeint);
        data->intsadded = FALSE;
    }

    OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
    ReturnPtr("SerialUnit::Dispose()", OOP_Object *, obj);
}

/*
 * The kernel clears INTREQ before running the server chain, so this only has
 * to collect the byte. SERDATR keeps the data until the next one arrives.
 */
static AROS_INTH1(serial_rbf_interrupt, struct HIDDSerialUnitData *, data)
{
    AROS_INTFUNC_INIT

    UBYTE c = SERDATR_DB8_of(custom_r(SERDATR));

    if (data->DataReceivedCallBack)
        data->DataReceivedCallBack(&c, 1, data->unitnum, data->DataReceivedUserData);

    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * SERDAT has drained. Ask the upper layer for the next byte; it answers by
 * calling Write() below. Debug output raises this too, in which case there is
 * usually no pending write and the callback does nothing.
 */
static AROS_INTH1(serial_tbe_interrupt, struct HIDDSerialUnitData *, data)
{
    AROS_INTFUNC_INIT

    if (data->DataWriteCallBack)
        data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);

    return FALSE;

    AROS_INTFUNC_EXIT
}

/******* SerialUnit::Init() ************************************************/
BOOL AmigaSerUnit__Hidd_SerialUnit__Init(OOP_Class *cl, OOP_Object *o,
                                         struct pHidd_SerialUnit_Init *msg)
{
    struct HIDDSerialUnitData *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("SerialUnit::Init()\n"));

    Disable();
    data->DataReceivedCallBack = msg->DataReceived;
    data->DataReceivedUserData = msg->DataReceivedUserData;
    data->DataWriteCallBack    = msg->WriteData;
    data->DataWriteUserData    = msg->WriteDataUserData;
    Enable();

    if (!data->intsadded)
    {
        data->rbfint.is_Node.ln_Pri = 0;
        data->rbfint.is_Node.ln_Type = NT_INTERRUPT;
        data->rbfint.is_Node.ln_Name = "ser rx";
        data->rbfint.is_Code = (VOID_FUNC)serial_rbf_interrupt;
        data->rbfint.is_Data = data;

        data->tbeint.is_Node.ln_Pri = 0;
        data->tbeint.is_Node.ln_Type = NT_INTERRUPT;
        data->tbeint.is_Node.ln_Name = "ser tx";
        data->tbeint.is_Code = (VOID_FUNC)serial_tbe_interrupt;
        data->tbeint.is_Data = data;

        /* Unmasks the bit for us, and RemIntServer() masks it again once
           the chain empties. */
        AddIntServer(INTB_RBF, &data->rbfint);
        AddIntServer(INTB_TBE, &data->tbeint);
        data->intsadded = TRUE;
    }

    ReturnBool("SerialUnit::Init()", TRUE);
}

/******* SerialUnit::Write() ***********************************************/
ULONG AmigaSerUnit__Hidd_SerialUnit__Write(OOP_Class *cl, OOP_Object *o,
                                           struct pHidd_SerialUnit_Write *msg)
{
    struct HIDDSerialUnitData *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("SerialUnit::Write()\n"));

    /* If the output is currently stopped just don't do anything here. */
    if (TRUE == data->stopped || msg->Length < 1)
        return 0;

    /*
     * Normally TBE is already set, since this runs either from the interrupt
     * that reported the drain or with the line idle. It is only not set when
     * debug output slipped a character in first, and then the wait is bounded
     * by one character time. Returning 0 here instead would end the write: the
     * byte that would have raised the next TBE never gets sent.
     */
    while ((custom_r(SERDATR) & SERDATR_TBE) == 0)
        ;

    custom_w(SERDAT, SERDAT_STP8 | SERDAT_DB8(msg->Outbuffer[0]));

    ReturnInt("SerialUnit::Write()", ULONG, 1);
}

/***************************************************************************/

/*
 * Anything the 15-bit SERPER divisor can express. The bottom end is where the
 * divisor saturates; the top is Paula's documented ceiling.
 */
static ULONG valid_baudrates[] =
{
    (SERPER_BASE_PAL / (SERPER_DIVISOR_MAX + 1)) | LIMIT_LOWER_BOUND,
    115200 | LIMIT_UPPER_BOUND,
    ~0
};

static ULONG valid_datalengths[] =
{
    8,
    ~0
};

/******* SerialUnit::SetBaudrate() *****************************************/
BOOL AmigaSerUnit__Hidd_SerialUnit__SetBaudrate(OOP_Class *cl, OOP_Object *o,
                                                struct pHidd_SerialUnit_SetBaudrate *msg)
{
    struct HIDDSerialUnitData *data = OOP_INST_DATA(cl, o);
    ULONG divisor;

    EnterFunc(bug("SerialUnit::SetBaudrate()\n"));

    if (msg->baudrate == 0)
        ReturnBool("SerialUnit::SetBaudrate()", FALSE);

    divisor = SERPER_BAUD(SERPER_BASE_PAL, msg->baudrate);

    if (divisor > SERPER_DIVISOR_MAX)
        ReturnBool("SerialUnit::SetBaudrate()", FALSE);

    /*
     * SERPER is the whole of Paula's line configuration and it is shared with
     * the kernel's debug output, which never reprograms it. Anything but the
     * rate the bootstrap chose therefore rebauds the debug stream as well.
     */
    custom_w(SERPER, (UWORD)divisor);
    data->baudrate = msg->baudrate;

    ReturnBool("SerialUnit::SetBaudrate()", TRUE);
}

/******* SerialUnit::SetParameters() ***************************************/
BOOL AmigaSerUnit__Hidd_SerialUnit__SetParameters(OOP_Class *cl, OOP_Object *o,
                                                  struct pHidd_SerialUnit_SetParameters *msg)
{
    struct TagItem *tag, *tstate = msg->tags;

    EnterFunc(bug("SerialUnit::SetParameters()\n"));

    /*
     * 8N1 only. Paula can do 7 or 9 data bits and 2 stop bits, but nothing
     * here needs them yet.
     */
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case TAG_DATALENGTH:
                if (tag->ti_Data != 8)
                    ReturnBool("SerialUnit::SetParameters()", FALSE);
            break;

            case TAG_STOP_BITS:
                if (tag->ti_Data != 1)
                    ReturnBool("SerialUnit::SetParameters()", FALSE);
            break;

            case TAG_PARITY:
                ReturnBool("SerialUnit::SetParameters()", FALSE);
            break;
        }
    }

    ReturnBool("SerialUnit::SetParameters()", TRUE);
}

/******* SerialUnit::SendBreak() *******************************************/
BYTE AmigaSerUnit__Hidd_SerialUnit__SendBreak(OOP_Class *cl, OOP_Object *o,
                                              struct pHidd_SerialUnit_SendBreak *msg)
{
    EnterFunc(bug("SerialUnit::SendBreak()\n"));

    return SerErr_LineErr;
}

/******* SerialUnit::Start() ***********************************************/
VOID AmigaSerUnit__Hidd_SerialUnit__Start(OOP_Class *cl, OOP_Object *o,
                                          struct pHidd_SerialUnit_Start *msg)
{
    struct HIDDSerialUnitData *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("SerialUnit::Start()\n"));

    /*
     * Allow output again and pull whatever queued up while stopped. The flag
     * has to be cleared first or the Write() that callback drives would refuse
     * the data all over again.
     */
    if (TRUE == data->stopped)
    {
        data->stopped = FALSE;

        if (NULL != data->DataWriteCallBack)
            data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);
    }
}

/******* SerialUnit::Stop() ************************************************/
VOID AmigaSerUnit__Hidd_SerialUnit__Stop(OOP_Class *cl, OOP_Object *o,
                                         struct pHidd_SerialUnit_Stop *msg)
{
    struct HIDDSerialUnitData *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("SerialUnit::Stop()\n"));

    data->stopped = TRUE;
}

/****** SerialUnit::GetCapabilities ****************************************/
VOID AmigaSerUnit__Hidd_SerialUnit__GetCapabilities(OOP_Class *cl, OOP_Object *o,
                                                    struct TagItem *tags)
{
    if (NULL != tags)
    {
        int i = 0;
        BOOL end = FALSE;

        while (FALSE == end)
        {
            switch (tags[i].ti_Tag)
            {
                case HIDDA_SerialUnit_BPSRate:
                    tags[i].ti_Data = (STACKIPTR)valid_baudrates;
                break;

                case HIDDA_SerialUnit_DataLength:
                    tags[i].ti_Data = (STACKIPTR)valid_datalengths;
                break;

                case TAG_DONE:
                    end = TRUE;
                break;
            }
            i++;
        }
    }
}

/****** SerialUnit::GetStatus **********************************************/
UWORD AmigaSerUnit__Hidd_SerialUnit__GetStatus(OOP_Class *cl, OOP_Object *o,
                                               struct pHidd_SerialUnit_GetStatus *msg)
{
    /*
     * CIA-B port A carries DSR, CTS, CD and DTR on bits 3, 4, 5 and 7, which
     * is where io_Status wants them; they are just active low.
     */
    return (UWORD)((~(*CIAB_PRA)) & CIAB_PRA_HANDSHAKE);
}

/******* init_serialunitclass **********************************************/

static int AmigaSerUnit_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;

    __IHidd_SerialUnitAB = OOP_ObtainAttrBase(IID_Hidd_SerialUnit);

    ReturnInt("AmigaSerUnit_Init", int, __IHidd_SerialUnitAB != 0);
}

static int AmigaSerUnit_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd;

    OOP_ReleaseAttrBase(IID_Hidd_SerialUnit);
    __IHidd_SerialUnitAB = 0;

    ReturnInt("AmigaSerUnit_Expunge", int, TRUE);
}

ADD2INITLIB(AmigaSerUnit_Init, 0)
ADD2EXPUNGELIB(AmigaSerUnit_Expunge, 0)
