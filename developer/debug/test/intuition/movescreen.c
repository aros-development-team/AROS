// calling MoveScreen() on a screen which was created with SA_Behind
// crashes on linux_x86_64

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <stdlib.h>

static struct Window *window;
static struct Screen *screen;

static void clean_exit(CONST_STRPTR s);
static void draw_stuff(void);
static void handle_events(void);

int main(void)
{
    // open a screen with SA_Behind
    screen = OpenScreenTags(NULL,
        SA_Behind, TRUE,
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

    // the following crashes on hosted
    MoveScreen(screen, 0, 1);
    
    handle_events();
    
    clean_exit(NULL);

    return 0;
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

    exit(0);
}
