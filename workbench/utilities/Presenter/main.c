/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <dos/dos.h>
#include <libraries/mui.h>

#include "presenter.h"

int main(void)
{
    Object *application;
    
    if ((application = PresenterObject, End) != NULL)
    {
        DoMethod(application, MUIM_Application_Execute);
        MUI_DisposeObject(application);
    }
    
    return 0;
}
