/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/mui.h>

#include <zune/aboutwindow.h>

#include <stdlib.h>
#include <stdio.h>

#include "locale.h"
#include "aboutaros.h"

VOID Cleanup(CONST_STRPTR error)
{
    Locale_Deinitialize();
    AboutAROS_Deinitialize();
    
    if (error != NULL)
    {
        PutStr(error);
        PutStr("\n");
        exit(20);
    }
    else
    {
        exit(0);
    }
}

int main()
{
    Object *application;

    if (!Locale_Initialize()) Cleanup("Locale!");
    if (!AboutAROS_Initialize()) Cleanup("Classes!");
    
    if ((application = AboutAROSObject, End) != NULL)
    {
        DoMethod(application, MUIM_Application_Execute);
        
        MUI_DisposeObject(application);
    }
    else
    {
        Cleanup("Objects!");
    }
    
    Cleanup(NULL);

    return 0; /* keep compiler happy */
}
