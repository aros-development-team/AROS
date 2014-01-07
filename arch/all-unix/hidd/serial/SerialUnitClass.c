/*
    Copyright � 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial Unit hidd class implementation.
    Lang: english
*/

/* Some POSIX includes */
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "unix_funcs.h"


/* the rest are Amiga includes */
#define timeval aros_timeval
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

#include <devices/serial.h>
#include <intuition/preferences.h>

#include <aros/symbolsets.h>


#include "serial_intern.h"

#include LC_LIBDEFS_FILE
#undef timeval

#undef  SDEBUG
#undef  DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

void serialunit_receive_data();
void serialunit_write_more_data();

/* Some utility functions */
static void settermios(struct HIDDSerialUnitData * data);
static void adapt_termios(struct termios * termios,
                          struct Preferences * prefs);

static char * unitname[] =
{
 "/dev/ttyS0",
 "/dev/ttyS1",
 "/dev/ttyS2",
 "/dev/ttyS3"
 };

/*************************** Classes *****************************/

/******* SerialUnit::New() ***********************************/
OOP_Object *UXSerUnit__Root__New(OOP_Class *cl, OOP_Object *obj, struct pRoot_New *msg)
{
  struct HIDDSerialUnitData * data;
#if 0
  static const struct TagItem tags[] = {{ TAG_END, 0}};
#endif
  struct TagItem *tag, *tstate;
  ULONG unitnum = 0;
  
  EnterFunc(bug("SerialUnit::New()\n"));
  D(bug("SerialUnit created on %s at %s.\n",__DATE__,__TIME__));

  tstate = msg->attrList;
  while ((tag = NextTagItem(&tstate)))
  {
      ULONG idx;

#define csd CSD(cl)
      if (IS_HIDDSERIALUNIT_ATTR(tag->ti_Tag, idx))
#undef csd
      {
	  switch (idx)
	  {
	      case aoHidd_SerialUnit_Unit:
		  unitnum = (ULONG)tag->ti_Data;
		  break;
	  }
      }

  } /* while (tags to process) */
  
  D(bug("!!!!Request for unit number %d\n",unitnum));

  obj = (OOP_Object *)OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);

  if (obj)
  {
    struct termios _termios;
    const struct TagItem tags[] = {
        {aHidd_UnixIO_Opener      , (IPTR)"serial.hidd"},
        {aHidd_UnixIO_Architecture, (IPTR)AROS_ARCHITECTURE},
        {TAG_END}
    };
    
    data = OOP_INST_DATA(cl, obj);
    
    data->unitnum = unitnum;

    data->unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);
    if (!data->unixio) {
        OOP_DisposeObject(obj);
        obj = NULL;
        goto exit;
    }

    D(bug("Opening %s.\n",unitname[data->unitnum]));

    data->filedescriptor = Hidd_UnixIO_OpenFile(data->unixio, unitname[data->unitnum], O_NONBLOCK|O_RDWR, 0, NULL);

    D(bug("Opened %s on handle %d\n",unitname[data->unitnum], data->filedescriptor));
    
    if (-1 != data->filedescriptor)
    {
      struct IntuitionBase * IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",0);

#if 0
      /*
      ** Configure the tty driver
      */
      tcgetattr(data->filedescriptor, &data->orig_termios);
      tcgetattr(data->filedescriptor, &_termios); 
      cfmakeraw(&_termios);
#endif

      /* 
       * Get the preferences information from intuition library.
       * If intuition could not be opened (?) then I will just
       * use the default termios.
       */
      if (NULL != IntuitionBase) {
        struct Preferences prefs;
        GetPrefs(&prefs,sizeof(prefs));
        data->baudrate       = prefs.BaudRate;
        adapt_termios(&_termios, &prefs);
        CloseLibrary((struct Library *)IntuitionBase);
      } else {
//          data->baudrate       = 
      }
      D(bug("Setting baudrate to %d.\n",data->baudrate));

#if 0
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

            data->replyport_read->mp_Flags = PA_SOFTINT;
            data->replyport_read->mp_SoftInt = data->softint_read;

            data->replyport_write->mp_Flags = PA_SOFTINT;
            data->replyport_write->mp_SoftInt = data->softint_write;

            data->unixio_read  = OOP_NewObject(NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);
            data->unixio_write = OOP_NewObject(NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);

            if (NULL != data->unixio_read && NULL != data->unixio_write)
            {
              ULONG error;
              D(bug("Creating UnixIO AsyncIO command!\n"));

              error = Hidd_UnixIO_AsyncIO(data->unixio_read,
                                          data->filedescriptor,
                                          vHidd_UnixIO_Terminal,
                                          data->replyport_read,
                                          vHidd_UnixIO_Read | vHidd_UnixIO_Keep,
                                          SysBase);

              error = Hidd_UnixIO_AsyncIO(data->unixio_write,
                                          data->filedescriptor,
                                          vHidd_UnixIO_Terminal,
                                          data->replyport_write,
                                          vHidd_UnixIO_Write | vHidd_UnixIO_Keep,
                                          SysBase);
              goto exit;

            }

            if (NULL != data->unixio_read)
              OOP_DisposeObject(data->unixio_read);

            if (NULL != data->unixio_write)
              OOP_DisposeObject(data->unixio_write);
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
#else
       goto exit;
#endif
      
      Hidd_UnixIO_CloseFile(data->unixio, data->filedescriptor, NULL);
    }

    OOP_DisposeObject(obj);
    obj = NULL;
  } /* if (obj) */

  D(bug("%s - an error occurred!\n",__FUNCTION__));

