#ifndef PARALLEL_INTERN_H
#define PARALLEL_INTERN_H
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private definitions for Parallel device.
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
#   include <devices/parallel.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif


/* Predeclaration */
struct parallelbase;


struct ParallelUnit;


/* Prototypes */
struct ParallelUnit * findUnit(struct parallelbase * ParallelDevice, 
                             ULONG unitnum);


ULONG RBF_InterruptHandler(UBYTE * data, ULONG length, ULONG unitnum, APTR userdata);
ULONG WBE_InterruptHandler(ULONG unitum, APTR userdata);

extern struct ExecBase * SysBase;




#define MINBUFSIZE 512

struct parallelbase
{
    struct Device      device;
    struct ExecBase *  sysBase;
    BPTR               seglist;
    
    struct List        UnitList;
    ULONG              Status;
    
    struct Library    *ParallelHidd;
    struct Library    *oopBase;
    
    OOP_Object        *ParallelObject;
};


struct ParallelUnit
{
  struct MinNode      pu_Node;
  OOP_Object        * pu_Unit;

  struct SignalSemaphore pu_Lock;

  struct MsgPort      pu_QReadCommandPort;
  struct Message    * pu_ActiveRead;
  
  struct MsgPort      pu_QWriteCommandPort;
  struct Message    * pu_ActiveWrite;
  ULONG     	      pu_NextToWrite;  /* index in the buffer of next data to tx */
  ULONG     	      pu_WriteLength;  /* Number of bytes left to tx */
  
  ULONG               pu_UnitNum;
  ULONG               pu_Flags;    // copy of IOExtSer->io_SerFlags;
  ULONG               pu_Status;
  
  ULONG               pu_CtlChar;
  struct IOPArray     pu_PTermArray;
 
  UBYTE               pu_OpenerCount; // Count how many openers this unit has. Important in shared mode

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

#ifdef OOPBase
    #undef OOPBase
#endif

#define OOPBase		(((struct parallelbase *)ParallelDevice)->oopBase)

#endif /* PARALLEL_INTERN_H */
