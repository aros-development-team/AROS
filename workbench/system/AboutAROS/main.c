/*
    Copyright © 2003-2004, The AROS Development Team. All rights reserved.
    This file is part of the About program, which is distributed under
    the terms of version 2 of the GNU General Public License.
    
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

int __nocommandline       = 1;
int __forceerrorrequester = 1;

int main()
{
    Object *application;

    if ((application = AboutAROSObject, End) != NULL)
    {
        DoMethod(application, MUIM_Application_Execute);
        MUI_DisposeObject(application);
    }

    return 0;
}
