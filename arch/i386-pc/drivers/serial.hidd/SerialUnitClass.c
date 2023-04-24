/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

    Desc: Serial Unit hidd class implementation.
*/

/*
  Right now I am assuming that there is a 1.8432 MHZ crystal connected to
  the 16550 UART.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/alib.h>
#include <proto/intuition.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/lists.h>

#include <utility/tagitem.h>
#include <hidd/serial.h>
#include <hidd/unixio.h>
#include <intuition/preferences.h>

#include <devices/serial.h>

#include "serial_intern.h"

#define DIRQ(x)

/* The speed of the crystal */
#define CRYSTAL_SPEED   1843200

AROS_INTP(serial_int_13);
AROS_INTP(serial_int_24);

void serialunit_receive_data();
ULONG serialunit_write_more_data();

unsigned char get_lcr(struct HIDDSerialUnitData * data);
unsigned char get_fcr(ULONG baudrate);
BOOL set_baudrate(struct HIDDSerialUnitData * data, ULONG speed);
static void adapt_data(struct HIDDSerialUnitData * data,
                       struct Preferences * prefs);

/*************************** Classes *****************************/

/* IO bases for every COM port */
static const ULONG bases[] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };

/******* Shutdown Handler ***********************************/
static AROS_INTH1(PCSerUnit_ResetHandler, struct HIDDSerialUnitData *, data)
{
    AROS_INTFUNC_INIT

    DIRQ(bug("[Serial:Unit:PC] %s()\n", __func__));

    serial_outp(data, UART_IER, 0);

    /* clear the interrupt registers ... */
    (void)serial_inp(data, UART_LSR);
    (void)serial_inp(data, UART_RX);
    (void)serial_inp(data, UART_IIR);
    (void)serial_inp(data, UART_MSR);

    serial_outp(data, UART_MCR, 0);

    DIRQ(bug("[Serial:Unit:PC] %s: done\n", __func__));

    return FALSE;

    AROS_INTFUNC_EXIT
}

