/*
    Copyright © 2002-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <libraries/mui.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "locale.h"
#include "wanderer.h"

/* global variables */
Object *app;

/* Don't output errors to the console, open requesters instead */
int __forceerrorrequester = 1;

int main(void)
{
    Locale_Initialize();
    
    if ((app = WandererObject, End) != NULL)
    {
	DoMethod(app, MUIM_Application_Execute);        
	MUI_DisposeObject(app);
    }
    
    Locale_Deinitialize();
    
    return 0;
}
