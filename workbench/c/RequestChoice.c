/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: RequestChoice CLI command
    Lang: english
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

        EasyRequest(), EasyRequestArgs()

    INTERNALS

    HISTORY

        27-Jul-1997     laguest     Initial inclusion into the AROS tree

******************************************************************************/

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>   /* This causes a spilled register error */

#include <dos/dos.h>
#include <dos/rdargs.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>

#define ARG_TEMPLATE    "TITLE/A,BODY/A,GADGETS/A/M,PUBSCREEN/K"
#define ARG_TITLE       0
#define ARG_BODY        1
#define ARG_GADGETS     2
#define ARG_PUBSCREEN   3
#define TOTAL_ARGS      4

/* To define whether a command line switch was set or not.
 */
#define NOT_SET         0

struct IntuitionBase * IntuitionBase;

static const char version[] = "$VER: RequestChoice 41.0 (02.07.1997)\n";

int Do_RequestChoice(STRPTR, STRPTR, STRPTR *, STRPTR);

int main(int argc, char *argv[])
{
    struct RDArgs * rda;
    IPTR          * args[TOTAL_ARGS] = { NULL, NULL, NULL, NULL };
    int             Return_Value;

    Return_Value = RETURN_OK;

    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39L);
    if (IntuitionBase != NULL)
    {
        rda = ReadArgs(ARG_TEMPLATE, (LONG *)args, NULL);
        if (rda) {
            Return_Value = Do_RequestChoice((STRPTR)args[ARG_TITLE],
                                            (STRPTR)args[ARG_BODY],
                                            (STRPTR *)args[ARG_GADGETS],
                                            (STRPTR)args[ARG_PUBSCREEN]
            );
        } else {
            PrintFault(IoErr(), "RequestChoice");
            Return_Value = RETURN_FAIL;
        }

        FreeArgs(rda);

        CloseLibrary((struct Library *)IntuitionBase);
    }
    else
    {
        VPrintf("Need \'intuition.library\' version 39 or above\n", NULL);
        Return_Value = RETURN_FAIL;
    }

    return (Return_Value);

} /* main */


void ComposeGadgetText(STRPTR *, char *);

int Do_RequestChoice(STRPTR   Title,
                     STRPTR   Body,
                     STRPTR * Gadgets,
                     STRPTR   PubScreen)
{
    struct Screen     * Scr;
    struct EasyStruct   ChoiceES;
    char                GadgetText[512];
    LONG                Result;
    IPTR                args[1];
    int                 Return_Value;

    Return_Value     = RETURN_OK;
    Result           = 0L;

    ComposeGadgetText(Gadgets, &GadgetText[0]);

    /* Make sure we can open the requester on the specified screen.
     *
     * If the PubScreen argument is not specified it will contain
     * NULL, and hence open on the Workbench screen.
     */
    Scr = (struct Screen *)LockPubScreen((UBYTE *)PubScreen);
    if (Scr != NULL)
    {
        ChoiceES.es_StructSize   = sizeof(struct EasyStruct);
        ChoiceES.es_Flags        = 0;
        ChoiceES.es_Title        = Title;
        ChoiceES.es_TextFormat   = Body;
        ChoiceES.es_GadgetFormat = &GadgetText[0];

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
            PrintFault(IoErr(), NULL);
        }
    }
    else
    {
        Return_Value = RETURN_FAIL;
        SetIoErr(ERROR_NO_FREE_STORE);
        PrintFault(IoErr(), NULL);
    }

    UnlockPubScreen(PubScreen, Scr);

    return (Return_Value);

} /* Do_RequestChoice */


void ComposeGadgetText(STRPTR * Gadgets,
                       char   * Buffer)
{
    LONG Length;
    int  BufferLength;

    BufferLength  = 0L;
    Buffer[0]     = NULL;

    /* Compose the es_GadgetFormat field.
     *
     * This function will take each argument in turn and produce a
     * string of the form "G1|G2|...|Gn".
     */
    while (*Gadgets != NULL)
    {
        Length = strlen(*Gadgets);

        CopyMem((APTR)*Gadgets, (APTR)&Buffer[BufferLength], Length);

        BufferLength += Length;

        if (*(Gadgets + 1) != NULL)
        {
            Buffer[BufferLength] = '|';

            BufferLength++;
        }
        else
        {
            /* Place a NULL at the end of the array.
             *
             * This stops EasyRequest() from displaying too
             * many characters.
             */

            Buffer[BufferLength] = NULL;

            BufferLength++;
        }

        Gadgets++;
    }    

} /* ComposeGadgetText */
