/*
    Copyright (C) 2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: Waits upto 10 seconds for a user specified Port to become available
    Lang: English
*/

#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/timer.h>
#include <stdio.h>
#include <proto/alib.h>

#include <aros/debug.h>

STRPTR version = "$VER: WaitForPort 0.0.1 (26.12.2005)";
STRPTR WaitForPort_ArgTemplate = "P=PORT/A";
char   *WaitForPort_Arguments[2];
struct RDArgs *WFP_rda = NULL;

struct Device              *TimerBase = NULL;
static struct MsgPort      *timerport = NULL;	      /* Timer message reply port */
static struct timerequest  *timerIORequest = NULL;    /* template IORequest */
ULONG wait_time;
ULONG wait_limit;
int
main(int argc, char *argv[])
{
   struct MsgPort *AROSTCP_Port = NULL;
   
   wait_time = 0;
   
   if (WFP_rda = ReadArgs(WaitForPort_ArgTemplate, WaitForPort_Arguments, NULL))
   {
 
      if (WaitForPort_Arguments[0])
      {
D(bug("[WaitForPort] Waiting for '%s' port\n",WaitForPort_Arguments[0]));
      }
      
      timerport = CreateMsgPort();
      if (timerport != NULL)
      {
         /* allocate and initialize the template message structure */
         timerIORequest = (struct timerequest *) CreateIORequest(timerport, sizeof(struct timerequest));

         if (timerIORequest != NULL)
         {
            if (!(OpenDevice(TIMERNAME, UNIT_VBLANK, 
			                      (struct IORequest *)timerIORequest, 0)))
            {
            	/* Make sure that we got at least V36 timer, since we use some
            	 * functions defined only in V36 and later. */

            	if ((timerIORequest->tr_node.io_Device)->dd_Library.lib_Version >= 36)
            	{
            	   /* initialize TimerBase from timerIORequest */
                  TimerBase = (struct Library *)timerIORequest->tr_node.io_Device;

            	   /* Initialize some fields of the IO request to common values */
            	   timerIORequest->tr_node.io_Command = TR_ADDREQUEST;

            	   /* NT_UNKNOWN means unused, too (see note on exec/nodes.h) */
            	   timerIORequest->tr_node.io_Message.mn_Node.ln_Type = NT_UNKNOWN;

                  timerIORequest->tr_time.tv_micro = 1000000;
                  wait_limit = timerIORequest->tr_time.tv_micro * 10; /* Default to a 10 second wait */

                  BeginIO((struct IORequest *)timerIORequest);

/* MAIN LOOP */
                  while(1)
                  {
D(bug("[WaitForPort] In Wait Loop ..\n"));
                     ULONG mask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | (1 << timerport->mp_SigBit);
      					mask = Wait(mask);
                     if (mask & SIGBREAKF_CTRL_C) break;
			         	if (mask & SIGBREAKF_CTRL_D) break;
                     if (mask & (1 << timerport->mp_SigBit))
                     {
D(bug("[WaitForPort] Recieved timer signal? ..\n"));
                        timerIORequest = (struct timerrequest *)GetMsg(timerport);
                        if (timerIORequest)
                        {
                           AROSTCP_Port = FindPort(WaitForPort_Arguments[0]);
                           wait_time += 1000000;
                           if (!(AROSTCP_Port))
                           {
                              if (wait_time > wait_limit)
                              {
D(bug("[WaitForPort] Timeout Reached\n"));
                                  break;
                              }
D(bug("[WaitForPort] Port not found .. secs=%d\n",wait_time/1000000));
                           }
                           else
                           {
D(bug("[WaitForPort] Port found ... escaping from wait loop\n"));
                              break;
                           }
                     	   timerIORequest->tr_node.io_Command = TR_ADDREQUEST;
                           timerIORequest->tr_time.tv_micro = 1000000;
                           BeginIO((struct IORequest *)timerIORequest);
                        }
                     }
                  }
               }
            }
         }

/* CLEANUP */

         if (timerIORequest)
         {
            TimerBase = NULL;

            if (timerIORequest->tr_node.io_Device != NULL) CloseDevice((struct IORequest *)timerIORequest);

            DeleteIORequest((struct IORequest *)timerIORequest);
            timerIORequest = NULL;
         }

         if (timerport)
         {
            DeleteMsgPort(timerport);
            timerport = NULL;
         }
      }

      FreeArgs(WFP_rda);
   }
   else
   {
      printf("WaitForPort: Bad Arguments .. Use 'WaitForPort ?' for correct useage\n");
   }

   if (AROSTCP_Port) return RETURN_OK;

   return RETURN_WARN;
}
