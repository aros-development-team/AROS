#ifndef SERIAL_INTERN_H
#define SERIAL_INTERN_H
/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.9  2001/10/07 14:26:09  bergers
    Implemented Write Buffer Empty interrupt handler.

    Revision 1.8  2000/11/03 23:37:44  stegerg
    oop renaming stuff

    Revision 1.7  2000/09/11 19:15:04  stegerg
    changes because of callback functions now having an additional param.

    Revision 1.6  2000/03/09 16:55:27  bergers
    Some bugfixes.

    Revision 1.5  2000/01/12 18:14:04  bergers
    Update: Support of shared unit. AbortIO() implementation. Bugfixes, clenups etc.

    Revision 1.4  2000/01/09 21:35:13  bergers
    Update on serial device.

    Revision 1.3  1999/03/08 13:07:51  bergers
    Update.

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
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef DEVICES_SERIAL_H
#   include <devices/serial.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif


/* Predeclaration */
struct serialbase;


struct SerialUnit;


/* Prototypes */
BOOL copyInData(struct SerialUnit * SU, struct IOStdReq * IOStdReq);
BOOL copyInDataUntilZero(struct SerialUnit * SU, struct IOStdReq * IOStdReq);
struct SerialUnit * findUnit(struct serialbase * SerialDevice, 
                             ULONG unitnum);


ULONG RBF_InterruptHandler(UBYTE * data, ULONG length, ULONG unitnum, APTR userdata);
ULONG WBE_InterruptHandler(ULONG unitum, APTR userdata);

extern struct ExecBase * SysBase;




#define MINBUFSIZE 512

struct serialbase
{
    struct Device      device;
    struct ExecBase *  sysBase;
    BPTR               seglist;
    
    struct List        UnitList;
    ULONG              Status;
    
    struct Library    *SerialHidd;
    struct Library    *oopBase;
    
    OOP_Object        *SerialObject;
};


struct SerialUnit
{
  struct MinNode      su_Node;
  OOP_Object        * su_Unit;

  struct SignalSemaphore su_Lock;

  struct MsgPort      su_QReadCommandPort;
  struct Message    * su_ActiveRead;
  
  struct MsgPort      su_QWriteCommandPort;
  struct Message    * su_ActiveWrite;
  ULONG               su_NextToWrite;  /* index in the buffer of next data to tx */
  ULONG               su_WriteLength;  /* Number of bytes left to tx */

  ULONG               su_UnitNum;
  ULONG               su_SerFlags;    // copy of IOExtSer->io_SerFlags;
  ULONG               su_Status;
  
  ULONG               su_CtlChar;
  ULONG               su_Baud;
  ULONG               su_BrkTime;
  struct IOTArray     su_TermArray;
  UBYTE               su_ReadLen;
  UBYTE               su_WriteLen;
  UBYTE               su_StopBits;
 
  UBYTE               su_OpenerCount; // Count how many openers this unit has. Important in shared mode

  BYTE *              su_InputBuffer;
  UWORD               su_InputNextPos; // InputCurPos may never "overtake" InputFirst
  UWORD               su_InputFirst;  // the input buffer is organized as a "circle"
                                      // Next Pos is the Position in the buffer
                                      // where the NEXT byte will go into.
  UWORD               su_InBufLength;
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
#define STATUS_BUFFEROVERFLOW 8     /* This flag indicates that an overflow
                                       occurred with the internal receive
                                       buffer. All further bytes will be
                                       dropped until somebody reads the contents
                                       of the buffer.
                                    */

#ifdef SysBase
    #undef SysBase
#endif
//#define SysBase SerialDevice->sysBase

#ifdef OOPBase
    #undef OOPBase
#endif

#define OOPBase		(((struct serialbase *)SerialDevice)->oopBase)

#endif /* SERIAL_INTERN_H */

