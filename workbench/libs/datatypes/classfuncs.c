
#include <proto/intuition.h>
#include <intuition/classusr.h>
#include "datatypes_intern.h"

extern ULONG Dispatcher(Class *class, Object *object, Msg msg);

BOOL InstallClass(struct Library *DTBase)
{
    BOOL Success = FALSE;

    if((GPB(DTBase)->dtb_DataTypesClass = MakeClass(DATATYPESCLASS, 
						    GADGETCLASS, NULL,
						    sizeof(struct DTObject),
						    0)))
    {
	GPB(DTBase)->dtb_DataTypesClass->cl_Dispatcher.h_Entry = (ULONG (*)())&Dispatcher;
	GPB(DTBase)->dtb_DataTypesClass->cl_UserData = (ULONG)DTBase;
	
	AddClass(GPB(DTBase)->dtb_DataTypesClass);
	
	Success = TRUE;
    }
    
    if(!Success)
	TryRemoveClass(DTBase);
    
    return Success;
}


BOOL TryRemoveClass(struct Library *DTBase)
{
   BOOL Success = FALSE;
   
   if(GPB(DTBase)->dtb_DataTypesClass != NULL)
   {
       if((Success = FreeClass(GPB(DTBase)->dtb_DataTypesClass)))
	   GPB(DTBase)->dtb_DataTypesClass = NULL;
   }

   return Success;
}