exit:
  ReturnPtr("SerialUnit::New()", OOP_Object *, obj);
}

/******* SerialUnit::Dispose() ***********************************/
OOP_Object *UXSerUnit__Root__Dispose(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
  struct HIDDSerialUnitData * data;
  EnterFunc(bug("SerialUnit::Dispose()\n"));

  data = OOP_INST_DATA(cl, obj);
  D(bug("Freeing filedescriptor (%d)!\n",data->filedescriptor));

  if (-1 != data->filedescriptor)
  { 
#if 0
    tcsetattr(data->filedescriptor, TCSANOW, &data->orig_termios);
    Hidd_UnixIO_AbortAsyncIO(data->unixio_read,
                             data->filedescriptor,
                             SysBase);
#endif

    Hidd_UnixIO_CloseFile(data->unixio, data->filedescriptor, NULL);
 
#if 0
    FreeMem(data->replyport_read,  sizeof(struct MsgPort));
    FreeMem(data->replyport_write, sizeof(struct MsgPort));

    FreeMem(data->softint_read , sizeof(struct Interrupt));
    FreeMem(data->softint_write, sizeof(struct Interrupt));

    OOP_DisposeObject(data->unixio_read);
    OOP_DisposeObject(data->unixio_write);
#endif
  }
  OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
  ReturnPtr("SerialUnit::Dispose()", OOP_Object *, obj);
}



/******* SerialUnit::Init() **********************************/
BOOL UXSerUnit__Hidd_SerialUnit__Init(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Init *msg)
{
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  
  EnterFunc(bug("SerialUnit::Init()\n"));
  
  data->DataReceivedCallBack = msg->DataReceived;
  data->DataReceivedUserData = msg->DataReceivedUserData;
  data->DataWriteCallBack    = msg->WriteData;
  data->DataWriteUserData    = msg->WriteDataUserData;
  
  ReturnBool("SerialUnit::Init()", TRUE);
}

/******* SerialUnit::Write() **********************************/
ULONG UXSerUnit__Hidd_SerialUnit__Write(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Write *msg)
{
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  ULONG len = 0;
  
  EnterFunc(bug("SerialUnit::Write()\n"));
  /*
   * If the output is currently suspended then I don't do anything...
   */

  if (TRUE == data->stopped)
    return 0;

  D(bug("Writing %d bytes to fd %d (stream: %s)\n",
        msg->Length,
        data->filedescriptor,
        msg->Outbuffer));

  len = Hidd_UnixIO_WriteFile(data->unixio, data->filedescriptor,
              msg->Outbuffer,
              msg->Length, NULL);


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
  ~0
};

/*** unused due to cfsetspeed ***

static LONG unix_baudrates[] =
{
  B0,
  B50,
  B75,
  B110
  B134,
  B150,
  B200,
  B300,
  B600,
  B1200,
  B1800,
  B2400,
  B4800,
  B9600,
  B19200,
  B38400,
  B57600,
  B115200
};

********************************/

/******* SerialUnit::SetBaudrate() **********************************/
ULONG UXSerUnit__Hidd_SerialUnit__SetBaudrate(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_SetBaudrate *msg)
{
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  BOOL valid = FALSE;
  
  if (msg->baudrate != data->baudrate)
  {
    int i = 0;
    D(bug("Trying to adjust the baudrate to %d\n",msg->baudrate));
    while (FALSE == valid && ~0 != valid_baudrates[i])
    {
      if (msg->baudrate == valid_baudrates[i])
      {
#if 0
        struct termios _termios;
        tcgetattr(data->filedescriptor, &_termios); 
        cfsetspeed(&_termios, msg->baudrate);

        if (tcsetattr(data->filedescriptor, TCSANOW, &_termios) <0)
        {
          D(bug("Failed to set to new baudrate %d\n",msg->baudrate));
        }
        else
        {
          data->baudrate = msg->baudrate;
          D(bug("Adjusted to new baudrate %d!\n",msg->baudrate));
          valid = TRUE;
        }
#else
        valid = TRUE;
#endif
      }
      i++;
    } /* while */
  } /* if */
  return valid;
}

