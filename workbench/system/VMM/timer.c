#include <exec/types.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>

/************************************************************************
 * This is a generic timer module, which exports routines to open and 
 * close a UNIT_VBLANK timer, post requests to the timer and call a
 * function when it returns.
 * The routines defined here are reentrant.
 * The only exported functions are as following:
 ********
 *   NAME
 *     InitTimer -- initialize the timer for subsequent use.
 *
 *   SYNOPSIS
 *     InitTimer (NumIO, TimerSignal)
 *
 *     void *InitTimer (int, UWORD *);
 *
 *   FUNCTION
 *     Initialize the timer for subsequent use. This will allocate
 *     all the necessary data to use the timer including a signal
 *     to be used when a previously started timer expires.
 *
 *   INPUTS
 *     NumIO - Maximum number of simultaneously active timer requests.
 *     TimerSignal - A pointer to a word where the signal number is 
 *                   stored which is used when a timer has expired.
 *
 *   RESULTS
 *     Pointer to an internally used struct. Must be given in any
 *     succeeding timer call. NULL otherwise.
 *
 ******
 *   NAME
 *     CloseTimer -- close a previously opened timer
 *
 *   SYNOPSIS
 *     CloseTimer (TimerStruct)
 *
 *     void CloseTimer (void *);
 *
 *   FUNCTION
 *     Closes a timer previously opened with a call to OpenTimer.
 *
 *   INPUTS
 *     TimerStruct - The value returned by OpenTimer. This function is
 *                   safe to call with a NULL argument for this.
 *
 *   RESULTS
 *     None.
 *
 ******
 *   NAME
 *     AddTimedFunction - Add a function to be called after a certain
 *                        amount of time.
 *
 *   SYNOPSIS
 *     AddTimedFunction (TimerStruct, Secs, Micros, Function);
 *
 *     BOOL AddTimedFunction (void *, ULONG, ULONG, void (*) ());
 *
 *   FUNCTION
 *     Installs a function to be called after the specified amount of
 *     time. The UNIT_VBLANK timer is used for this so this will not
 *     be more accurate than to 1/50 s. For periodically called functions
 *     'Function' will typically include a call to AddTimedFunction.
 *     'Function' is called without any parameters.
 *
 *   INPUTS
 *     TimerStruct - The value returned by OpenTimer. This function is
 *                   safe to call with a NULL argument for this. In this
 *                   case AddTimedFunction does nothing.
 *     Secs - Number of seconds before 'Function' will be called.
 *     Micros - Together with Secs time before 'Function' will be called.
 *     Function - A function without parameters to be called when the
 *                timer expires.
 *   
 *   RESULTS
 *     Returns FALSE if either TimerStruct is NULL or the number of
 *     concurrently active timer request specified in OpenTimer is
 *     exceeded.
 *
 ******
 *   NAME
 *     HandleTimerReturn - Perform internal timer administration when
 *                         TimerSignal goes off.
 *
 *   SYNOPSIS
 *     HandleTimerReturn (TimerStruct)
 *
 *     void HandleTimerReturn (void *);
 *
 *   FUNCTION
 *     Performs internal management function for the timer whenever
 *     the timer signal returned by OpenTimer arrives.
 *
 *   INPUTS
 *     TimerStruct - The value returned by OpenTimer. This function is
 *                   safe to call with a NULL argument for this. In this
 *                   case AddTimedFunction does nothing.
 *
 *   RESULTS
 *     None
 *
 **********************************************************************/


struct MyTimerIO
  {
  struct timerequest StdTimeReq;
  void (*function) (void);         /* function to be called after time has */
  };                               /* elapsed */

struct TimerStruct
  {
  struct List TimerIOList;
  struct MsgPort *TimerPort;
  int NumTimerIOs;
  BOOL DeviceOpen;
  };

void CloseTimer (void*);

/************************************************************************/

void *InitTimer (int NumIO, UWORD *TimerSignal)

