/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "common.c"

/*** Main *******************************************************************/
int main()
{
    struct timeval  tv_start, 
                    tv_end;
    int             count   = 100000000;
    double          elapsed = 0.0;
    Object         *object  = NULL;
    int             i;
    
    if (!Test_Initialize()) goto error;
    
    object  = NewObject(Test_CLASS->mcc_Class, NULL, NULL);
    
    GetSysTime(&tv_start);
    
    for(i = 0; i < count; i++)
    {    
        DoMethod(object, MUIM_Test_Dummy);        
    }
    
    GetSysTime(&tv_end);
    
    DisposeObject(object);
    
    elapsed = ((double)(((tv_end.tv_secs * 1000000) + tv_end.tv_micro) 
            - ((tv_start.tv_secs * 1000000) + tv_start.tv_micro)))/1000000.;
    
    printf
    (
        "Elapsed time:       %f seconds\n"
        "Number of calls:    %d\n"
        "Calls per second:   %f\n"
        "Seconds per call:   %f\n",
        elapsed, count, (double) count / elapsed, (double) elapsed / count
    );
    
    Test_Deinitialize();
    
    return 0;
    
error:
    printf("Could not initialize Test class!\n");

    return 20;
}