static UBYTE valid_datalengths[] =
{
  5,
  6,
  7,
  8,
  0
};

static UBYTE unix_datalengths[] =
{
  CS5,
  CS6,
  CS7,
  CS8
};

/******* SerialUnit::SetParameters() **********************************/
ULONG UXSerUnit__Hidd_SerialUnit__SetParameters(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_SetParameters *msg)
{
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  BOOL valid = TRUE;
  int i = 0;
  struct TagItem * tags = msg->tags;

  while (TAG_END != tags[i].ti_Tag && TRUE == valid)
  {
    switch (tags[i].ti_Tag)
    {
      case TAG_DATALENGTH:
        if ((UBYTE)tags[i].ti_Data != data->datalength)
        {
          int j = 0;
          BOOL found = FALSE;
          while (0 != valid_datalengths[j])
          {
            if ((UBYTE)tags[i].ti_Data == valid_datalengths[j])
            {
              found = TRUE;
              data->datalength = unix_datalengths[j];
              break;
            }

            j++;
          }
          valid = found;
          
        }
      break;
        
      case TAG_STOP_BITS:
          if (2 == tags[i].ti_Data) {
            data->stopbits = 2;
          } else 
            data->stopbits = 1;
      break;
      
      case TAG_PARITY:
	  if ( /* PARITY_0    == tags[i].ti_Data ||
	          PARITY_1    == tags[i].ti_Data || */
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
      
      case TAG_SKIP:
      case TAG_IGNORE:
      break;
      
      default: 
        valid = FALSE;
    }
    i++;
  }
  
  settermios(data);
  
  return valid;
}

/******* SerialUnit::SendBreak() **********************************/
BYTE UXSerUnit__Hidd_SerialUnit__SendBreak(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_SendBreak *msg)
{
#if 0 
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);
  if (0 == tcsendbreak(data->filedescriptor, msg->duration))
#endif
    return 0;
  
  return SerErr_LineErr;
}

/******* SerialUnit::Start() **********************************/
VOID UXSerUnit__Hidd_SerialUnit__Start(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Start *msg)
{
	struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);

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
VOID UXSerUnit__Hidd_SerialUnit__Stop(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_Stop *msg)
{
  struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);

  /*
   * The next time the interrupt comes along and asks for
   * more data we just don't do anything...
   */
  data->stopped = TRUE;
}

/****** SerialUnit::GetCapabilities ********************************/
VOID UXSerUnit__Hidd_SerialUnit__GetCapabilities(OOP_Class * cl, OOP_Object *o, struct TagItem * tags)
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
UWORD UXSerUnit__Hidd_SerialUnit__GetStatus(OOP_Class *cl, OOP_Object *o, struct pHidd_SerialUnit_GetStatus *msg)
{
//	struct HIDDSerialUnitData * data = OOP_INST_DATA(cl, o);

	return 0;
}

/************* The software interrupt handler that gets data from UART *****/


#define READBUFFER_SIZE 513

AROS_INTH1(serialunit_receive_data, struct HIDDSerialUnitData *, data)
{
  AROS_INTFUNC_INIT

  ssize_t len;
  UBYTE buffer[READBUFFER_SIZE];
//  struct Message * msg;

  /*
  ** Get the unixio message from my port but don't free it
  */
//  msg = GetMsg(data->replyport_read);
//  FreeMem(msg, sizeof(struct uioMessage));

  /*
  ** Read the data from the port ...
  */
  len = Hidd_UnixIO_ReadFile(data->unixio, data->filedescriptor, buffer, READBUFFER_SIZE, NULL);
  /*
  ** ... and deliver them to whoever is interested. 
  */

  if (NULL != data->DataReceivedCallBack)
    data->DataReceivedCallBack(buffer, len, data->unitnum, data->DataReceivedUserData);

  return FALSE;

  AROS_INTFUNC_EXIT
}

