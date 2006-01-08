/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RequestChoice CLI command
    Lang: English
*/

/*****************************************************************************

    NAME

        RequestChoice

    SYNOPSIS

        TITLE/A,BODY/A,GADGETS/A/M,PUBSCREEN/K

    LOCATION

        Workbench:c

    FUNCTION

        Allows AmigaDOS scripts to have access to the EasyRequest() function
        for input.

    INPUTS

        TITLE       - The text to display in the title bar of the requester.

        BODY        - The text to display in the body of the requester.

        GADGETS     - The text for each of the buttons.

        PUBSCREEN   - The name of the public screen to open the requester
                      upon.

    RESULT

        Standard DOS return codes.

    NOTES

        To place a newline into the body of the requester use *n or *N.

        To place a quotation mark in the body of the requester use *".

        The CLI template gives the GADGETS option as ALWAYS given; this
        is different from the original program. This way, we do not have
        to check to see if the gadgets have been given.

    EXAMPLE

        RequestChoice "This is a title" "This is*Na body" Okay|Cancel

            This is self-explanitory, except for the "*N". This is the
            equivalent of using a '\n' in C to get a newline in the body
            of the requester. This requester will open on the Workbench
            screen.

        RequestChoice Title="This is a title" Body="This is*Na body"
                      Gadgets=Okay|Cancel PubScreen=DOPUS.1

            This will do exactly the same as before except that it will
            open on the Directory Opus public screen.

    BUGS

    SEE ALSO

        intuition.library/EasyRequest(), intuition.library/EasyRequestArgs()

    INTERNALS

    HISTORY

        08-Sep-1997     srittau     Use dynamic buffer for gadget-labels
                                    Small changes/fixes

        27-Jul-1997     laguest     Initial inclusion into the AROS tree

******************************************************************************/

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>   /* This causes a spilled register error */

#include <dos/dos.h>
#include <dos/rdargs.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>

#include <string.h>

#define ARG_TEMPLATE    "TITLE/A,BODY/A,GADGETS/A/M,PUBSCREEN/K"
#define ARG_TITLE       0
#define ARG_BODY        1
#define ARG_GADGETS     2
#define ARG_PUBSCREEN   3
#define TOTAL_ARGS      4

/* To define whether a command line switch was set or not.
 */
#define NOT_SET         0

static const char version[] = "$VER: RequestChoice 41.1 (08.09.1997)\n";

static char ERROR_HEADER[] = "RequestChoice";

int Do_RequestChoice(STRPTR, STRPTR, STRPTR *, STRPTR);

int __nocommandline;

int main(void)
{
    struct RDArgs * rda;
    IPTR          * args[TOTAL_ARGS] = { NULL, NULL, NULL, NULL };
    int             Return_Value;

    Return_Value = RETURN_OK;

    rda = ReadArgs(ARG_TEMPLATE, (IPTR *)args, NULL);
    if (rda)
    {
	Return_Value = Do_RequestChoice((STRPTR)args[ARG_TITLE],
                                        (STRPTR)args[ARG_BODY],
                                        (STRPTR *)args[ARG_GADGETS],
                                        (STRPTR)args[ARG_PUBSCREEN]);
	FreeArgs(rda);
    }
    else
    {
	PrintFault(IoErr(), ERROR_HEADER);
        Return_Value = RETURN_FAIL;
    }

    return (Return_Value);
} /* main */


STRPTR ComposeGadgetText(STRPTR *);

int Do_RequestChoice(STRPTR   Title,
                     STRPTR   Body,
                     STRPTR * Gadgets,
                     STRPTR   PubScreen)
{
    struct Screen     * Scr;
    struct EasyStruct   ChoiceES;
    STRPTR              GadgetText;
    LONG                Result;
    IPTR                args[1];
    int                 Return_Value;

    Return_Value     = RETURN_OK;
    Result           = 0L;

    GadgetText = ComposeGadgetText(Gadgets);
    if (!GadgetText)
        return RETURN_FAIL;

    /* Make sure we can open the requester on the specified screen.
     *
     * If the PubScreen argument is not specified it will contain
     * NULL, and hence open on the Workbench screen.
     */
    Scr = (struct Screen *)LockPubScreen((UBYTE *)PubScreen);
    if (Scr != NULL)
    {
        ChoiceES.es_StructSize   = sizeof(struct EasyStruct);
        ChoiceES.es_Flags        = 0L;
        ChoiceES.es_Title        = Title;
        ChoiceES.es_TextFormat   = Body;
        ChoiceES.es_GadgetFormat = GadgetText;

        /* Open the requester.
         */
        Result = EasyRequestArgs(Scr->FirstWindow, &ChoiceES, NULL, NULL);
        if (Result != -1)
        {
            args[0] = (IPTR)Result;

            VPrintf("%ld\n", args);
        }
        else
        {
            Return_Value = RETURN_FAIL;
            SetIoErr(ERROR_NO_FREE_STORE);
            PrintFault(IoErr(), ERROR_HEADER);
        }
        UnlockPubScreen(NULL, Scr);
    }
    else
    {
        Return_Value = RETURN_FAIL;
        SetIoErr(ERROR_NO_FREE_STORE);
        PrintFault(IoErr(), ERROR_HEADER);
    }

    FreeVec(GadgetText);

    return (Return_Value);

} /* Do_RequestChoice */


STRPTR ComposeGadgetText(STRPTR * Gadgets)
{
    STRPTR GadgetText, BufferPos;
    int    GadgetLength = 0;
    int    CurrentGadget;

    for (CurrentGadget = 0; Gadgets[CurrentGadget]; CurrentGadget++)
    {
        GadgetLength += strlen(Gadgets[CurrentGadget]) + 1;
    }

    GadgetText = AllocVec(GadgetLength, MEMF_ANY);
    if (!GadgetText)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        PrintFault(IoErr(), ERROR_HEADER);
        return NULL;
    }

    BufferPos = GadgetText;
    for (CurrentGadget = 0; Gadgets[CurrentGadget]; CurrentGadget++)
    {
        int LabelLength = strlen(Gadgets[CurrentGadget]);
        CopyMem(Gadgets[CurrentGadget], BufferPos, LabelLength);
        if (Gadgets[CurrentGadget + 1])
        {
            BufferPos[LabelLength] = '|';
            BufferPos += LabelLength + 1;
        }
        else
            BufferPos[LabelLength] = '\0';
    }

    return GadgetText;

} /* ComposeGadgetText */
