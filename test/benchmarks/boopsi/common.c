/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <sys/time.h>
#include <stdio.h>

#include <string.h>

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

/*** Instance data **********************************************************/
struct Test_DATA
{
    ULONG td_Dummy1,
          td_Dummy2,
          td_Dummy3,
          td_Dummy4;
};

/*** Methods ****************************************************************/
#define MUIM_Test_Dummy (TAG_USER | 0x42)

Object *Test__OM_NEW
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
       
    return self;
    
error:
    
    return NULL;
}

IPTR Test__MUIM_Test_Dummy
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
        case OM_NEW:          return (IPTR) Test__OM_NEW(CLASS, self, (struct opSet *) message);
        case MUIM_Test_Dummy: return Test__MUIM_Test_Dummy(CLASS, self, message);
        default:              return DoSuperMethodA(CLASS, self, message);
    }
    
    return (IPTR) NULL;
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

    if (Test_CLASS != NULL) return TRUE;
    else                    return FALSE;
}

void Test_Deinitialize()
{
    MUI_DeleteCustomClass(Test_CLASS);
}

#define TestObject BOOPSIOBJMACRO_START(Test_CLASS->mcc_Class)
