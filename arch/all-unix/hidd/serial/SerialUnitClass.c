/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Serial Unit hidd class implementation.
    Lang: english
*/

/* Some POSIX includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>


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

#include "serial_intern.h"

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

void serialunit_receive_data();
void serialunit_write_more_data();

char * unitname[] =
{
 "/dev/ttyS0",
 "/dev/ttyS1",
 "/dev/ttyS2",
 "/dev/ttyS3"
 };

/*************************** Classes *****************************/

/******* SerialUnit::New() ***********************************/
static Object *serialunit_new(Class *cl, Object *obj, ULONG *msg)
{
  struct HIDDSerialUnitData * data;
  static const struct TagItem tags[] = {{ TAG_END, 0}};
  
  EnterFunc(bug("SerialUnit::New()\n"));
  D(bug("!!!!Request for unit number %d\n",*msg));

  obj = (Object *)DoSuperMethod(cl, obj, (Msg)msg);

  if (obj)
  {
    struct termios _termios;
    
    data = INST_DATA(cl, obj);
    
    data->unitnum = *msg;

    D(bug("Opening %s.\n",unitname[data->unitnum]));

    data->filedescriptor = open(unitname[data->unitnum], O_RDWR);
    D(bug("Opened %s on handle %d\n",unitname[data->unitnum], data->filedescriptor));
    
    if (-1 != data->filedescriptor)
    {
      /*
      ** Configure the tty driver
      */
      data->baudrate       = SER_DEFAULT_BAUDRATE;
      
      tcgetattr(data->filedescriptor, &data->orig_termios);
      tcgetattr(data->filedescriptor, &_termios); 
      cfmakeraw(&_termios);
      cfsetspeed(&_termios, data->baudrate);

      if (tcsetattr(data->filedescriptor, TCSANOW, &_termios) >=0)
      {
        data->replyport_read = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC|MEMF_CLEAR);
        data->replyport_write= AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC|MEMF_CLEAR);

        if (data->replyport_read && data->replyport_write)
        {
          /*
          ** Init the msg ports. They don't need a signal to be allocated
          */
          NEWLIST(&data->replyport_read->mp_MsgList);
          data->replyport_read ->mp_Node.ln_Type = NT_MSGPORT;

          NEWLIST(&data->replyport_write->mp_MsgList);
          data->replyport_write->mp_Node.ln_Type = NT_MSGPORT;

          data->softint_read  = AllocMem(sizeof(struct Interrupt), MEMF_CLEAR);
          data->softint_write = AllocMem(sizeof(struct Interrupt), MEMF_CLEAR);

          if (data->softint_read && data->softint_write)
          {
            data->softint_read->is_Data = data;
            data->softint_read->is_Code = serialunit_receive_data;

            data->softint_write->is_Data = data;
            data->softint_write->is_Code = serialunit_write_more_data;

            data->replyport_read->mp_SigBit = PA_SOFTINT;
            data->replyport_read->mp_SigTask = data->softint_read;

            data->replyport_write->mp_SigBit = PA_SOFTINT;
            data->replyport_write->mp_SigTask = data->softint_write;

            data->unixio_read  = NewObject(NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);
            data->unixio_write = NewObject(NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);

            if (NULL != data->unixio_read && NULL != data->unixio_write)
            {
              ULONG error;
              D(bug("Creating UnixIO AsyncIO command!\n"));

              error = Hidd_UnixIO_AsyncIO(data->unixio_read,
                                          data->filedescriptor,
                                          data->replyport_read,
                                          vHidd_UnixIO_Read);
              goto exit;

            }

            if (NULL != data->unixio_read)
              DisposeObject(data->unixio_read);

            if (NULL != data->unixio_write)
              DisposeObject(data->unixio_write);
          }
          
          if (data->softint_read) 
            FreeMem(data->softint_read, sizeof(struct Interrupt));
          if (data->softint_write)
            FreeMem(data->softint_write, sizeof(struct Interrupt));
        }
        
        if (data->replyport_read)
          FreeMem(data->replyport_read , sizeof(struct MsgPort));
        if (data->replyport_write)
          FreeMem(data->replyport_write, sizeof(struct MsgPort));

      } 
      
      close(data->filedescriptor);  
    }

    DisposeObject(obj);
    obj = NULL;
  } /* if (obj) */

  D(bug("%s - an error occurred!\n",__FUNCTION__));

exit:
  ReturnPtr("SerialUnit::New()", Object *, obj);
}

