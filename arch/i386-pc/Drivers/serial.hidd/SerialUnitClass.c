/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Serial Unit hidd class implementation.
    Lang: english
*/

/*
  Right now I am assuming that there is a 1.8432 MHZ crystal connected to
  the 16550 UART.
*/

#define AROS_ALMOST_COMPATIBLE 1

/* the rest are Amiga includes */
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/alib.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/interrupts.h>
#include <exec/lists.h>

#include <utility/tagitem.h>
#include <hidd/serial.h>
#include <hidd/unixio.h>

#include <devices/serial.h>

#include "serial_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

/* The speed of the crystal */
#define CRYSTAL_SPEED 	1842000 


void serialunit_receive_data();
void serialunit_write_more_data();

unsigned char get_lcr(struct HIDDSerialUnitData * data);
unsigned char get_fcr(ULONG baudrate);
BOOL set_baudrate(struct HIDDSerialUnitData * data, ULONG speed);


inline void outb(unsigned char value, unsigned short port)
{
  __asm__ __volatile__ ("outb %b0,%w1" : : "a" (value), "Nd"(port));
}

inline void outb_p(unsigned char value, unsigned short port)
{
  __asm__ __volatile__ ("outb %b0,%w1 \noutb %%al,$0x80" : : "a" (value), "Nd" (port));
}

inline unsigned char inb(unsigned short port)
{
  unsigned char _v;
  __asm__ __volatile__ ("inb %w1,%b0" : "=a" (_v) : "Nd" (port) );
  return _v;
}

inline unsigned char inb_p(unsigned short port)
{
  unsigned char _v;
  __asm__ __volatile__ ("inb %w1,%b0 \noutb %%al,$0x80" : "=a" (_v) : "Nd" (port) );
  return _v;
}

static inline void serial_out(struct HIDDSerialUnitData * data, 
                              int offset, 
                              int value)
{
  outb(value, data->baseaddr+offset);
}

static inline void serial_outp(struct HIDDSerialUnitData * data, 
                               int offset, 
                               int value)
{
  outb_p(value, data->baseaddr+offset);
}

static inline unsigned int serial_in(struct HIDDSerialUnitData * data,
                                     int offset)
{
  return inb(data->baseaddr+offset);
}

static inline unsigned int serial_inp(struct HIDDSerialUnitData * data,
                                      int offset)
{
  return inb_p(data->baseaddr+offset);
}



/*************************** Classes *****************************/

/******* SerialUnit::New() ***********************************/
static Object *serialunit_new(Class *cl, Object *obj, ULONG *msg)
{
  struct HIDDSerialUnitData * data;
  
  EnterFunc(bug("SerialUnit::New()\n"));

  obj = (Object *)DoSuperMethod(cl, obj, (Msg)msg);

  if (obj)
  {
    data = INST_DATA(cl, obj);
    
    data->baseaddr = 0x3fb; //?????????????
    
    data->datalength = 8;
    data->parity     = FALSE;
    data->baudrate   = 0; /* will be initialize in set_baudrate() */
    
    /* clear the FIFOs */
    serial_outp(data, UART_FCR, (UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT));
    
    /* clear the interrupt registers */
    (void)serial_inp(data, UART_RX);
    (void)serial_inp(data, UART_IIR);
    (void)serial_inp(data, UART_MSR);

    /* initilize the UART */
    serial_outp(data, UART_LCR, get_lcr(data));
     
    serial_outp(data, UART_MCR, 0);
    serial_outp(data, UART_IER, UART_IER_MSI | UART_IER_RLSI | UART_IER_RDI);
     
    /* clear the interrupt registers again ... */
    (void)serial_inp(data, UART_LSR);
    (void)serial_inp(data, UART_RX);
    (void)serial_inp(data, UART_IIR);
    (void)serial_inp(data, UART_MSR);
     
     set_baudrate(data, SER_DEFAULT_BAUDRATE);
  } /* if (obj) */

  ReturnPtr("SerialUnit::New()", Object *, obj);
}

