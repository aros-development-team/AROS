
#include <proto/intuition.h>
#include <intuition/classusr.h>
#include "datatypes_intern.h"

extern ULONG Dispatcher(Class *class, Object *object, Msg msg);

BOOL InstallClass(struct Library *DataTypesBase)
{
    BOOL Success = FALSE;

    if((GPB(DataTypesBase)->dtb_DataTypesClass = MakeClass(DATATYPESCLASS, 
						    GADGETCLASS, NULL,
						    sizeof(struct DTObject),
						    0)))
    {
	GPB(DataTypesBase)->dtb_DataTypesClass->cl_Dispatcher.h_Entry = (ULONG (*)())&Dispatcher;
	GPB(DataTypesBase)->dtb_DataTypesClass->cl_UserData = (ULONG)DataTypesBase;
	
	AddClass(GPB(DataTypesBase)->dtb_DataTypesClass);
	
	Success = TRUE;
    }
    
    if(!Success)
	TryRemoveClass(DataTypesBase);
    
    return Success;
}


BOOL TryRemoveClass(struct Library *DataTypesBase)
{
   BOOL Success = FALSE;
   
   if(GPB(DataTypesBase)->dtb_DataTypesClass != NULL)
   {
       if((Success = FreeClass(GPB(DataTypesBase)->dtb_DataTypesClass)))
	   GPB(DataTypesBase)->dtb_DataTypesClass = NULL;
   }

   return Success;
}
