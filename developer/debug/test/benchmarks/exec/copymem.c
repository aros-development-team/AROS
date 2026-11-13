/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <sys/time.h>
#include <stdio.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>

static void bench_copymem_block(LONG blocksize)
{
    struct timeval  tv_start,
                    tv_end;
    QUAD            count = 0;
    double          elapsed = 0.0;
    APTR            srcmem, dstmem;
    const LONG      BUFSIZE = 10 * 1024 * 1024;
    const LONG      BENCHTIME = 10;
    const LONG      maxoffset = BUFSIZE - blocksize - 1;
    LONG            srcoffset = 0;
    LONG            dstoffset = blocksize;
    LONG            offsetstep = 4000;

    srcmem = AllocMem(BUFSIZE, MEMF_PUBLIC);
    dstmem = AllocMem(BUFSIZE, MEMF_PUBLIC);

    gettimeofday(&tv_start, NULL);

    while(1)
    {
        if (count % 32768 == 0)
        {
            gettimeofday(&tv_end, NULL);
            if (tv_end.tv_sec - tv_start.tv_sec > BENCHTIME)
                break;
        }

        /* Randomize read and write location */
        srcoffset += offsetstep;
        if (srcoffset > maxoffset) srcoffset -= maxoffset;
        dstoffset += offsetstep;
        if (dstoffset > maxoffset) dstoffset -= maxoffset;

        CopyMem((APTR)((IPTR)srcmem + srcoffset), (APTR)((IPTR)dstmem + dstoffset), blocksize);

        count++;
    }


    elapsed = ((double)(((tv_end.tv_sec * 1000000) + tv_end.tv_usec)
            - ((tv_start.tv_sec * 1000000) + tv_start.tv_usec)))/1000000.0;

    FreeMem(srcmem, BUFSIZE);
    FreeMem(dstmem, BUFSIZE);

    printf
    (
        "Block size:            %d bytes\n"
        "Elapsed time:          %f seconds\n"
        "Number of copies:      %ld\n"
        "MBs per second:        %f\n"
        "Milliseconds per copy: %f\n",
        blocksize, elapsed , count,
        (double) count * blocksize / elapsed / (1024 * 1024), (double) elapsed * 1000.0 / count
    );
}

int main()
{

    bench_copymem_block(100);
    bench_copymem_block(1000);
    bench_copymem_block(10000);
    bench_copymem_block(100000);
    bench_copymem_block(1000000);

    return 0;
}