/******* SerialUnit::Dispose() ***********************************/
static Object *serialunit_dispose(Class *cl, Object *obj, struct pRoot_New *msg)
{
  struct HIDDSerialUnitData * data;
  EnterFunc(bug("SerialUnit::Dispose()\n"));

  data = INST_DATA(cl, obj);

  /* stop all interrupts */
  serial_outp(data, UART_IER, 0);  

  DoSuperMethod(cl, obj, (Msg)msg);
  ReturnPtr("SerialUnit::Dispose()", Object *, obj);
}



/******* SerialUnit::Init() **********************************/
BOOL serialunit_init(Class *cl, Object *o, struct pHidd_SerialUnit_Init *msg)
{
  struct HIDDSerialUnitData * data = INST_DATA(cl, o);
  
  EnterFunc(bug("SerialUnit::Init()\n"));
  data->DataReceivedCallBack = msg->DataReceived;
  data->DataWriteCallBack    = msg->WriteData;

  ReturnBool("SerialUnit::Init()", TRUE);
}

/******* SerialUnit::Write() **********************************/
ULONG serialunit_write(Class *cl, Object *o, struct pHidd_SerialUnit_Write *msg)
{
  struct HIDDSerialUnitData * data = INST_DATA(cl, o);
  unsigned char status;
  ULONG len = msg->Length;
  ULONG count = 0;
  
  EnterFunc(bug("SerialUnit::Write()\n"));

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
  -1
};


/******* SerialUnit::SetBaudrate() **********************************/
BOOL serialunit_setbaudrate(Class *cl, Object *o, struct pHidd_SerialUnit_SetBaudrate *msg)
{
  struct HIDDSerialUnitData * data = INST_DATA(cl, o);
  BOOL valid = FALSE;
  
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
  -1
};

/******* SerialUnit::SetParameters() **********************************/
BOOL serialunit_setparameters(Class *cl, Object *o, struct pHidd_SerialUnit_SetParameters *msg)
{
  struct HIDDSerialUnitData * data = INST_DATA(cl, o);
  BOOL valid = TRUE;
  int i = 0;
  struct TagItem * tags = msg->tags;
  
  while (TAG_END != tags[i].ti_Tag && TRUE == valid)
  {
    switch (tags[i].ti_Tag)
    {
      case TAG_DATALENGTH:
        if (tags[i].ti_Data >= 5 && tags[i].ti_Data <= 8)
          data->datalength = tags[i].ti_Data;
        else
          valid = FALSE;
      break;
      
      case TAG_STOP_BITS:
        if (16 == tags[i].ti_Data ||
            32 == tags[i].ti_Data ||
            24 == tags[i].ti_Data)
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
      
      default:
        valid = FALSE;
    }
    i++;
  }
  
  serial_outp(data, UART_LCR, get_lcr(data));

  return valid;
}

/******* SerialUnit::SendBreak() **********************************/
BYTE serialunit_sendbreak(Class *cl, Object *o, struct pHidd_SerialUnit_SendBreak *msg)
{
  struct HIDDSerialUnitData * data = INST_DATA(cl, o);
  
  return SerErr_LineErr;
}

/****** SerialUnit::GetCapabilities ********************************/
VOID serialunit_getcapabilities(Class * cl, Object *o, struct TagItem * tags)
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

/************* The software interrupt handler that gets data from UART *****/


#undef OOPBase
#undef SysBase
#undef UtilityBase

#define READBUFFER_SIZE 513

AROS_UFH3(void, serialunit_receive_data,
   AROS_UFHA(APTR, iD, A1),
   AROS_UFHA(APTR, iC, A5),
   AROS_UFHA(struct ExecBase *, SysBase, A6))
{
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
    data->DataReceivedCallBack(buffer, len, data->unitnum);
}

