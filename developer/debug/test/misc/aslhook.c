/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    Test for FRF_INTUIFUNC. Activating of the window "ASL hook" triggers
    the callback function. Note that the function used for ASL_HookFunc isn't
    a Hook function. It's a function whose arguments are given on the stack.
*/

#include <exec/types.h>
#include <intuition/intuition.h>
#include <dos/dosasl.h>
#include <libraries/asl.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <proto/intuition.h>
#include <proto/intuition.h>

#include <stdio.h>

static struct Window  *window;

static IPTR func(ULONG mask, APTR object, struct FileRequester *fr)
{
    struct IntuiMessage *msg;
    printf("hookfunc mask %u object %p filerequester %p\n", (unsigned int)mask, object, fr);
    switch (mask)
    {
        case FRF_INTUIFUNC:
            msg = (struct IntuiMessage *)object;
            printf("intuimsg msg %p, class %u code %d\n", msg, (unsigned int)msg->Class, msg->Code);
            return (IPTR)object;
            break;
        case FRF_FILTERFUNC:
            printf("filterfunc: %s\n", ((struct AnchorPath *)object)->ap_Info.fib_FileName);
            return 0;
            break;
    }
    return 0;
}

int main(void)
{
    struct FileRequester *fr;

    if (window = OpenWindowTags(NULL,
        WA_Title, "ASL hook",
        WA_Width, 300,
        WA_Height, 200,
        WA_IDCMP, IDCMP_ACTIVEWINDOW,
        WA_Flags, WFLG_DEPTHGADGET,
        TAG_END))
    {
        if (fr = AllocFileRequest())
        {
            if (AslRequestTags(fr,
                ASL_Dir, "RAM:",
                ASL_Window, window,
                ASL_Hail, "Test",
                ASL_HookFunc, func,
                ASLFR_Flags1, FRF_FILTERFUNC | FRF_INTUIFUNC,
                TAG_END))
            {
                printf("path %s file %s\n", fr->rf_Dir, fr->rf_File);
            }
            FreeFileRequest(fr);
        }
        CloseWindow(window);
    }
    return 0;
}
