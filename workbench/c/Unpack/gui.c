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

struct Window   *background = NULL;
struct Window   *window     = NULL;
struct Screen   *screen     = NULL;
struct RastPort *rp         = NULL;
WORD             width, height;

BOOL GUI_Open()
{
    screen = LockPubScreen( NULL );
    if( screen != NULL )
    {
        width  = screen->Width / 3;
        height = width / 12;
        
        background = OpenWindowTags
        (
            NULL,
            WA_Left,       0,
            WA_Top,        0,
            WA_Width,      screen->Width,
            WA_Height,     screen->Height,
            WA_Borderless, TRUE,
            TAG_DONE
        );
        
        window = OpenWindowTags
        ( 
            NULL,
            WA_Title,         (IPTR) "Unpacking...",
            WA_InnerWidth,           width,
            WA_InnerHeight,          height,
            WA_Left,                 screen->Width / 2 - width / 2,
            WA_Top,                  screen->Height / 2 - height / 2,
            WA_GimmeZeroZero,        TRUE,
            WA_Activate,             TRUE,
            WA_DragBar,              TRUE,
            WA_CustomScreen,  (IPTR) screen,
            TAG_DONE
        );        
        
        if( background != NULL && window != NULL )
        {
            SetAPen( background->RPort, 1 );
            RectFill( background->RPort, 0, 0, screen->Width, screen->Height );
        
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
    if( background != NULL ) CloseWindow( background );
    if( screen != NULL ) UnlockPubScreen( NULL, screen );
    
    CloseWorkBench();
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
