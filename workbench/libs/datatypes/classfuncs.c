/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/intuition.h>
#include <intuition/classusr.h>
#include "datatypes_intern.h"

AROS_UFP3(IPTR, Dispatcher,
	  AROS_UFHA(Class *, class, A0),
	  AROS_UFHA(Object *, object, A2),
	  AROS_UFHA(Msg, msg, A1));

BOOL InstallClass(struct Library *DataTypesBase)
{
    BOOL Success = FALSE;

    if((GPB(DataTypesBase)->dtb_DataTypesClass = MakeClass(DATATYPESCLASS, 
						    GADGETCLASS, NULL,
						    sizeof(struct DTObject),
						    0)))
    {
	GPB(DataTypesBase)->dtb_DataTypesClass->cl_Dispatcher.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(Dispatcher);
	GPB(DataTypesBase)->dtb_DataTypesClass->cl_UserData = (IPTR)DataTypesBase;
	
	AddClass(GPB(DataTypesBase)->dtb_DataTypesClass);
	
	Success = TRUE;
    }
    
    if(!Success)
	TryRemoveClass(DataTypesBase);
    
    return Success;
}


BOOL TryRemoveClass(struct Library *DataTypesBase)
{
   if(GPB(DataTypesBase)->dtb_DataTypesClass != NULL)
   {
       if(FreeClass(GPB(DataTypesBase)->dtb_DataTypesClass))
	   GPB(DataTypesBase)->dtb_DataTypesClass = NULL;
   }

   return GPB(DataTypesBase)->dtb_DataTypesClass == NULL;
}
