#ifndef SERIAL_INTERN_H
#define SERIAL_INTERN_H
/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.2  1999/03/05 13:29:29  bergers
    Update.

    Revision 1.1  1999/03/03 04:33:31  bergers
    Very preliminary version. Doesn't do anything useful yet. (Mainly just to prevent that I erase my harddrive)


    Desc: Private definitions for Serial device.
    Lang:
*/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef EXEC_DEVICES_H
#   include <exec/devices.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef DEVICES_SERIAL_H
#   include <devices/serial.h>
#endif

/* Size of the serial device's stack */
#define IDTASK_STACKSIZE 20000
/* Priority of the serial.device task */
#define IDTASK_PRIORITY 20

/* Predeclaration */
struct serialbase;


struct SerialUnit;


/* Prototypes */
struct Task *CreateSerialTask(APTR taskparams, struct serialbase *SerialDevice);
BOOL copyInData(struct SerialUnit * SU, struct IOStdReq * IOStdReq);
BOOL copyInDataUntilZero(struct SerialUnit * SU, struct IOStdReq * IOStdReq);


extern struct ExecBase * SysBase;




#define MINBUFSIZE 512

struct serialbase
{
    struct Device      device;
    struct ExecBase *  sysBase;
    BPTR               seglist;
    
    struct SerialUnit *FirstUnit;
    ULONG              Status;
};


struct SerialUnit
{
  struct Unit         su_Unit;

  struct SerialUnit * su_Next;
  
  struct SignalSemaphore su_Lock;

  struct MsgPort      su_ReadCommandPort;
  struct MsgPort      su_WriteCommandPort;

  ULONG               su_UnitNum;
  ULONG               su_Flags;    // copy of IOExtSer->io_SerFlags;
  ULONG               su_Status;
  
  ULONG               su_CtlChar;
  ULONG               su_Baud;
  ULONG               su_BrkTime;
  struct IOTArray     su_TermArray;
  UBYTE               su_ReadLen;
  UBYTE               su_WriteLen;
  UBYTE               su_StopBits;
  

  BYTE *              su_InputBuffer;
  UWORD               su_InputNextPos; // InputCurPos may never "overtake" InputFirst
  UWORD               su_InputFirst;  // the input buffer is organized as a "circle"
                                      // Next Pos is the Position in the buffer
                                      // where the NEXT byte will go into.
  UWORD               su_InBufLength;
  // Pointer to the HIDD
  APTR                su_HIDD;
};

/* a few flags for the status */
#define STATUS_CHANGING_IN_BUFFER 1 /* the user requested a change of the size
                                       of the input buffer and is now changing
                                       the input buffer. During this time
                                       it could happen that the interrupt that
                                       holds incoming bytes writes to a memory
                                       location that would be invalid.
                                       This tells the interrupt to simply drop
                                       the data that it was about to write
                                       to the input buffer.
                                     */
#define STATUS_READS_PENDING  2 
#define STATUS_WRITES_PENDING 4    /* When the task write the first byte into the UART
                                      it has to set this flag. 
                                      When the HW-interrupt happens and there is
                                      no more byte to write to the UART this
                                      flag has to be cleared immediately
                                    */
#define STATUS_BUFFEROVERFLOW 8

#ifdef SysBase
    #undef SysBase
#endif
//#define SysBase SerialDevice->sysBase

#endif /* SERIAL_INTERN_H */

