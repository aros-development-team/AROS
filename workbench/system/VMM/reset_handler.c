/**************************************************************************
 * This is a reentrant generic reset handler module. It provides routines *
 * to install and remove a function to be called upon keyboard reset.     *
 * The available functions are:                                           *
 *   1. void *InstallResetHandler (void (*func) (void), LONG priority);   *
 *        This installs the function 'func', which is called in the event *
 *        of a reset. If this function is not successful, it returns NULL,*
 *        otherwise it returns a pointer to be given to RemoveResetHandler*
 *        when deinstalling the handler.                                  *
 *                                                                        *
 *   2. void RemoveResetHandler (void* ResetHandlerParams);               *
 *        Removes a previously install reset handler. This function is    *
 *        safe to call even if InstallResetHandler failed.                *
 *                                                                        *
 *   3. void ResetHandlerDone (void *ResetHandlerParams);                 *
 *        Informs the system that it's OK to reset the system as far as   *
 *        this resethandler is concerned.                                 *
 **************************************************************************/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/interrupts.h>
#include <devices/keyboard.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>

struct ResetHandlerData
  {
  struct MsgPort *ResetPort;
  struct Interrupt *ResetInt;
  struct IOStdReq *ResetReq;
  };

void *InstallResetHandler (void (*func) (void), LONG priority)

{
struct Interrupt *ResetInt;
struct MsgPort *ResetPort;
struct IOStdReq *ResetReq;
struct ResetHandlerData *rhd;

if ((rhd = AllocMem (sizeof (struct ResetHandlerData), MEMF_PUBLIC)) == NULL)
  return (NULL);

if ((ResetInt = AllocMem (sizeof (struct Interrupt), MEMF_PUBLIC)) == NULL)
  {
  FreeMem (rhd, sizeof (struct ResetHandlerData));
  return (NULL);
  }

rhd->ResetInt = ResetInt;

if ((ResetPort = CreateMsgPort ()) == NULL)
  {
  FreeMem (ResetInt, sizeof (struct Interrupt));
  FreeMem (rhd, sizeof (struct ResetHandlerData));
  return (NULL);
  }

rhd->ResetPort = ResetPort;

if ((ResetReq = CreateIORequest (ResetPort, sizeof (struct IOStdReq))) == NULL)
  {
  DeleteMsgPort (ResetPort);
  FreeMem (ResetInt, sizeof (struct Interrupt));
  FreeMem (rhd, sizeof (struct ResetHandlerData));
  return (NULL);
  }

rhd->ResetReq = ResetReq;

if (OpenDevice ("keyboard.device", 0L, (struct IORequest*)ResetReq, 0L) != NULL)
  {
  DeleteIORequest (ResetReq);
  DeleteMsgPort (ResetPort);
  FreeMem (ResetInt, sizeof (struct Interrupt));
  FreeMem (rhd, sizeof (struct ResetHandlerData));
  return (NULL);
  }

/* All needed resources allocated now */
ResetInt->is_Node.ln_Pri = priority;
ResetInt->is_Code = func;
ResetReq->io_Data = ResetInt;
ResetReq->io_Command = KBD_ADDRESETHANDLER;
DoIO ((struct IORequest*)ResetReq);
if (ResetReq->io_Error != 0)
  {
  CloseDevice ((struct IORequest*)ResetReq);
  DeleteIORequest (ResetReq);
  DeleteMsgPort (ResetPort);
  FreeMem (ResetInt, sizeof (struct Interrupt));
  FreeMem (rhd, sizeof (struct ResetHandlerData));
  return (NULL);
  }

return (rhd);
}

/************************************************************************/

void RemoveResetHandler (void* ResetHandlerParams)

{
struct ResetHandlerData *rhd;
struct Interrupt *ResetInt;
struct MsgPort *ResetPort;
struct IOStdReq *ResetReq;

if (ResetHandlerParams == NULL)
  return;

rhd = (struct ResetHandlerData*)ResetHandlerParams;
ResetInt = rhd->ResetInt;
ResetPort = rhd->ResetPort;
ResetReq = rhd->ResetReq;

ResetReq->io_Command = KBD_REMRESETHANDLER;
ResetReq->io_Data = ResetInt;
DoIO ((struct IORequest*)ResetReq);
CloseDevice ((struct IORequest*)ResetReq);
DeleteIORequest (ResetReq);
DeleteMsgPort (ResetPort);
FreeMem (ResetInt, sizeof (struct Interrupt));
FreeMem (rhd, sizeof (struct ResetHandlerData));
}

/************************************************************************/

void ResetHandlerDone (void *ResetHandlerParams)

{
struct ResetHandlerData *rhd;
struct Interrupt *ResetInt;
struct IOStdReq *ResetReq;

if (ResetHandlerParams == NULL)
  return;

rhd = (struct ResetHandlerData*)ResetHandlerParams;
ResetInt = rhd->ResetInt;
ResetReq = rhd->ResetReq;


ResetReq->io_Command = KBD_RESETHANDLERDONE;
ResetReq->io_Data = ResetInt;
DoIO ((struct IORequest*)ResetReq);
}
