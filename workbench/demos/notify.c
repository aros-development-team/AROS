/*
    Copyright © 2001, The AROS Development Team. All rights reserved.
    $Id$

    Test program for ram.handler notification.
*/

#include <proto/exec.h>
#include <exec/memory.h>
#include <stdio.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/notify.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdlib.h>


int main(int argc, char* argv[])
{
    struct NotifyRequest *nr = AllocVec(sizeof(struct NotifyRequest),
					MEMF_CLEAR);
    BPTR lock;

    if (nr == NULL)
    {
	printf("Could not allocate memory.\n");
	exit(1);
    }

    nr->nr_Name = "Olle";
    nr->nr_Flags = NRF_SEND_SIGNAL;
    nr->nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
    nr->nr_stuff.nr_Signal.nr_SignalNum = SIGB_SINGLE;
    
    lock = Lock("Ram Disk:", SHARED_LOCK);

    if (lock == NULL)
    {
	printf("Could not lock Ram Disk:\n");
	exit(1);
    }

    CurrentDir(lock);

    StartNotify(nr);

    printf("Waiting for notification\n");

    Wait(SIGF_SINGLE);

    printf("Got notification!\n");

    return 0;
}

