/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include "gui.h"

struct Window   *window = NULL;
struct Screen   *screen = NULL;
struct RastPort *rp     = NULL;
WORD             width, height;

BOOL GUI_Open()
{
    screen = LockPubScreen( NULL );
    if( screen != NULL )
    {
        width  = screen->Width / 3;
        height = width / 12;
        
        window = OpenWindowTags
        ( 
            NULL, 
            WA_Title,         "Unpacking...",
            WA_InnerWidth,    width,
            WA_InnerHeight,   height,
            WA_Left,          screen->Width / 2 - width / 2,
            WA_Top,           screen->Height / 2 - height / 2,
            WA_GimmeZeroZero, TRUE,
            WA_Activate,      TRUE,
            WA_DragBar,       TRUE,
            TAG_END           
        );        
        
        if( window != NULL )
        {
            rp = window->RPort;
            SetAPen( rp, 3 );
            
            return TRUE;
        }
    }
    
    GUI_Close();
    
    return FALSE;
}

void GUI_Close()
{
    if( window != NULL ) CloseWindow( window );
    if( screen != NULL ) UnlockPubScreen( NULL, screen );
}

void GUI_Update( LONG position, LONG max )
{
    static WORD oldx = 0;
    WORD        newx;
    
    newx = (position * width) / max;
    if( newx > oldx )
    {
        RectFill( rp, oldx, 0, newx, height );
        oldx = newx;
    }
}
