/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/keymap.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <intuition/intuition.h>
#include <exec/libraries.h>

#include <stdio.h>

#define BUFSIZE 10
VOID HandleEvents(struct Window *win, UBYTE *buffer, LONG bufsize);

struct IntuitionBase *IntuitionBase;
struct Library *KeymapBase;

int main (int argc, char **argv)
{
    UBYTE buffer[BUFSIZE];
    
    KeymapBase = OpenLibrary("keymap.library", 37);
    if (!KeymapBase)
        printf("Failed to open keymap.library v37\n");
    else
    {
        IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
        if (!IntuitionBase)
            printf("Failed to open intuition.library v37\n");
        else
        {
            struct Window *win;
            
            win = OpenWindowTags(NULL,
                WA_Width,    200,
                WA_Height,    60,
                WA_Title,    (IPTR)"Maprawkey test (ESC to exit)",
                WA_Flags,    WFLG_ACTIVATE,
                WA_IDCMP,    IDCMP_RAWKEY,
                TAG_DONE);
                
            if (!win)
                printf("Could not open window\n");
            else
            {
                HandleEvents(win, buffer, BUFSIZE);
                
                CloseWindow(win);
            }
            CloseLibrary((struct Library *)IntuitionBase);
        }
        CloseLibrary(KeymapBase);
    }
    return (0);
}

/*******************
**  HandleEvents  **
*******************/
VOID HandleEvents(struct Window *win, UBYTE *buffer, LONG bufsize)
{
    struct IntuiMessage *imsg;
    struct MsgPort *port = win->UserPort;
    BOOL terminated = FALSE;
    WORD written;
    struct InputEvent ie ={0,};
    
    ie.ie_Class = IECLASS_RAWKEY;
    
    while (!terminated)
    {
        if ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
        {
            
            switch (imsg->Class)
            {
            
            case IDCMP_REFRESHWINDOW:
                BeginRefresh(win);
                EndRefresh(win, TRUE);
                break;
                
            case IDCMP_RAWKEY:
                ie.ie_Code     = imsg->Code;
                ie.ie_Qualifier    = imsg->Qualifier;
                printf("rawkey: code=$%04x, qual=$%04x\n",
                    ie.ie_Code, ie.ie_Qualifier);
                
                ie.ie_EventAddress =  imsg->IAddress;
                
                written = MapRawKey(&ie, buffer, bufsize, NULL);
                if (written == -1)
                    printf("Buffer owerflow !!\n");
                else if (written)
                {
                    printf("Map:");
                    Write(Output(), buffer, written);
                    printf("\n");
                }
                if (ie.ie_Code == 197)
                    terminated = TRUE;
                break;
                                    
            } /* switch (imsg->Class) */
            ReplyMsg((struct Message *)imsg);
            
                        
        } /* if ((imsg = GetMsg(port)) != NULL) */
        else
        {
            Wait(1L << port->mp_SigBit);
        }
    } /* while (!terminated) */
    
    return;
} /* HandleEvents() */


