/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG


#include <sys/time.h>

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <stdio.h>
#include <string.h>

#define MUIM_Test_Dummy (TAG_USER | 0x42)

/*** Instance data **********************************************************/
struct Test_DATA
{
    ULONG td_Dummy1,
          td_Dummy2,
          td_Dummy3,
          td_Dummy4;
};

/*** Methods ****************************************************************/
IPTR Test$OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message 
)
{
    struct Test_DATA *data = NULL;
        
    self = (Object *) DoSuperMethodA(CLASS, self, (Msg) message);
    if (self == NULL) goto error;
    
    data = INST_DATA(CLASS, self);
    data->td_Dummy1 = 42;
    data->td_Dummy2 = 42;
    data->td_Dummy3 = 42;
    data->td_Dummy4 = 42;
       
    return (IPTR) self;
    
error:
    
    return NULL;
}

IPTR Test$MUIM_Test_Dummy
(
    Class *CLASS, Object *self, Msg message
)
{
    struct Test_DATA *data = INST_DATA(CLASS, self);
    
    /* 
        Need to do *something* so that the compiler doesn't optimize away
        this function call completely...
    */
    data->td_Dummy1 += data->td_Dummy2; 
    
    return TRUE;
}


/*** Dispatcher *************************************************************/
BOOPSI_DISPATCHER(IPTR, Test_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW:          return Test$OM_NEW(CLASS, self, (struct opSet *) message);
        case MUIM_Test_Dummy: return Test$MUIM_Test_Dummy(CLASS, self, message);
        default:              return DoSuperMethodA(CLASS, self, message);
    }
    
    return NULL;
}

/*** Setup ******************************************************************/
struct MUI_CustomClass *Test_CLASS;

BOOL Test_Initialize()
{
    Test_CLASS = MUI_CreateCustomClass
    (
        NULL, MUIC_Notify, NULL, 
        sizeof(struct Test_DATA), Test_Dispatcher
    );

    if (Test_CLASS != NULL)
        return TRUE;
    else
        return FALSE;
}

void Test_Deinitialize()
{
    MUI_DeleteCustomClass(Test_CLASS);
}

#define TestObject BOOPSIOBJMACRO_START(Test_CLASS->mcc_Class)

/*** Main *******************************************************************/

int main()
{
    struct timeval  tv_start, 
                    tv_end;
    int             count   = 1000000;
    double          elapsed = 0.0;
    Object         *object  = NULL;
    int             i;
    
    if (!Test_Initialize()) goto error;
    
    /** Object allocation benchmark */
    
    gettimeofday(&tv_start, NULL);
    
    for(i = 0; i < count; i++)
    {    
        object = NewObject(Test_CLASS->mcc_Class, NULL, NULL);
        DisposeObject(object);
    }
    
    gettimeofday(&tv_end, NULL);
    
    elapsed = ((double)(((tv_end.tv_sec * 1000000) + tv_end.tv_usec) 
            - ((tv_start.tv_sec * 1000000) + tv_start.tv_usec)))/1000000.;
    
    printf
    (
        "Elapsed time:       %f seconds\n"
        "Number of objects:  %d\n"
        "Objects per second: %f\n"
        "Seconds per object: %f\n",
        elapsed, count, (double) count / elapsed, (double) elapsed / count
    );
    
    /** Method call benchmark */
    printf("****************************\n");
    object  = NewObject(Test_CLASS->mcc_Class, NULL, NULL);
    count  *= 20;
    gettimeofday(&tv_start, NULL);
    
    for(i = 0; i < count; i++)
    {    
        DoMethod(object, MUIM_Test_Dummy);        
    }
    
    gettimeofday(&tv_end, NULL);
    
    DisposeObject(object);
    
    elapsed = ((double)(((tv_end.tv_sec * 1000000) + tv_end.tv_usec) 
            - ((tv_start.tv_sec * 1000000) + tv_start.tv_usec)))/1000000.;
    
    printf
    (
        "Elapsed time:       %f seconds\n"
        "Number of calls:    %d\n"
        "Calls per second:   %f\n"
        "Seconds per call:   %f\n",
        elapsed, count, (double) count / elapsed, (double) elapsed / count
    );
    
    
    return 0;
    
error:
    printf("ERROR!\n");

    return 20;
}
