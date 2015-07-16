/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    Example for custom screen
    
    This time we are setting the colors directly.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <stdlib.h>

static struct Window *window;
static struct Screen *screen;
static struct RastPort *rp;

static void clean_exit(CONST_STRPTR s);
static void draw_stuff(void);
static void handle_events(void);

/*
    Initial color values for the screen.
    Must be an array with index, red, green, blue for each color.
    The range for each color component is between 0 and 255.
*/
static struct ColorSpec colors[] =
{
    {0, 240, 100, 0},  // Color 0 is background
    {1, 240, 0, 0},
    {2, 0, 0, 240},
    {-1}               // Array must be terminated with -1
};


int main(void)
{
    screen = OpenScreenTags(NULL,
        SA_Width,  800,
        SA_Height, 600,
        SA_Depth,  16,
        SA_Colors, colors,
        TAG_END);

    if (! screen) clean_exit("Can't open screen\n");
    
    window = OpenWindowTags(NULL,
        WA_Activate,      TRUE,
        WA_Borderless,    TRUE,
        WA_Backdrop,      TRUE,
        WA_IDCMP,         IDCMP_VANILLAKEY,
        WA_RMBTrap,       TRUE,
        WA_NoCareRefresh, TRUE,   // We don't want to listen to refresh messages
        WA_CustomScreen,  screen, // Link to screen
        TAG_END);

    if (! window) clean_exit("Can't open window\n");
    
    rp = window->RPort;
    
    draw_stuff();
    
    handle_events();
    
    clean_exit(NULL);

    return 0;
}

static void draw_stuff(void)
{
    SetAPen(rp, 1);
    Move(rp, 100, 50);
    Text(rp, "Press any key to quit", 21); 
    
    Move(rp, 100, 100);
    Draw(rp, 500, 100);

    SetAPen(rp, 2);
    Move(rp, 100, 200);
    Draw(rp, 500, 200);
    
    /*
        We can change single colors with SetRGB32() or a range of
        colors with LoadRGB32(). In contrast to the color table above
        we need 32 bit values for the color components.
    */
    SetRGB32(&screen->ViewPort, 2, 0, 0xFFFFFFFF, 0);
    
    /*
        Even when we use the same pen number as before we have to
        set it again.
    */
    SetAPen(rp, 2);
    Move(rp, 100, 300);
    Draw(rp, 500, 300);
}


static void handle_events(void)
{
    struct IntuiMessage *imsg;
    struct MsgPort *port = window->UserPort;

    BOOL terminated = FALSE;
    
    while (! terminated)
    {
        Wait(1L << port->mp_SigBit);

        while ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
        {
            switch (imsg->Class)
            {
                case IDCMP_VANILLAKEY:
                    terminated = TRUE;
                    break;
            }
            ReplyMsg((struct Message *)imsg);
        }
    }
}


static void clean_exit(CONST_STRPTR s)
{
    if (s) PutStr(s);

    // Give back allocated resourses
    if (window) CloseWindow(window);
    if (screen) CloseScreen(screen);

    exit(0);
}
