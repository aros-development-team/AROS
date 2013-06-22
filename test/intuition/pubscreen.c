// Opens a PubScreen. The name can be given as argument.
// Default name is "MYPUBSCREEN".

#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdlib.h>
#include <stdio.h>

struct Screen *myscreen;
struct Window *mywindow;
BYTE signalnum;

void clean_exit(STRPTR txt)
{
    if (mywindow) CloseWindow(mywindow);
    if (myscreen) CloseScreen(myscreen);
    FreeSignal(signalnum);

    if (txt)
    {
        PutStr(txt);
        exit(RETURN_FAIL);
    }
    exit(RETURN_OK);
}

int main(int argc, char **argv)
{
    BOOL done = FALSE;
    BOOL closewindow = FALSE;
    ULONG signals, winsig, pubsig;
    struct IntuiMessage *message;
    char *name;

    if (argc == 2)
    {
        name = argv[1];
    }
    else
    {
        name = "MYPUBSCREEN";
    }

    if ((signalnum = AllocSignal(-1)) == -1)
    {
        clean_exit("Can't allocate signal\n");
    }

    if
    (
        (
            myscreen = OpenScreenTags
            (
                NULL,
                SA_PubName, name,
                SA_PubSig, signalnum,
                SA_LikeWorkbench, TRUE,
                SA_Title, name,
                TAG_DONE
            )
        ) == NULL
    )
    {
        clean_exit("Can't open screen\n");
    }

    if
    (
        (
            mywindow = OpenWindowTags
            (
                NULL,
                WA_Left, 30,
                WA_Top, 30,
                WA_Width, 250,
                WA_Height, 100,
                WA_DragBar, TRUE,
                WA_DepthGadget, TRUE,
                WA_CloseGadget, TRUE,
                WA_Activate, TRUE,
                WA_Title, "Close me to close the screen",
                WA_IDCMP, IDCMP_CLOSEWINDOW,
                WA_PubScreen, myscreen,
                TAG_DONE
            )
        ) == NULL
    )
    {
        clean_exit("Can't open window\n");
    }

    if ((PubScreenStatus(myscreen, 0) & 1) == 0)
    {
        clean_exit("Can't make screen public");
    }

    winsig = 1L << mywindow->UserPort->mp_SigBit;
    pubsig = 1L << signalnum;

    while (!done)
    {
        signals = Wait(winsig | pubsig);

        if (mywindow && (signals & winsig))
        {
            while (NULL != (message = (struct IntuiMessage *)GetMsg(mywindow->UserPort)))
            {
                if (message->Class == IDCMP_CLOSEWINDOW)
                {
                    closewindow = TRUE;
                }
                ReplyMsg((struct Message *)message);
            }
        }
        if (signals & pubsig)
        {
            if (PubScreenStatus(myscreen, PSNF_PRIVATE) & 1)
            {
                done = TRUE;
            }
            else
            {
                PutStr("Failed to make screen private\n");
            }
        }

        if (closewindow)
        {
            if (mywindow) CloseWindow(mywindow);
            winsig = 0;
            mywindow = NULL;
            closewindow = FALSE;
        }
    }

    clean_exit(NULL);
    return 0;
}
