#ifndef VMM_STAT_H
#define VMM_STAT_H

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/ports.h>

/**********************************************************************/
/*    Getting statistics messages from VMM for further processing     */
/**********************************************************************/

struct VMStatMsg
  {
  struct Message VMMessage;   /* don't forget to put sizeof (struct VMStatMsg) */
                              /* into mn_Length. Future version of VMM might   */
                              /* extend this structure */
  struct Task   *Sender;      
  UWORD          Command;
  UWORD          ReplySignal;
  ULONG          VMSize;
  ULONG          VMFree;
  ULONG          Faults;
  ULONG          PagesWritten;
  ULONG          PagesRead;
  ULONG          NumFrames;
  ULONG          PagesUsed;
  ULONG          PageSize;
  ULONG          TrapStructsFree;
  };

#define VMCMD_AskStat 1472

/* Do the following to get a statistics message from VMM:
 * 
 * struct VMStatMsg *StatMsg;
 * LONG   VMSignal;
 * struct MsgPort *VMPort;
 *
 * if ((StatMsg = AllocMem (sizeof (struct VMStatMsg), MEMF_PUBLIC)) == NULL)
 *   Cleanup ();
 *
 * if ((VMSignal = AllocSignal (-1L)) == -1L)
 *   Cleanup ();
 *
 * StatMsg->VMMessage.mn_Length = sizeof (struct VMStatMsg);
 * StatMsg->Sender      = FindTask (NULL);
 * StatMsg->Command     = VMCMD_AskStat;
 * StatMsg->ReplySignal = VMSignal;
 *
 * Forbid ();
 * if ((VMPort = FindPort ("VMM_Port")) != NULL)
 *   PutMsg (VMPort, (struct Message*)StatMsg);
 * else
 *   ....
 *
 * Permit ();
 * 
 * Wait (1L << VMSignal);
 *
 * When this returns the StatMsg will contain the necessary parameters.
 * Do whatever you like with it.
 *
 * Cleanup ();
 *
 * Easy, isn't it?
 */

/**********************************************************************/
/*                Getting VM usage statistics from VMM                */
/**********************************************************************/

/* Send the following message structure to 'VMM_Port' just as a few lines
 * above.
 */

struct VMUsageMsg
  {
  struct Message VMMessage;   /* don't forget to put sizeof (struct VMStatMsg) */
                              /* into mn_Length. Future version of VMM might   */
                              /* extend this structure */
  struct Task   *Sender;      
  UWORD          Command;
  UWORD          ReplySignal;
  struct List   *TaskList;
  };

/* Use this for the Command value */

#define VMCMD_AskVMUsage                1481

/* As soon as this returns, 'TaskList' will have been filled in. 
 * 'TaskList' is a list of 'struct TaskVMUsage' described below.
 * You can do whatever you like with the list, but as soon as you are
 * finished you should call FreeMem for every 'struct TaskVMUsage'
 * with a size of 'FreeSize' and for the List structure.
 */

struct TaskVMUsage
  {
  struct Node vu_Node;
  ULONG  VMInUse;
  ULONG  FreeSize;
  };

#endif
