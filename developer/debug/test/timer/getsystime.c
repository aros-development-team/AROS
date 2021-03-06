/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <devices/timer.h>

#include <proto/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

struct Device *TimerBase;
struct MsgPort *TimerMP;
struct timerequest *TimerIO;

int main(int argc, char **argv)
{
    struct timeval t1, t2;
    ULONG secs = 10;

    if (argc == 2)
        secs = atoi(argv[1]);

    printf("Waiting %u seconds\n", (unsigned int)secs);

    if (TimerMP = CreatePort(NULL, 0))
    {
        if (TimerIO = (struct timerequest *)CreateExtIO(TimerMP,
            sizeof(struct timerequest)))
        {
            if (!(OpenDevice(TIMERNAME, UNIT_MICROHZ,
                (struct IORequest *)TimerIO, 0)))
            {
                TimerBase = TimerIO->tr_node.io_Device;

                GetSysTime(&t1);

                Delay(secs * 50);

                GetSysTime(&t2);

                SubTime(&t2, &t1);
                printf("result %u secs %u micros\n",
                    (unsigned int)t2.tv_secs,
                    (unsigned int)t2.tv_micro);
                CloseDevice((struct IORequest *)TimerIO);
            }
            DeleteExtIO((struct IORequest *)TimerIO);
        }
        DeletePort(TimerMP);
    }
    return 0;
}
