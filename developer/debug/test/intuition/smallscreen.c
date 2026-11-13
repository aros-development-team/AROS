// Script to expose a problem with Compositor on hosted.
// If you open a screen which is smaller than the workbench you can
// move the workbench screen above the limit of the X11 Window.

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <stdlib.h>

static struct Window *window;
static struct Screen *screen;
static struct Screen *wbscreen;

static void clean_exit(CONST_STRPTR s);
static void draw_stuff(void);
static void handle_events(void);

int main(void)
{
    wbscreen = LockPubScreen(NULL);
    
    // open a small screen
    screen = OpenScreenTags(NULL,
        SA_Width, 320,
        SA_Height, 200,
        SA_Type, CUSTOMSCREEN,
        TAG_END);

    if (! screen) clean_exit("Can't open screen\n");
    
    window = OpenWindowTags(NULL,
        WA_Width,         300,
        WA_Height,        200,
        WA_Activate,      TRUE,
        WA_CloseGadget,   TRUE,
        WA_DragBar,       TRUE,
        WA_IDCMP,         IDCMP_CLOSEWINDOW,
        WA_RMBTrap,       TRUE,
        WA_CustomScreen,  screen, // Link to screen
        TAG_END);

    if (! window) clean_exit("Can't open window\n");

    // you can either move the screens with the mouse
    // or you can move them with the following lines.
#if 0
    MoveScreen(screen, 0, 100);
    MoveScreen(wbscreen, 0, -100);
#endif

    handle_events();
    
    clean_exit(NULL);

    return 0;
}

static void handle_events(void)
{
    struct MsgPort *port = window->UserPort;

    BOOL terminated = FALSE;
    
    while (! terminated)
    {
        struct IntuiMessage *imsg;
        Wait(1L << port->mp_SigBit);

        while ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
        {
            switch (imsg->Class)
            {
                case IDCMP_CLOSEWINDOW:
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
    if (wbscreen) UnlockPubScreen(NULL, wbscreen);

    exit(0);
}