/******* SerialUnit::Dispose() ***********************************/
static Object *serialunit_dispose(Class *cl, Object *obj, struct pRoot_New *msg)
{
  struct HIDDSerialUnitData * data;
  EnterFunc(bug("SerialUnit::Dispose()\n"));

  data = INST_DATA(cl, obj);
  D(bug("Freeing filedescriptor (%d)!\n",data->filedescriptor));

  tcsetattr(data->filedescriptor, TCSANOW, &data->orig_termios);
  if (-1 != data->filedescriptor)
  { 
    close(data->filedescriptor);
  
    FreeMem(data->replyport_read,  sizeof(struct MsgPort));
    FreeMem(data->replyport_write, sizeof(struct MsgPort));

    FreeMem(data->softint_read , sizeof(struct Interrupt));
    FreeMem(data->softint_write, sizeof(struct Interrupt));

    DisposeObject(data->unixio_read);
    DisposeObject(data->unixio_write);
  }
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
  ULONG len = 0;
  ULONG error;
  
  EnterFunc(bug("SerialUnit::Write()\n"));

  D(bug("Writing %d bytes to fd %d (stream: %s)\n",
        msg->Length,
        data->filedescriptor,
        msg->Outbuffer));

  len = write(data->filedescriptor,
              msg->Outbuffer,
              msg->Length);

  if (len < msg->Length)
  {

    // !!!!!! FROM WHAT I CAN TELL THE FOLLOWING LINE
    //        CAUSES PROBLEMS. IT IS NECESSARY TO HAVE IT, THOUGH.
    error = Hidd_UnixIO_AsyncIO(data->unixio_write,
                                data->filedescriptor,
                                data->replyport_write,
                                vHidd_UnixIO_Write);

  }

  ReturnInt("SerialUnit::Write()",ULONG, len);
}

/***************************************************************************/

static ULONG valid_baudrates[] =
{
  0,
  50,
  75,
  110,
  134,
  150,
  200,
  300,
  600,
  1200,
  1800,
  2400,
  4800,
  9600,
  19200,
  38400,
  57600,
  115200,
  -1
};

/******* SerialUnit::SetBaudrate() **********************************/
ULONG serialunit_setbaudrate(Class *cl, Object *o, struct pHidd_SerialUnit_SetBaudrate *msg)
{
  struct HIDDSerialUnitData * data = INST_DATA(cl, o);
  BOOL valid = FALSE;
  
  if (msg->baudrate != data->baudrate)
  {
    int i = 0;
    D(bug("Trying to adjust the baudrate to %d\n",msg->baudrate));
    while (FALSE == valid && -1 != valid_baudrates[i])
    {
      if (msg->baudrate == valid_baudrates[i])
      {
        struct termios _termios;
        tcgetattr(data->filedescriptor, &_termios); 
        cfsetspeed(&_termios, data->baudrate);

        if (tcsetattr(data->filedescriptor, TCSADRAIN, &_termios) <0)
        {
          D(bug("Failed to set to new baudrate"));
        }
        else
        {
          D(bug("Adjusted to new baudrate!\n"));
          valid = TRUE;
        }
      }
      i++;
    } /* while */
  } /* if */
  return valid;
}

/******* SerialUnit::SendBreak() **********************************/
VOID serialunit_sendbreak(Class *cl, Object *o, struct pHidd_SerialUnit_SendBreak *msg)
{
  struct HIDDSerialUnitData * data = INST_DATA(cl, o);
  tcsendbreak(data->filedescriptor, msg->duration);
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
  ULONG error;
  struct HIDDSerialUnitData * data = iD;
  ssize_t len;
  UBYTE buffer[READBUFFER_SIZE];
  struct Message * msg;

  /*
  ** Get the unixio message from my port and free it
  */
  msg = GetMsg(data->replyport_read);
  FreeMem(msg, sizeof(struct uioMessage));

  /*
  ** Read the data from the port ...
  */
  len = read(data->filedescriptor, buffer, READBUFFER_SIZE);
  /*
  ** ... and deliver them to whoever is interested. 
  */

  if (NULL != data->DataReceivedCallBack)
    data->DataReceivedCallBack(buffer, len, data->unitnum);

  /*
  ** I want to be notified when the next data are coming in.
  */
  error = Hidd_UnixIO_AsyncIO(data->unixio_read,
                              data->filedescriptor,
                              data->replyport_read,
                              vHidd_UnixIO_Read);

}

AROS_UFH3(void, serialunit_write_more_data,
   AROS_UFHA(APTR, iD, A1),
   AROS_UFHA(APTR, iC, A5),
   AROS_UFHA(struct ExecBase *, SysBase, A6))
{
  struct HIDDSerialUnitData * data = iD;
  struct Message * msg;

  /*
  ** Get the unixio message from my port and free it
  */
  msg = GetMsg(data->replyport_read);
  FreeMem(msg, sizeof(struct uioMessage));

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
#define NUM_SERIALUNIT_METHODS 4

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
        {(IPTR (*)())serialunit_sendbreak,	moHidd_SerialUnit_SendBreak},
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