AROS_INTH1(serialunit_write_more_data, struct HIDDSerialUnitData *, data)
{
  AROS_INTFUNC_INIT

//  struct Message * msg;

  /*
  ** Get the unixio message from my port but don't free it
  */
//  msg = GetMsg(data->replyport_write);
//  FreeMem(msg, sizeof(struct uioMessage));

  /*
  ** Ask for more data be written to the unit
  ** but only if output is not currently suspended.
  */
  if (TRUE == data->stopped)
    return;
  D(bug("Asking for more data to be written to unit %d\n",data->unitnum));

  if (NULL != data->DataWriteCallBack)
    data->DataWriteCallBack(data->unitnum, data->DataWriteUserData);

  return FALSE;

  AROS_INTFUNC_EXIT
}


/******* init_serialunitclass ********************************/

#undef __IHidd_SerialUnitAB
#define __IHidd_SerialUnitAB (LIBBASE->hdg_csd.hiddSerialUnitAB)

#undef __IHidd_UnixIO
#define __IHidd_UnixIO (LIBBASE->hdg_csd.hiddUnixIOAttrBase)

static int UXSerUnit_InitAttrBase(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("    UXSerUnit_InitAttrBase(LIBBASE=%p)\n", LIBBASE));

    __IHidd_SerialUnitAB = OOP_ObtainAttrBase(IID_Hidd_SerialUnit);

    __IHidd_UnixIO = OOP_ObtainAttrBase(IID_Hidd_UnixIO);

    ReturnInt("UXSerUnit_InitAttrBase", ULONG, __IHidd_SerialUnitAB != 0 && __IHidd_UnixIO != 0);
}

ADD2INITLIB(UXSerUnit_InitAttrBase, 0)


/**************************************************************/

static void settermios(struct HIDDSerialUnitData * data)
{
#if 0
  struct termios _termios;
  tcgetattr(data->filedescriptor, &_termios);

  _termios.c_cflag &= ~CSIZE;
  _termios.c_cflag |= data->datalength;

  /*
   * Parity
   */
  if (FALSE == data->parity)
    _termios.c_cflag &= ~(PARENB|PARODD);
  else
  {
    _termios.c_cflag |= PARENB;
    switch (data->paritytype)
    {
      case PARITY_EVEN:
        _termios.c_cflag &= ~PARODD;
      break;
      
      case PARITY_ODD:
        _termios.c_cflag |= PARODD;
      break;
    }
  }

  /*
   * Stop Bits 
   */
  _termios.c_cflag &= ~CSTOPB;
  if (2 == data->stopbits)
    _termios.c_cflag |= CSTOPB;

  if (tcsetattr(data->filedescriptor, TCSADRAIN, &_termios) < 0)
  {
//    D(bug("Failed to set new termios\n"));
  }
  else
  {
//    D(bug("Adjusted to new termios!\n"));
  }
#endif
} /* settermios */

/**************************************************************/

/*
 * Adapt the termios structure to the preferences
 */
static void adapt_termios(struct termios * termios,
                          struct Preferences * prefs)
{
#if 0
	cfmakeraw(termios);
	/*
	 * Parity.
	 */
	termios->c_cflag &= ~(PARENB|PARODD);
	switch ((prefs->SerParShk >> 4) & 0x0f) {
		case SPARITY_NONE:
			termios->c_cflag &= ~(PARENB|PARODD);
		break;
		
		case SPARITY_EVEN:
			termios->c_cflag |= PARENB;
		break;
		
		case SPARITY_ODD:
			termios->c_cflag |= (PARODD|PARENB);
		break;

		case SPARITY_MARK:
		case SPARITY_SPACE:
		default:
		break;
	}
	
	/*
	 * Bit per character 
	 */
	termios->c_cflag &= ~CSIZE;
	switch ((prefs->SerRWBits & 0x0f)) {
		default: /* 8 bit */
		case 0:
			termios->c_cflag |= CS8;
		break;
		
		case 1: /* 7 bit */
			termios->c_cflag |= CS7;
		break;
		
		case 2: /* 6 bit */
			termios->c_cflag |= CS6;
		break;
		
		case 3: /* 5 bit */
			termios->c_cflag |= CS5;
		break;
	}

	/*
	 * 2 stop bits ? default is '1'.
	 */
	if (1 == (prefs->SerStopBuf >> 4))
		termios->c_cflag |= CSTOPB;
	else
		termios->c_cflag &= ~CSTOPB;
	
	/*
	 * Handshake to be used.
	 */
	termios->c_iflag &= ~(IXON|IXOFF);
	termios->c_cflag &= ~CRTSCTS;
	switch((prefs->SerParShk & 0x0f)) {
		case SHSHAKE_XON:
			termios->c_iflag |= (IXON|IXOFF);
		break;
		
		case SHSHAKE_RTS:
			termios->c_cflag |= CRTSCTS;
		break;
		
		default:
		break;
	}

	cfsetspeed(termios, prefs->BaudRate);
#endif
	
} /* adapt_termios */