/******* SerialUnit::New() ***********************************/
OOP_Object *PCSerUnit__Root__New(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
  struct class_static_data *csd = (&((struct IntHIDDSerialBase *)cl->UserData)->hdg_csd);
  struct pRoot_New serunitNew;
  struct TagItem serunitTags[] =
  {
      { aHidd_Name,            (IPTR)"serial.hidd"                 },
      { aHidd_HardwareName,    (IPTR)"PC 16550 UART Serial-Port"   },
      { TAG_DONE,              0                                   }
  };
  struct HIDDSerialUnitData * data;
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

  } /* while (tags to process) */

  if (msg->attrList)
  {
      serunitTags[1].ti_Tag  = TAG_MORE;
      serunitTags[1].ti_Data = (IPTR)msg->attrList;
  }
  serunitNew.mID      = msg->mID;
  serunitNew.attrList = serunitTags;

  obj = (OOP_Object *)OOP_DoSuperMethod(cl, obj, (OOP_Msg)&serunitNew);

  if (obj)
  {
    data = OOP_INST_DATA(cl, obj);

    if (unitnum < 5 )
    {
        struct Interrupt *irq;
        if ((unitnum == 1) || (unitnum == 3))
        {
            D(bug("[Serial:PC] %s: Adding IRQ handlers for COM2/4\n", __func__));
            /* Install COM2 and COM4 interrupt */
            irq = &CSD(cl->UserData)->intHandler[1];
            if (!irq->is_Node.ln_Name)
            {
                irq->is_Node.ln_Name = "COM2/COM4";
                irq->is_Code = (VOID_FUNC)serial_int_24;
                irq->is_Node.ln_Type = NT_INTERRUPT;
                irq->is_Node.ln_Pri = 127;            /* Set the highest pri */
                irq->is_Data = (APTR)CSD(cl->UserData);
                AddIntServer(INTB_KERNEL + 3, irq);
                D(bug("[Serial:PC] %s: Interrupt handler installed\n", __func__));

                /* Install reset callback */
                data->unitsdh.is_Node.ln_Pri = -61;
                data->unitsdh.is_Node.ln_Name = ((struct Node *)obj)->ln_Name;
                data->unitsdh.is_Code = (VOID_FUNC)PCSerUnit_ResetHandler;
                data->unitsdh.is_Data = data;
                AddResetCallback(&data->unitsdh);
                D(bug("[Serial:PC] %s: Shutdown handler installed\n", __func__));
            }
        }
        else
        {
            D(bug("[Serial:PC] %s: Adding IRQ handlers for COM1/3\n", __func__));
            /* Install COM1 and COM3 interrupt */
            irq = &CSD(cl->UserData)->intHandler[0];
            if (!irq->is_Node.ln_Name)
            {
                irq->is_Node.ln_Name = "COM1/COM3";
                irq->is_Code = (VOID_FUNC)serial_int_13;
                irq->is_Node.ln_Type = NT_INTERRUPT;
                irq->is_Node.ln_Pri = 127;            /* Set the highest pri */
                irq->is_Data = (APTR)CSD(cl->UserData);
                AddIntServer(INTB_KERNEL + 4, irq);
                D(bug("[Serial:PC] %s: Interrupt handler installed\n", __func__));

                /* Install reset callback */
                data->unitsdh.is_Node.ln_Pri = -61;
                data->unitsdh.is_Node.ln_Name = ((struct Node *)obj)->ln_Name;
                data->unitsdh.is_Code = (VOID_FUNC)PCSerUnit_ResetHandler;
                data->unitsdh.is_Data = data;
                AddResetCallback(&data->unitsdh);
                D(bug("[Serial:PC] %s: Shutdown handler installed\n", __func__));
            }
        }
    }
    data->baseaddr = bases[unitnum];
    data->unitnum    = unitnum;

    D(bug("[Serial:PC] %s: Base Address %08x\n", __func__, data->baseaddr));

    struct IntuitionBase * IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",0);
    if (NULL != IntuitionBase) {
      struct Preferences prefs;
      GetPrefs(&prefs,sizeof(prefs));
      data->baudrate      = prefs.BaudRate;
      adapt_data(data, &prefs);
      CloseLibrary((struct Library *)IntuitionBase);
    } else {
      data->datalength = 8;
      data->parity     = FALSE;
      data->baudrate   = 9600; /* will be initialize in set_baudrate() */
    }

    Disable();
    CSD(cl->UserData)->units[data->unitnum] = data;
    Enable();

    D(bug("[Serial:PC] %s: Unit %d at 0x0%x\n", __func__, data->unitnum, data->baseaddr));

    /* Wake up UART */
    serial_outp(data, UART_LCR, 0xBF);
    serial_outp(data, UART_EFR, UART_EFR_ECB);
    serial_outp(data, UART_IER, 0);
    serial_outp(data, UART_EFR, 0);
    serial_outp(data, UART_LCR, 0);

    /* clear the FIFOs */
    serial_outp(data, UART_FCR, (UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT));
    
    /* clear the interrupt registers */
    (void)serial_inp(data, UART_RX);
    (void)serial_inp(data, UART_IIR);
    (void)serial_inp(data, UART_MSR);

    /* initilize the UART */
    serial_outp(data, UART_LCR, get_lcr(data));
     
    serial_outp(data, UART_MCR, UART_MCR_OUT2 | UART_MCR_DTR | UART_MCR_RTS);
    serial_outp(data, UART_IER, UART_IER_RDI | UART_IER_THRI | UART_IER_RLSI | UART_IER_MSI);
     
    /* clear the interrupt registers again ... */
    (void)serial_inp(data, UART_LSR);
    (void)serial_inp(data, UART_RX);
    (void)serial_inp(data, UART_IIR);
    (void)serial_inp(data, UART_MSR);
     
    set_baudrate(data, data->baudrate);
  } /* if (obj) */

  ReturnPtr("SerialUnit::New()", OOP_Object *, obj);
}

/******* SerialUnit::Dispose() ***********************************/
OOP_Object *PCSerUnit__Root__Dispose(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
  struct HIDDSerialUnitData * data;
  EnterFunc(bug("SerialUnit::Dispose()\n"));

  data = OOP_INST_DATA(cl, obj);

  if (data->unitsdh.is_Node.ln_Name)
  {
    struct Interrupt *irq;
    int irqnum;
    RemResetCallback(&data->unitsdh);
    if ((data->unitnum == 1) || (data->unitnum == 3))
    {
      irq = &CSD(cl->UserData)->intHandler[1];
      irqnum = 3;
    }
    else
    {
      irq = &CSD(cl->UserData)->intHandler[0];
      irqnum = 4;
    }
    RemIntServer(INTB_KERNEL + irqnum, irq);
  }

  Disable();
  CSD(cl->UserData)->units[data->unitnum] = NULL;
  Enable();

  /* stop all interrupts */
  serial_outp(data, UART_IER, 0);

  OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
  ReturnPtr("SerialUnit::Dispose()", OOP_Object *, obj);
}



