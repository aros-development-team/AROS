/*
    Copyright © 2002-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <libraries/mui.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "wanderer.h"

/* global variables */
Object *app;

int main(void)
{
    app = WandererObject, End;
    
    if (app != NULL)
    {
	DoMethod(app, MUIM_Application_Execute);        
	MUI_DisposeObject(app);
    }
    
    return 0;
}