AROS_UFH3(void, serialunit_write_more_data,
   AROS_UFHA(APTR, iD, A1),
   AROS_UFHA(APTR, iC, A5),
   AROS_UFHA(struct ExecBase *, SysBase, A6))
{
  struct HIDDSerialUnitData * data = iD;

  /*
  ** Ask for more data be written to the unit
  */
  D(bug("Asking for more data to be written to unit %d\n",data->unitnum));

  if (NULL != data->DataWriteCallBack)
    data->DataWriteCallBack(data->unitnum);
}


/******* init_serialunitclass ********************************/

#define SysBase     (csd->sysbase)
#define OOPBase     (csd->oopbase)
#define UtilityBase (csd->utilitybase)


#define NUM_ROOT_METHODS 2
#define NUM_SERIALUNIT_METHODS moHidd_SerialUnit_NumMethods

Class *init_serialunitclass (struct class_static_data *csd)
{
    Class *cl = NULL;
    
    struct MethodDescr serialunithiddroot_descr[NUM_ROOT_METHODS + 1] = 
    {
        {(IPTR (*)())serialunit_new,		moRoot_New},
        {(IPTR (*)())serialunit_dispose,	moRoot_Dispose},
/*
        {(IPTR (*)())serialunit_set,		moRoot_Set},
        {(IPTR (*)())serialunit_get,		moRoot_Get},
*/
        {NULL, 0UL}
    };
    
    struct MethodDescr serialunithidd_descr[NUM_SERIALUNIT_METHODS + 1] =
    {
        {(IPTR (*)())serialunit_init,		moHidd_SerialUnit_Init},
        {(IPTR (*)())serialunit_write,		moHidd_SerialUnit_Write},
        {(IPTR (*)())serialunit_setbaudrate,	moHidd_SerialUnit_SetBaudrate},
        {(IPTR (*)())serialunit_setparameters,	moHidd_SerialUnit_SetParameters},
        {(IPTR (*)())serialunit_sendbreak,	moHidd_SerialUnit_SendBreak},
        {(IPTR (*)())serialunit_getcapabilities,moHidd_SerialUnit_GetCapabilities},
        {NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
        {serialunithiddroot_descr	, IID_Root		, NUM_ROOT_METHODS},
        {serialunithidd_descr		, IID_Hidd_SerialUnit	, NUM_SERIALUNIT_METHODS},
        {NULL, NULL, 0}
    };

    AttrBase MetaAttrBase = GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
        { aMeta_SuperID,                (IPTR)CLID_Root},
        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_SerialUnit},
        { aMeta_InstSize,               (IPTR)sizeof (struct HIDDSerialUnitData) },
        {TAG_DONE, 0UL}
    };


    EnterFunc(bug("    init_serialunitclass(csd=%p)\n", csd));

    cl = NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
    if(cl)
    {
        D(bug("SerialUnit Class ok\n"));
        cl->UserData = (APTR)csd;

        AddClass(cl);
    }

    ReturnPtr("init_serialunitclass", Class *, cl);
}


void free_serialunitclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_serialhiddclass(csd=%p)\n", csd));

    if(csd)
    {
        RemoveClass(csd->serialhiddclass);
	
        if(csd->serialhiddclass) DisposeObject((Object *) csd->serialhiddclass);
        csd->serialhiddclass = NULL;
    }

    ReturnVoid("free_serialhiddclass");
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
  
    default: lcr = 0;
  }
  
  switch (data->stopbits)
  {
    case 16: /* 1 stopbit */
      /* nothing to do */
    break;
    
    case 24: /* 1.5 stopbits */
      if (data->datalength == 5)
        lcr |= (1 << 2);
    break;
    
    case 32: /* 2 stopbits */
      if (data->datalength >= 6 && data->datalength <= 8)
        lcr |= (1 << 2);
    break;
    
    default:
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
    serial_outp(data, UART_FCR, get_fcr(speed));
    
    return TRUE;
}
