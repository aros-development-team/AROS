/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/time.h>
#include <stdio.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>

#define BUFFERSIZE (10*1024*1024) // 10 MB
#define FUNCTION   memset

int main()
{
    struct timeval  tv_start, 
                    tv_end;
    APTR            buffer = NULL;
    UBYTE           patterns[] = { 0x15, 0x25, 0x65, 0x42 };
    int             count   = 300;
    double          elapsed = 0.0;
    int             i;
    
    buffer = AllocVec(BUFFERSIZE, MEMF_ANY);
    
    gettimeofday(&tv_start, NULL);
    
    for(i = 0; i < count; i++)
    {
        FUNCTION(buffer, 0, BUFFERSIZE);
    }
    
    gettimeofday(&tv_end, NULL);
    
    elapsed = ((double)(((tv_end.tv_sec * 1000000) + tv_end.tv_usec) 
            - ((tv_start.tv_sec * 1000000) + tv_start.tv_usec)))/1000000.;
    
    printf
    (
        "Clearing memory\n"
        "---------------\n"
        "Elapsed time:     %f seconds\n"
        "Number of bytes:  %f\n"
        "Bytes per second: %f\n",
        elapsed, (double) count * BUFFERSIZE, 
        (double) count * BUFFERSIZE / elapsed
    );
   
    gettimeofday(&tv_start, NULL);
    
    for(i = 0; i < count; i++)
    {
        FUNCTION(buffer, patterns[i % sizeof(patterns)], BUFFERSIZE);
    }
    
    gettimeofday(&tv_end, NULL);
    
    elapsed = ((double)(((tv_end.tv_sec * 1000000) + tv_end.tv_usec) 
            - ((tv_start.tv_sec * 1000000) + tv_start.tv_usec)))/1000000.;
    
    printf
    (
        "Filling memory with pattern\n"
        "---------------------------\n"
        "Elapsed time:     %f seconds\n"
        "Number of bytes:  %f\n"
        "Bytes per second: %f\n",
        elapsed, (double) count * BUFFERSIZE, 
        (double) count * BUFFERSIZE / elapsed
    );
    FreeVec(buffer);
   
    return 0;
}
