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

int main()
{
    struct timeval  tv_start, 
                    tv_end;
    int             count   = 100000000;
    double          elapsed = 0.0;
    int             i;
    APTR            pool;
    APTR            memory;
    
    pool = CreatePool(MEMF_ANY, 4 * 100, 100);
    AllocPooled(pool, 100); // Avoid bad behaviour of FreePooled()
    
    gettimeofday(&tv_start, NULL);
    
    for(i = 0; i < count; i++)
    {    
        memory = AllocPooled(pool, 100);
        if (memory) FreePooled(pool, memory, 100);
    }
    
    gettimeofday(&tv_end, NULL);
    
    DeletePool(pool);
    
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
