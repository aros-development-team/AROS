/*
 * Copyright (c) 1992 Commodore-Amiga, Inc.
 * 
 * This example is provided in electronic form by Commodore-Amiga, Inc. for 
 * use with the "Amiga ROM Kernel Reference Manual: Devices", 3rd Edition, 
 * published by Addison-Wesley (ISBN 0-201-56775-X).
 * 
 * The "Amiga ROM Kernel Reference Manual: Devices" contains additional 
 * information on the correct usage of the techniques and operating system 
 * functions presented in these examples.  The source and executable code 
 * of these examples may only be distributed in free electronic form, via 
 * bulletin board or as part of a fully non-commercial and freely 
 * redistributable diskette.  Both the source and executable code (including 
 * comments) must be included, without modification, in any copy.  This 
 * example may not be published in printed form or distributed with any
 * commercial product.  However, the programming techniques and support
 * routines set forth in these examples may be used in the development
 * of original executable software products for Commodore Amiga computers.
 * 
 * All other rights reserved.
 * 
 * This example is provided "as-is" and is subject to change; no
 * warranties are made.  All use is at your own risk. No liability or
 * responsibility is assumed.
 *
 *****************************************************************************
 *
 *  Multiple_Timers.c
 *
 *  This program is designed to do multiple (3) time requests using one
 *  OpenDevice.  It creates a message port - TimerMP, creates an
 *  extended I/O structure of type timerequest named TimerIO[0] and
 *  then uses that to open the device.  The other two time request
 *  structures - TimerIO[1] and TimerIO[2] - are created using AllocMem
 *  and then copying TimerIO[0] into them.  The tv_secs field of each
 *  structure is set and then three SendIOs are done with the requests.
 *  The program then goes into a while loop until all messages are received.
 *
 * Compile with SAS C 5.10  lc -b1 -cfistq -v -y -L
 *
 * Run from CLI only
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <devices/timer.h>

#include <proto/exec.h>
#include <clib/alib_protos.h>

#include <stdio.h>

#ifdef LATTICE
int CXBRK(void) { return(0); }  /* Disable Lattice CTRL/C handling */
int chkabort(void) { return(0); }  /* really */
#endif

int main(void)
{
struct timerequest *TimerIO[3];
struct MsgPort *TimerMP;
struct Message *TimerMSG;

ULONG error,x,seconds[3]={4,1,2}, microseconds[3]={0,0,0};

int allin = 3;
char *position[]={"last","second","first"};

if ((TimerMP = CreatePort(0,0)))
    {
    if ((TimerIO[0] = (struct timerequest *)
                      CreateExtIO(TimerMP,sizeof(struct timerequest))))
        {
            /* Open the device once */
        if (!(error=OpenDevice( TIMERNAME, UNIT_VBLANK,(struct IORequest *) TimerIO[0], 0L)))
            {
            /* Set command to TR_ADDREQUEST */
            TimerIO[0]->tr_node.io_Command = TR_ADDREQUEST;

            if ((TimerIO[1]=(struct timerequest *)
                    AllocMem(sizeof(struct timerequest),MEMF_PUBLIC | MEMF_CLEAR)))
                {
                if ((TimerIO[2]=(struct timerequest *)
                       AllocMem(sizeof(struct timerequest),MEMF_PUBLIC | MEMF_CLEAR)))
                    {
                    /* Copy fields from the request used to open the timer device */
                    *TimerIO[1] = *TimerIO[0];
                    *TimerIO[2] = *TimerIO[0];

                    /* Initialize other fields */
                    for (x=0;x<3;x++)
                        {
                        TimerIO[x]->tr_time.tv_secs   = seconds[x];
                        TimerIO[x]->tr_time.tv_micro  = microseconds[x];
                        printf("\nInitializing TimerIO[%ld]",x);
                        }

                    printf("\n\nSending multiple requests\n\n");

                    /* Send multiple requests asynchronously */
                    /* Do not got to sleep yet...            */
                    SendIO((struct IORequest *)TimerIO[0]);
                    SendIO((struct IORequest *)TimerIO[1]);
                    SendIO((struct IORequest *)TimerIO[2]);

                    /* There might be other processing done here */

                    /* Now go to sleep with WaitPort() waiting for the requests */
                    while (allin)
                          {
                          WaitPort(TimerMP);
                          /* Get the reply message */
                          TimerMSG=GetMsg(TimerMP);
                          for (x=0;x<3;x++)
                              if (TimerMSG==(struct Message *)TimerIO[x])
                                  printf("Request %ld finished %s\n",x,position[--allin]);
                          }

                    FreeMem(TimerIO[2],sizeof(struct timerequest));
                    }

                else
                    printf("Error: could not allocate TimerIO[2] memory\n");

                FreeMem(TimerIO[1],sizeof(struct timerequest));
                }

            else
                printf("Error could not allocate TimerIO[1] memory\n");

            CloseDevice((struct IORequest *) TimerIO[0]);
            }

        else
            printf("\nError: Could not OpenDevice\n");

        DeleteExtIO((struct IORequest *) TimerIO[0]);
        }

    else
        printf("Error: could not create IORequest\n");

    DeletePort(TimerMP);
    }

else
    printf("\nError: Could not CreatePort\n");

    return 0;
}
