/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include "modes.h"
#include "package.h"
#include "gui.h"

int main()
{
    if( !GUI_Open() ) goto error;
    
    APTR pkg = PKG_Open( "/test.pkg", MODE_READ );
    if( pkg == NULL ) goto error;
    
    PKG_ExtractEverything( pkg );
    
    PKG_Close( pkg );

error:
    GUI_Close();

    return 0;
}

