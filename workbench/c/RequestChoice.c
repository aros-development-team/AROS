/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
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

        C:

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

        intuition.library/EasyRequestArgs()

    INTERNALS

    HISTORY

******************************************************************************/

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

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

const TEXT version[] = "$VER: RequestChoice 41.3 (3.4.2014)\n";

static char ERROR_HEADER[] = "RequestChoice";

static int Do_RequestChoice(STRPTR, STRPTR, STRPTR *, STRPTR);
static STRPTR FilterBodyText(STRPTR Body);
static STRPTR ComposeGadgetText(STRPTR * Gadgets);

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
    STRPTR              BodyText;
    LONG                Result;
    IPTR                args[1];
    int                 Return_Value;

    Return_Value     = RETURN_OK;
    Result           = 0L;

    GadgetText = ComposeGadgetText(Gadgets);
    if (!GadgetText)
        return RETURN_FAIL;

    BodyText = FilterBodyText(Body);
    if (!BodyText)
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
        ChoiceES.es_TextFormat   = BodyText;
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
            PrintFault(ERROR_NO_FREE_STORE, ERROR_HEADER);
        }
        UnlockPubScreen(NULL, Scr);
    }
    else
    {
        Return_Value = RETURN_FAIL;
        PrintFault(ERROR_NO_FREE_STORE, ERROR_HEADER);
    }

    FreeVec(GadgetText);
    FreeVec(BodyText);

    return Return_Value;

} /* Do_RequestChoice */


// Combine gadget strings and separate them by "|".
// Replace all "%" by "%%" because EasyRequest
// uses printf-like formatting.
static STRPTR ComposeGadgetText(STRPTR * Gadgets)
{
    STRPTR GadgetText, BufferPos;
    int    GadgetLength = 0;
    int    CurrentGadget;
    int    PercentCnt = 0;
    int    StrLen;
    int    i;

    for (CurrentGadget = 0; Gadgets[CurrentGadget]; CurrentGadget++)
    {
        StrLen = strlen(Gadgets[CurrentGadget]);
        GadgetLength +=  StrLen + 1;
        
        // Count "%"
        for (i = 0; i < StrLen; i++)
        {
            if (Gadgets[CurrentGadget][i] == '%')
            {
                PercentCnt++;
            }
        }
    }

    GadgetLength += PercentCnt;

    GadgetText = AllocVec(GadgetLength, MEMF_ANY);
    if (!GadgetText)
    {
        PrintFault(ERROR_NO_FREE_STORE, ERROR_HEADER);
        return NULL;
    }

    BufferPos = GadgetText;
    for (CurrentGadget = 0; Gadgets[CurrentGadget]; CurrentGadget++)
    {
        int LabelLength = strlen(Gadgets[CurrentGadget]);

        for (i = 0; i < LabelLength; i++)
        {
            if (Gadgets[CurrentGadget][i] == '%')
            {
                *BufferPos = '%';
                BufferPos++;
                *BufferPos = '%';
                BufferPos++;
            }
            else
            {
                *BufferPos = Gadgets[CurrentGadget][i];
                BufferPos++;
            }
        }
        if (Gadgets[CurrentGadget + 1])
        {
            *BufferPos = '|';
            BufferPos++;
        }
        else
            *BufferPos = '\0';
    }

    return GadgetText;

} /* ComposeGadgetText */


// Replace % by %% because EasyRequest uses
// printf-like formatting characters.
// Result string must be freed with FreeVec().
static STRPTR FilterBodyText(STRPTR Body)
{
    STRPTR GadgetText;
    int    GadgetLength = 0;
    int    StrLen;
    int    PercentCnt = 0;
    int    i, j;

    if (!Body)
        return NULL;

    // Count % characters
    StrLen = strlen(Body);
    for (i = 0; i < StrLen; i++)
    {
        if (Body[i] == '%')
        {
            PercentCnt++;
        }
    }

    GadgetLength = StrLen + PercentCnt + 1;

    GadgetText = AllocVec(GadgetLength, MEMF_ANY);
    if (!GadgetText)
    {
        PrintFault(IoErr(), ERROR_HEADER);
        return NULL;
    }

    for (i = 0, j = 0; i < StrLen; i++)
    {
        if (Body[i] == '%')
        {
            GadgetText[j++] = '%';
            GadgetText[j++] = '%';
        }
        else
        {
            GadgetText[j++] = Body[i];
        }
    }

    GadgetText[j] = '\0';

    return GadgetText;

} /* FilterBodyText */
