#define AROS_ALMOST_COMPATIBLE 1
#include <proto/exec.h>
#include <proto/timer.h>
#include <exec/lists.h>
#include <exec/interrupts.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <devices/inputevent.h>
#include <devices/input.h>
#include <devices/timer.h>
#include <devices/keyboard.h>
#include <devices/serial.h>
#include <intuition/intuition.h>
#include <aros/asmcall.h>

#include "serial_intern.h"

/***********************************
** Serial device task entry point  **
***********************************/
void ProcessEvents (struct IDTaskParams *taskparams)
{
  struct serialbase *SerialDevice = taskparams->SerialDevice;
  struct MsgPort * CommandPort = &SerialDevice->CommandPort;
  
  CommandPort->mp_Flags   = PA_SIGNAL;
  /* This will always successd, as this task just has been created */
  CommandPort->mp_SigBit = AllocSignal(-1L);

  CommandPort->mp_SigTask = FindTask(NULL);
  
  NEWLIST( &(CommandPort->mp_MsgList) );
  
  /* Tell the task that created us, that we are finished initilaizing */
  Signal(taskparams->Caller, taskparams->Signal);
  
  /* process all the incoming requests */
  for (;;)
  {
    struct IOExtSer * msg;
    WaitPort(CommandPort);
    /* hey, there's a message for me. So I gotta do something, I guess */
    msg = (struct IOExtSer *)GetMsg(CommandPort);

    kprintf("serial.task: Got a message! %x\n",msg);

    switch (msg->IOSer.io_Command)
    {
      case CMD_READ:
        kprintf("got a request for a read!!!\n");
        strcpy((char *)msg->IOSer.io_Data,
               "Greetings from serial device.");
        /* reply to the message */
        kprintf("replying to port : %x\n",msg->IOSer.io_Message.mn_ReplyPort);
        ReplyMsg((struct Message *)msg);
      break;
      
      
    }
  }
    
} /* ProcessEvents */