{
int i;
struct TimerStruct *MyTimer;
struct MyTimerIO *TmpReq;
struct Device *TimerDev;
struct Unit   *TimerUnit;

if ((MyTimer = AllocMem (sizeof (struct TimerStruct), MEMF_ANY)) == NULL)
  return (NULL);

NewList (&(MyTimer->TimerIOList));

if ((MyTimer->TimerPort = CreateMsgPort ()) == NULL)
  {
  CloseTimer (MyTimer);
  return (NULL);
  }

MyTimer->DeviceOpen = FALSE;
MyTimer->NumTimerIOs = NumIO;

for (i = 0; i < NumIO; i++)
  {
  if ((TmpReq = CreateIORequest (MyTimer->TimerPort, 
                                 sizeof (struct MyTimerIO))) == NULL)
    {
    CloseTimer (MyTimer);
    return (NULL);
    }
  AddTail (&(MyTimer->TimerIOList), (struct Node*)TmpReq);
  }

TmpReq = (struct MyTimerIO*) MyTimer->TimerIOList.lh_Head;
if (OpenDevice ("timer.device", UNIT_VBLANK, (struct IORequest*)TmpReq, 0L) != 0)
  {
  CloseTimer (MyTimer);
  return (NULL);
  }

MyTimer->DeviceOpen = TRUE;

TimerDev  = TmpReq->StdTimeReq.tr_node.io_Device;
TimerUnit = TmpReq->StdTimeReq.tr_node.io_Unit;

for (TmpReq = (struct MyTimerIO*) TmpReq->StdTimeReq.tr_node.io_Message.mn_Node.ln_Succ;
     TmpReq->StdTimeReq.tr_node.io_Message.mn_Node.ln_Succ != NULL;
     TmpReq = (struct MyTimerIO*) TmpReq->StdTimeReq.tr_node.io_Message.mn_Node.ln_Succ)
  {
  TmpReq->StdTimeReq.tr_node.io_Device = TimerDev;
  TmpReq->StdTimeReq.tr_node.io_Unit   = TimerUnit;
  }

*TimerSignal = MyTimer->TimerPort->mp_SigBit;
return (MyTimer);
}

/************************************************************************/

void CloseTimer (void *TimerStruct)

{
struct TimerStruct *MyTimer;
struct MyTimerIO *TmpReq;
int NumOutstanding;

MyTimer = (struct TimerStruct*) TimerStruct;

if (MyTimer != NULL)
  {
  if (MyTimer->TimerPort != NULL)
    {
    NumOutstanding = MyTimer->NumTimerIOs;
    while ((TmpReq = (struct MyTimerIO*)RemHead (&(MyTimer->TimerIOList))) != NULL)
      {
      if (--NumOutstanding == 0 && MyTimer->DeviceOpen)
        CloseDevice ((struct IORequest*)TmpReq);
      DeleteIORequest ((struct IORequest*)TmpReq);
      }

    while (NumOutstanding > 0)
      {
      WaitPort (MyTimer->TimerPort);
        while ((TmpReq = (struct MyTimerIO*)GetMsg (MyTimer->TimerPort)) != NULL)
        {
        if (--NumOutstanding == 0)
          CloseDevice ((struct IORequest*)TmpReq);
        DeleteIORequest ((struct IORequest*)TmpReq);
        }
      }
    DeleteMsgPort (MyTimer->TimerPort);
    }
  FreeMem (MyTimer, sizeof (struct TimerStruct));
  }
}

/************************************************************************/

BOOL AddTimedFunction (void *TimerStruct, ULONG secs, ULONG micros, 
                       void (*function) ())

{
struct TimerStruct *MyTimer;
struct MyTimerIO *TmpReq;

if (TimerStruct == NULL)
  return (FALSE);

MyTimer = (struct TimerStruct*) TimerStruct;
if ((TmpReq = (struct MyTimerIO*)RemHead (&(MyTimer->TimerIOList))) == NULL)
  return (FALSE);

TmpReq->function = function;
TmpReq->StdTimeReq.tr_time.tv_secs = secs;
TmpReq->StdTimeReq.tr_time.tv_micro = micros;
TmpReq->StdTimeReq.tr_node.io_Command = TR_ADDREQUEST;
TmpReq->StdTimeReq.tr_node.io_Flags = 0;

SendIO ((struct IORequest*)TmpReq);
return (TRUE);
}

/************************************************************************/

void HandleTimerReturn (void *TimerStruct)

{
struct TimerStruct *MyTimer;
struct MyTimerIO *TmpReq;
void (*tmp_func) (void);

if (TimerStruct == NULL)
  return;

MyTimer = (struct TimerStruct*) TimerStruct;
while ((TmpReq = (struct MyTimerIO*)GetMsg (MyTimer->TimerPort)) != NULL)
  {
  tmp_func = TmpReq->function;
  AddTail (&(MyTimer->TimerIOList), (struct Node*)TmpReq);
  (*tmp_func) ();
  }
}
