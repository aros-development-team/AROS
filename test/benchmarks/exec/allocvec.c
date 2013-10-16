/*
    Copyright © 2003-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/time.h>
#include <stdio.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>

int main()
{
    struct timeval  tv_start, 
                    tv_end;
    int             count   = 10000000;
    double          elapsed = 0.0;
    int             i;
    APTR            memory;
    
    gettimeofday(&tv_start, NULL);
    
    for(i = 0; i < count; i++)
    {    
        memory = AllocVec(100, MEMF_ANY);
        FreeVec(memory);
    }
    
    gettimeofday(&tv_end, NULL);
    
    elapsed = ((double)(((tv_end.tv_sec * 1000000) + tv_end.tv_usec) 
            - ((tv_start.tv_sec * 1000000) + tv_start.tv_usec)))/1000000.;
    
    printf
    (
        "Elapsed time:           %f seconds\n"
        "Number of allocations:  %d\n"
        "Allocations per second: %f\n"
        "Seconds per allocation: %f\n",
        elapsed, count, (double) count / elapsed, (double) elapsed / count
    );
   
    return 0;
}
