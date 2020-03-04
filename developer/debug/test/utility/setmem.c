/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Test the utility.library V50 SetMem function
*/

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/utility.h>

#include <exec/rawfmt.h>

#include <stdio.h>
#include <string.h>

const char *result_ok = "Ok!";
const char *result_fail = "FAILED!";
struct Device* TimerBase;
struct IORequest timereq;

VOID test_setmemvalidity(char *buffer, char val, ULONG size)
{
    const char *testresult = result_ok;
    int i;

    printf("\nfilling buffer @ 0x%p with 0x%02x (%d bytes)\n", buffer, val, size);
    SetMem(buffer, val, size);
    printf("checking results ... ");
    for (i = 0; i < sizeof(buffer); i ++)
    {
        if (buffer[i] != val)
            testresult = result_fail;
    }
    printf("%s\n", testresult);
}

const UBYTE MSG_MEM_G[] = "GB";
const UBYTE MSG_MEM_M[] = "MB";
const UBYTE MSG_MEM_K[] = "KB";
const UBYTE MSG_MEM_B[] = "Bytes";

///FmtSizeToString()
static void FmtSizeToString(UBYTE *buf, UQUAD num)
{
    UQUAD       d;
    const UBYTE       *ch;
    struct
    {
        ULONG    val;
        ULONG    dec;
    } array =
    {
        num,
        0
    };

    if (num >= 1073741824)
    {
        //Gigabytes
        array.val = num >> 30;
        d = ((UQUAD)num * 10 + 536870912) / 1073741824;
        array.dec = d % 10;
        ch = MSG_MEM_G;
    }
    else if (num >= 1048576)
    {
        //Megabytes
        array.val = num >> 20;
        d = ((UQUAD)num * 10 + 524288) / 1048576;
        array.dec = d % 10;
        ch = MSG_MEM_M;
    }
    else if (num >= 1024)
    {
        //Kilobytes
        array.val = num >> 10;
        d = (num * 10 + 512) / 1024;
        array.dec = d % 10;
        ch = MSG_MEM_K;
    }
    else
    {
        //Bytes
        array.val = num;
        array.dec = 0;
        d = 0;
        ch = MSG_MEM_B;
    }

    if (!array.dec && (d > array.val * 10))
    {
        array.val++;
    }

    RawDoFmt(array.dec ? "%lu.%lu" : "%lu", (RAWARG)&array, RAWFMTFUNC_STRING, buf);

    while (*buf)
    {
        buf++;
    }

    sprintf(buf, " %s", ch);
}
///

int main(void)
{
    TEXT buffer[128];

    printf("AROS utility.library v50 SetMem test\n\xA9 The AROS Development Team. All rights reserved.\n");

    printf("\nPerforming output validtity tests ...\n");

    test_setmemvalidity(buffer, 'o', sizeof(buffer));
    test_setmemvalidity(&buffer[1], 0, sizeof(buffer) - 1);
    test_setmemvalidity(buffer, 0xFF, sizeof(buffer));

    if (!OpenDevice("timer.device", 0, &timereq, 0))
    {
        ULONG tmp, testsize = (10 * 1024 * 1024) << 1;
        struct timeval start, resulttime;
        char *perfbuffer;
        int count, iter = 10;

        TimerBase = timereq.io_Device;

        perfbuffer = AllocMem(testsize, MEMF_ANY);

        printf("\nPerforming performance tests ...\n");

        while (testsize > 1)
        {
            printf("\nFilling %d bytes @ 0x%p x%d...  ", testsize, perfbuffer, iter);
            GetSysTime(&start);
            Forbid();
            for (count = 0; count < iter; count ++)
            {
                SetMem(perfbuffer, 0, testsize);
            }
            Permit();
            GetSysTime(&resulttime);
            SubTime(&resulttime, &start);
            tmp = (resulttime.tv_secs*1000000 + resulttime.tv_micro);
            printf("%uus\n", tmp);
            if (tmp == 0)
                tmp = 1;
            FmtSizeToString(buffer, (1000000000 / tmp) * (iter * testsize));
            printf("~ %s/s\n", buffer);

            printf("\nFilling %d bytes @ %p x%d...  ", testsize - 1, &perfbuffer[1], iter);
            GetSysTime(&start);
            Forbid();
            for (count = 0; count < iter; count ++)
            {
                SetMem(&perfbuffer[1], 0, testsize - 1);
            }
            Permit();
            GetSysTime(&resulttime);
            SubTime(&resulttime, &start);
            tmp = (resulttime.tv_secs*1000000 + resulttime.tv_micro);
            printf("%uus\n", tmp);
            if (tmp == 0)
                tmp = 1;
            FmtSizeToString(buffer, (1000000000 / tmp) * (iter * testsize));
            printf("~ %s/s\n", buffer);

            testsize >>= 1;
            iter <<= 1;
        }

		FreeMem(perfbuffer, (10 * 1024 * 1024) << 1);

        CloseDevice(&timereq);
    }
    printf("\nAll tests complete...\n");

    return 0;
}