/******* SerialUnit::Init() **********************************/
BOOL PCSerUnit__Hidd_SerialUnit__Init(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Init *msg)
{
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  
  EnterFunc(bug("SerialUnit::Init()\n"));
  Disable();
  data->DataReceivedCallBack = msg->DataReceived;
  data->DataReceivedUserData = msg->DataReceivedUserData;
  data->DataWriteCallBack    = msg->WriteData;
  data->DataWriteUserData    = msg->WriteDataUserData;
  Enable();

  ReturnBool("SerialUnit::Init()", TRUE);
}

/******* SerialUnit::Write() **********************************/
ULONG PCSerUnit__Hidd_SerialUnit__Write(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Write *msg)
{
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  unsigned char status;
  ULONG len = msg->Length;
  ULONG count = 0;
  
  EnterFunc(bug("SerialUnit::Write()\n"));

  /*
   * If the output is currently stopped just don't do anything here.
   */
  if (TRUE == data->stopped)
    return 0;

  status = serial_inp(data, UART_LSR);
  
  if (status & UART_LSR_THRE)
  {
    /* write data into FIFO */
    do
    {
      serial_outp(data, UART_TX, msg->Outbuffer[count++]);
      len--;
    } while (len > 0 && serial_inp(data, UART_LSR & UART_LSR_TEMT));
  }

  ReturnInt("SerialUnit::Write()",ULONG, count);
}

/***************************************************************************/

static ULONG valid_baudrates[] =
{
  2 | LIMIT_LOWER_BOUND,
  115200 | LIMIT_UPPER_BOUND,
  ~0
};


/******* SerialUnit::SetBaudrate() **********************************/
BOOL PCSerUnit__Hidd_SerialUnit__SetBaudrate(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_SetBaudrate *msg)
{
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  BOOL valid = FALSE;
  
  EnterFunc(bug("SerialUnit::SetBaudrate()\n"));

  if (msg->baudrate != data->baudrate)
  {
    valid = set_baudrate(data, msg->baudrate);
  } /* if */
  return valid;
}

static UBYTE valid_datalengths[] =
{
  5,
  6,
  7,
  8,
  ~0
};

/******* SerialUnit::SetParameters() **********************************/
BOOL PCSerUnit__Hidd_SerialUnit__SetParameters(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_SetParameters *msg)
{
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  BOOL valid = TRUE;
  int i = 0;
  struct TagItem * tags = msg->tags;

  EnterFunc(bug("SerialUnit::SetParameters()\n"));

  while (TAG_END != tags[i].ti_Tag && TRUE == valid)
  {
    switch (tags[i].ti_Tag)
    {
      case TAG_DATALENGTH:
        if ((BYTE)tags[i].ti_Data >= 5 && (BYTE)tags[i].ti_Data <= 8)
          data->datalength = tags[i].ti_Data;
        else
          valid = FALSE;
      break;
      
      case TAG_STOP_BITS: /* 3 means 1.5 stopbits (if supported) */
        if (1 == tags[i].ti_Data ||
            2 == tags[i].ti_Data ||
            3 == tags[i].ti_Data)
          data->stopbits = tags[i].ti_Data;
        else
          valid = FALSE;
      break;

      case TAG_PARITY:
        if (PARITY_0    == tags[i].ti_Data ||
            PARITY_1    == tags[i].ti_Data ||
            PARITY_EVEN == tags[i].ti_Data ||
            PARITY_ODD  == tags[i].ti_Data)
        {
          data->parity     = TRUE;
          data->paritytype = tags[i].ti_Data;
        }
        else
          valid = FALSE;
      break;

      case TAG_PARITY_OFF:
        data->parity = FALSE;
      break;

      case TAG_SET_MCR:
        serial_outp(data, UART_MCR, (tags[i].ti_Data & 0x0f) | 0x08);
      break;
      
      case TAG_SKIP:
      case TAG_IGNORE:
      break;
      
      default:
        valid = FALSE;
    }
    i++;
  }
  
  if (TRUE == valid)
    serial_outp(data, UART_LCR, get_lcr(data));

  return valid;
}

/******* SerialUnit::SendBreak() **********************************/
BYTE PCSerUnit__Hidd_SerialUnit__SendBreak(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_SendBreak *msg)
{

  EnterFunc(bug("SerialUnit::SendBreak()\n"));

  return SerErr_LineErr;
}

/******* SerialUnit::Start() **********************************/
VOID PCSerUnit__Hidd_SerialUnit__Start(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Start *msg)
{
    struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("SerialUnit::Start()\n"));
    /*
     * Allow or start feeding the UART with data. Get the data
     * from upper layer.
     */
    if (TRUE == data->stopped) {
            if (NULL != data->DataWriteCallBack)
                    data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);
            /*
             * Also mark the stopped flag as FALSE.
             */
            data->stopped = FALSE;
    }
}

/******* SerialUnit::Stop() **********************************/
VOID PCSerUnit__Hidd_SerialUnit__Stop(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Stop *msg)
{
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  
  /*
   * The next time the interrupt comes along and asks for
   * more data we just don't do anything...
   */
  data->stopped = TRUE;
}

/****** SerialUnit::GetCapabilities ********************************/
VOID PCSerUnit__Hidd_SerialUnit__GetCapabilities(OOP_Class * cl, OOP_Object *o, struct TagItem * tags)
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

/****** SerialUnit::GetStatus ********************************/
UWORD PCSerUnit__Hidd_SerialUnit__GetStatus(OOP_Class * cl, OOP_Object *o, struct pHidd_SerialUnit_GetStatus *msg)
{
        struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
        UWORD status = 0;
        UBYTE msr = serial_inp(data, UART_MSR);
        UBYTE mcr = serial_inp(data, UART_MCR);

        if (msr & UART_MSR_DCD)
                status |= (1<<5);
        if (msr & UART_MSR_DSR)
                status |= (1<<3);
        if (msr & UART_MSR_CTS)
                status |= (1<<4);

        if (mcr & UART_MCR_DTR)
                status |= (1<<7);
        if (mcr & UART_MCR_RTS)
                status |= (1<<6);  /* old RKMs say 'ready to send' */
        return status;
}

/************* The software interrupt handler that gets data from UART *****/


#define READBUFFER_SIZE 513

AROS_UFH3(void, serialunit_receive_data,
   AROS_UFHA(APTR, iD, A1),
   AROS_UFHA(APTR, iC, A5),
   AROS_UFHA(struct ExecBase *, SysBase, A6))
{
  AROS_USERFUNC_INIT

  struct HIDDSerialUnitData * data = iD;
  int len = 0;
  UBYTE buffer[READBUFFER_SIZE];

  /*
  ** Read the data from the port ...
  */
  do
  {
    buffer[len++] = serial_inp(data, UART_RX);
  }
  while (serial_inp(data, UART_LSR) & UART_LSR_DR);
  
  /*
  ** ... and deliver them to whoever is interested.
  */

  if (NULL != data->DataReceivedCallBack)
    data->DataReceivedCallBack(buffer, len, data->unitnum, data->DataReceivedUserData);

  return;

  AROS_USERFUNC_EXIT
}

AROS_UFH3(ULONG, serialunit_write_more_data,
   AROS_UFHA(APTR, iD, A1),
   AROS_UFHA(APTR, iC, A5),
   AROS_UFHA(struct ExecBase *, SysBase, A6))
{
  AROS_USERFUNC_INIT

  ULONG bytes = 0;
  struct HIDDSerialUnitData * data = iD;

  /*
   * If the output is currently stopped just don't do
   * anything here.
   */
  if (TRUE == data->stopped)
    return 0;
    
  /*
  ** Ask for more data be written to the unit
  */
  D(bug("Asking for more data to be written to unit %d\n",data->unitnum));

  if (NULL != data->DataWriteCallBack)
    bytes = data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);
  return bytes;

  AROS_USERFUNC_EXIT
}


/* some help routines */

unsigned char get_lcr(struct HIDDSerialUnitData * data)
{
  char lcr;
  switch (data->datalength)
  {
    case 5: lcr = 0;
    break;
    
    case 6: lcr = 1;
    break;
    
    case 7: lcr = 2;
    break;
    
    case 8: lcr = 3;
    break;
  
    default: lcr = 3;
  }
  
  switch (data->stopbits)
  {
    case 1: /* 1 stopbit */
      /* nothing to do */
    break;
    
    case 3: /* 1.5 stopbits (is this supported ?!!!) */
      if (data->datalength == 5)
        lcr |= (1 << 2);
    break;
    
    case 2: /* 2 stopbits */
      if (data->datalength >= 6 && data->datalength <= 8)
        lcr |= (1 << 2);
    break;
    
    default:
    break;
  }
  
  if (TRUE == data->parity)
  {
    lcr |= (1 << 3);
  
    switch (data->paritytype)
    {
      case PARITY_EVEN:
        lcr |= (1 << 4);
      break;
      
      case PARITY_1:
        lcr |= (1 << 5);
      break;
      
      case PARITY_0:
        lcr |= (1 << 4) | (1 << 5);
      break;
    
    }
  }
  
  if (TRUE == data->breakcontrol)
    lcr |= (1 << 6);
  return lcr;
}

unsigned char get_fcr(ULONG baudrate)
{
  unsigned char fcr;
  fcr = (1 << 0);
  
  /*
    Depending on the baudrate set the fifo interrupt threshold to a
    different value.
  */
   
  if (baudrate < 1200)
    fcr |= (3 << 6);
  else
  if (baudrate < 9600)
    fcr |= (2 << 6);
  else
  if (baudrate < 38400)
    fcr |= (1 << 6);
  
  return fcr;
}

BOOL set_baudrate(struct HIDDSerialUnitData * data, ULONG speed)
{
    int quot;
    
    if (!(speed >= 50 && speed <= 115200))
      return FALSE;
    
    quot = CRYSTAL_SPEED / (speed << 4);
    
    /* set the speed on the UART now */
    serial_outp(data, UART_LCR, get_lcr(data) | UART_LCR_DLAB);
    serial_outp(data, UART_DLL, quot & 0xff);
    serial_outp(data, UART_DLM, quot >> 8);
    serial_outp(data, UART_LCR, get_lcr(data));
    serial_outp(data, UART_FCR, get_fcr(speed) | UART_FCR_ENABLE_FIFO);
    
    return TRUE;
}

/* Serial interrupts */

static void common_serial_int_handler(struct class_static_data *csd,
                                      ULONG unitnum)
{
        UBYTE code = UART_IIR_NO_INT;

        if (csd->units[unitnum])
                code = serial_inp(csd->units[unitnum], UART_IIR) & 0x07;

        switch (code)
        {
                case UART_IIR_RLSI:
                        (void)serial_inp(csd->units[unitnum], UART_LSR);
                break;

                case UART_IIR_RDI:
                        if (csd->units[unitnum]) {
                                AROS_UFC3(void, serialunit_receive_data,
                                  AROS_UFCA(APTR               , csd->units[unitnum], A1),
                                  AROS_UFCA(APTR               , NULL   , A5),
                                  AROS_UFCA(struct ExecBase *  , SysBase, A6));
                        }
                break;

                case UART_IIR_MSI:
                        (void)serial_inp(csd->units[unitnum], UART_MSR);
                break;

                case UART_IIR_THRI:
                        if (csd->units[unitnum])
                                if (0 == serialunit_write_more_data(csd->units[unitnum], NULL, SysBase))
                                        (void)serial_inp(csd->units[unitnum], UART_IIR);
                break;
        }
}

AROS_INTH1(serial_int_13, void *, data)
{
    AROS_INTFUNC_INIT

        common_serial_int_handler(data, 0);
        common_serial_int_handler(data, 2);

    return FALSE;

        AROS_INTFUNC_EXIT
}

AROS_INTH1(serial_int_24, void *, data)
{
    AROS_INTFUNC_INIT

        common_serial_int_handler(data, 1);
        common_serial_int_handler(data, 3);

    return FALSE;

        AROS_INTFUNC_EXIT
}

static void adapt_data(struct HIDDSerialUnitData * data,
                       struct Preferences * prefs)
{
        /*
         * Parity.
         */
        data->parity = TRUE;

        switch ((prefs->SerParShk >> 4) & 0x0f) {

                case SPARITY_NONE:
                default:             /* DEFAULT !! */
                        data->parity = FALSE;
                break;
                
                case SPARITY_EVEN:
                        data->paritytype = PARITY_EVEN;
                break;
                
                case SPARITY_ODD:
                        data->paritytype = PARITY_ODD;
                break;

                case SPARITY_MARK:
                        data->paritytype = PARITY_1;
                break;
                case SPARITY_SPACE:
                        data->paritytype = PARITY_0;
                break;

        }
        
        /*
         * Bit per character
         */
        switch ((prefs->SerRWBits & 0x0f)) {
                default: /* 8 bit */
                case 0:
                        data->datalength = 8;
                break;
                
                case 1: /* 7 bit */
                        data->datalength = 7;
                break;
                
                case 2: /* 6 bit */
                        data->datalength = 6;
                break;
                
                case 3: /* 5 bit */
                        data->datalength = 5;
                break;
        }

        /*
         * 2 stop bits ? default is '1'.
         */
        if (1 == (prefs->SerStopBuf >> 4))
                data->stopbits = 2;
        else
                data->stopbits = 1;
        
        /*
         * Handshake to be used.
         */
        // MISSING!
}
