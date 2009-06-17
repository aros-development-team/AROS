/*
 * RequestString
 *
 * A program which requests a string from the user and prints it on
 * the standard output.
 *
 * Written by Peter Bengtsson who does not agree to be held responsible
 * for any damage or loss resulting from the use of this program.
 *
 * Use it as you wish for any and all nice things you can think of.
 *
 * $Id$
 *
 */

/******************************************************************************
 
 
    NAME
 
        RequesString [STRING] [TEXT] [TITLE] [NOGADS] [WIDTH] [SAFE] [PERSIST]
                     [ENCRYPT] [COMPARE] [PUBSCREEN]
 
    SYNOPSIS
 
        STRING, TEXT/K, TITLE/K, NOGADS/S, WIDTH/N, SAFE/S, PERSIST/S,
        ENCRYPT/S, COMPARE/K, PUBSCREEN/K
 
    LOCATION
 
        C:
 
    FUNCTION

        Shows a requester with a string gadget for user input.

    INPUTS
 
        STRING     -- Initial content of string gadget.
        TEXT       -- Label string.
        TITLE      -- Title string of requester. This also adds dragbar, closegadget
                      and a depthgadget.
        NOGADS     -- Suppress gadgets when TITLE argument is given.
        WIDTH      -- Minimal width as number of characters.
        SAFE       -- Hide user input with "*".
        PERSIST    -- Intuition is blocked until requester is quitted.
        ENCRYPT    -- Encrypt result before returning. Requires that one of these
                      environment variables is set: USER, USERNAME or LOGIN.
        COMPARE    -- If the input string is not equal to the argument
                      of COMPARE return WARN.
        PUBSCREEN  -- Open requester on given pubscreen.

    RESULT
 
    NOTES
        TODO: improve layout

    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
******************************************************************************/

#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <intuition/sghooks.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <clib/alib_protos.h>

#include <string.h>


AROS_UFP3(ULONG, HookFunc,
    AROS_UFPA(struct Hook *,   TheHook, A0),
    AROS_UFPA(struct SGWork *, Obj,     A2),
    AROS_UFPA(UBYTE *,         Mess,    A1));


#define  TEMPLATE "STRING,TEXT/K,TITLE/K,NOGADS/S,WIDTH/N,SAFE/S,PERSIST/S,ENCRYPT/S,COMPARE/K,PUBSCREEN/K"

#define  NArgs 10          /* Number of arguments */

#define  STRING      0
#define  TEXT        1
#define  TITLE       2
#define  NOGADS      3
#define  WIDTH       4
#define  SAFE        5
#define  PERSIST     6
#define  ENCRYPT     7
#define  COMPARE     8
#define  PUBSCREEN   9

#define  MINWIDTH    8     /* Minimum width of gadget in characters */

const char version[] = "\0$VER: RequestString 39.5 (16.06.2009)";

AROS_UFH3(__startup static int, RequestString,
	  AROS_UFHA(char *, argstr, A0),
	  AROS_UFHA(ULONG, argsize, D0),
	  AROS_UFHA(struct ExecBase *, sBase, A6))
{
    AROS_USERFUNC_INIT

    struct DosLibrary       *DOSBase = NULL;
    struct Library          *GadToolsBase = NULL;
    struct IntuitionBase    *IntuitionBase = NULL;
    struct GfxBase          *GfxBase = NULL;

    IPTR                    ArgList[NArgs];
    ULONG                   MsgClass = 0;
    APTR                    VisInfo = NULL;
    char                    CBuffer[12], UName[48];
    struct TextExtent       TExtent;
    struct Hook             SGHook = { {0} , 0, 0, 0};
    struct RDArgs           *Args = NULL;
    struct ExtIntuiMessage  *EIMess = NULL;
    struct Gadget           *Gad = NULL, *GList = NULL;
    struct Window           *Win = NULL;
    struct Screen           *Scr = NULL;
    char                    *ReturnText = NULL;

    int                     WWidth = 0, WHeight = 0;
    int                     GWidth = 32, GHeight = 1;
    long                    TXPos = 0, TYPos = 0;

    LONG                    Ret = 20;

    struct TagItem WindowTags[] =
    {
        {WA_PubScreen,   0},
        {WA_Gadgets,     0},
        {WA_Left,        0},
        {WA_Top,         0},
        {WA_Width,       0},
        {WA_Height,      0},
        {WA_DragBar,     FALSE},
        {WA_DepthGadget, FALSE},
        {WA_CloseGadget, FALSE},
        {WA_Title,       0},
        {WA_IDCMP,       IDCMP_GADGETUP | IDCMP_INACTIVEWINDOW | IDCMP_ACTIVEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_CLOSEWINDOW},
        {WA_SizeGadget,  FALSE},
        {WA_Activate,    TRUE}
    };

    struct NewGadget StringGad = {10, 5, 80, 15, NULL, NULL, 0, 0, NULL, NULL};

    if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 39)) != NULL)
    {
        if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)) != NULL)
        {
            if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)) != NULL)
            {
                if ((GadToolsBase = OpenLibrary("gadtools.library", 39)) != NULL)
                {
                    memset(ArgList, 0, sizeof(ArgList)); /* Clear the ArgList array */

                    Args = ReadArgs(TEMPLATE, ArgList, NULL);  /* How do we check for failure? */
                    if ((Scr = LockPubScreen((UBYTE *)ArgList[PUBSCREEN])) != NULL)
                    {
                        if ((VisInfo=GetVisualInfo(Scr, NULL)) != NULL)
                        {
                            if ((Gad = CreateContext(&GList)) != NULL)
                            {
                                if (ArgList[WIDTH] != 0)
                                    GWidth = *((LONG *)ArgList[WIDTH]);
                                if (GWidth < 8)
                                    GWidth = 8;

                                GHeight = GHeight * Scr->RastPort.TxHeight + 6;
                                WHeight = GHeight + 8;

                                do 
                                {
                                    GWidth = GWidth * Scr->RastPort.TxWidth + 4;
                                    WWidth = GWidth + 20;
                                    if (WWidth > Scr->Width)
                                        GWidth = (Scr->Width - 20) / (Scr->RastPort.TxWidth) - 1;
                                }
                                 while (WWidth > Scr->Width);

                                /* Forbid()/Permit() around this following poking in GfxBase? */

                                FontExtent(GfxBase->DefaultFont, &TExtent);

                                if (ArgList[TEXT] != 0)
                                {
                                    WHeight += (TExtent.te_Height);
                                    if ((TExtent.te_Width * strlen((char *)ArgList[TEXT]) + 20) > WWidth)
                                        WWidth = (TExtent.te_Width * strlen((char *)ArgList[TEXT]) + 20);
                                }

                                /* If the window has a title, it will also have a dragbar, closegadget
                                 * and a depthgadget.
                                 */
                                if (ArgList[TITLE] != 0)
                                {
                                    WindowTags[6].ti_Data = TRUE;
                                    if (ArgList[NOGADS] == 0)
                                    {
                                        WindowTags[7].ti_Data = TRUE;
                                        WindowTags[8].ti_Data = TRUE;
                                    }
                                    WindowTags[9].ti_Data = ArgList[TITLE];
                                    WHeight += Scr->RastPort.TxHeight; /* Should be Win->BorderTop, see also note below. */
                                }

                                StringGad.ng_VisualInfo = VisInfo;
                                StringGad.ng_Width = GWidth;
                                StringGad.ng_Height = GHeight;
                                StringGad.ng_TopEdge = WHeight - (GHeight + 4); /* The '4' should be replaces by Win->BorderTop but how? The window is not yet opened. */
                                StringGad.ng_LeftEdge = (WWidth - GWidth) >> 1;

                                /* Initialize the Hook if we are using a safe stringgadget.
                                 * SGHook.d_Data points to storage space for the string which
                                 * will be returned. Memory should be cleared.
                                 */

                                if (ArgList[SAFE] != 0)
                                {
                                    SGHook.h_Entry = (HOOKFUNC)HookFunc;
                                    /* SGHook.h_SubEntry=NULL; */
                                    SGHook.h_Data = AllocVec(98, MEMF_ANY | MEMF_CLEAR); /* Should be made dynamic if possible */
                                    if (ArgList[STRING] != 0)
                                    {
                                        int i = -1;
                                        strcpy((char *)SGHook.h_Data, (char *)ArgList[STRING]);
                                        while (((char *)ArgList[STRING])[++i] != 0)
                                            ((char *)ArgList[STRING])[i] = '*';
                                    }
                                }

                                if
                                (
                                    (
                                        Gad = CreateGadget
                                        (
                                            STRING_KIND,
                                            Gad, &StringGad,
                                            GTST_EditHook, (ArgList[SAFE] == 0) ? NULL : &SGHook, GTST_String, ArgList[STRING],
                                            TAG_END
                                        )
                                    ) != NULL
                                )
                                {
                                    /* Screen Gadget */
                                    WindowTags[0].ti_Data = (IPTR)Scr;
                                    WindowTags[1].ti_Data = (IPTR)GList;
                                    /* Width & Height */
                                    WindowTags[4].ti_Data = WWidth;
                                    WindowTags[5].ti_Data = WHeight;
                                    /* X & Y Pos */
                                    WindowTags[2].ti_Data = (Scr->Width-WWidth) >> 1;
                                    WindowTags[3].ti_Data = (Scr->Height-WHeight) >> 1;

                                    if ((Win = OpenWindowTagList(NULL, WindowTags)) != NULL)
                                    {
                                        /* Is there some (un)informative text in the window? */

                                        if (ArgList[TEXT] != 0)
                                        {
                                            /* Colour should rather be selected with thougt to the background of the window, fix this. */
                                            SetAPen(Win->RPort, 1);
                                            TXPos = (WWidth - TextLength(Win->RPort, (char *)ArgList[TEXT], strlen((char *)ArgList[TEXT]))) >> 1;
                                            TYPos = GfxBase->DefaultFont->tf_Baseline + Win->BorderTop + 1;
                                            Move(Win->RPort, TXPos, TYPos);
                                            Text(Win->RPort, (char *)ArgList[TEXT], strlen((char *)ArgList[TEXT]));
                                        }

                                        do
                                        {
                                            WaitPort(Win->UserPort);
                                            EIMess = (struct ExtIntuiMessage *)GT_GetIMsg(Win->UserPort);
                                            MsgClass=EIMess->eim_IntuiMessage.Class;
                                            GT_ReplyIMsg((struct IntuiMessage *)EIMess);

                                            switch(MsgClass)
                                            {
                                                case  IDCMP_ACTIVEWINDOW   :
                                                    ActivateGadget(Gad, Win, NULL);
                                                    break;

                                                case  IDCMP_INACTIVEWINDOW :
                                                    if (ArgList[PERSIST])
                                                    {
                                                        ScreenToFront(Win->WScreen);
                                                        WindowToFront(Win);
                                                        ActivateWindow(Win);
                                                    }
                                                    break;

                                                case  IDCMP_REFRESHWINDOW  :
                                                    GT_BeginRefresh(Win);
                                                    if (ArgList[TEXT] != 0)
                                                    {
                                                        Move(Win->RPort, TXPos, TYPos);
                                                        Text(Win->RPort, (char *)ArgList[TEXT], strlen((char *)ArgList[TEXT]));
                                                    }
                                                    GT_EndRefresh(Win,TRUE);
                                                    break;

                                            }
                                        }
                                        while ((MsgClass != IDCMP_GADGETUP) && (MsgClass != IDCMP_CLOSEWINDOW));

                                        GT_GetGadgetAttrs(Gad, Win, NULL, GTST_String, &ReturnText, TAG_END);

                                        if (ArgList[SAFE] != 0)
                                            ReturnText = (char *)SGHook.h_Data;

                                        /* Try to find out who the user is if we are to encrypt the output.   */
                                        /* I really don't know how to acquire the username, but this might    */
                                        /* be a good guess of how to do it.                                   */

                                        if (ArgList[ENCRYPT] != 0)
                                        {
                                            if (GetVar("USER", UName, 47, 0) == -1)
                                                if (GetVar("USERNAME", UName, 47, 0) == -1)
                                                    if (GetVar("LOGIN", UName, 47, 0) == -1)
                                                        UName[0] = 0;
                                            ACrypt(CBuffer, ReturnText, UName);
                                            ReturnText = CBuffer;
                                        }

                                        Printf("\"%s\"\n", ReturnText);

                                        /* Here follows the COMPARE parameter. If the input string is not equal
                                         * to the argument of COMPARE we return WARN.
                                         */

                                        if (ArgList[COMPARE] != 0)
                                            Ret = (strcmp(ReturnText, (char *)ArgList[COMPARE])) ? RETURN_WARN : 0;
                                        else
                                            Ret = 0;

                                    }
                                }
                            }
                        }
                    }
                    else if (ArgList[PUBSCREEN] != 0)
                        Ret = 10;  // FIXME: should we fallback to default pubscreen?

                    /* Clean up. */
                    if (SGHook.h_Data != NULL)
                        FreeVec(SGHook.h_Data);
                    if (Args)
                        FreeArgs(Args);
                    if (Win)
                        CloseWindow(Win);
                    if (GList)
                        FreeGadgets(GList);
                    if (VisInfo)
                        FreeVisualInfo(VisInfo);
                    if (Scr)
                        UnlockPubScreen(NULL,Scr);

                    CloseLibrary(GadToolsBase);
                }
                CloseLibrary((struct Library *)IntuitionBase);
            }
            CloseLibrary((struct Library *)GfxBase);
        }
        CloseLibrary((struct Library *)DOSBase);
    }
    return Ret;

    AROS_USERFUNC_EXIT
}


/* EditHook function for safe stringgadgets */

AROS_UFH3(ULONG, HookFunc,
    AROS_UFHA(struct Hook *, TheHook, A0),
    AROS_UFHA(struct SGWork *, Obj, A2),
    AROS_UFHA(UBYTE *, Mess, A1))
{
    AROS_USERFUNC_INIT

    int   i0,i1;
    char  * const Tmp=(char *)TheHook->h_Data;

    if (Mess[0] == SGH_KEY)
    {
        Obj->Actions |= SGA_USE;

        switch(Obj->EditOp)
        {
            case  EO_DELBACKWARD :
                i0 = Obj->StringInfo->BufferPos;
                i1 = Obj->StringInfo->NumChars - Obj->NumChars;
                strcpy(&Tmp[i0 - i1], &Tmp[i0]);
                break;

            case  EO_DELFORWARD :
                i0 = Obj->StringInfo->BufferPos;
                i1 = Obj->StringInfo->NumChars - Obj->NumChars;
                strcpy(&Tmp[i0], &Tmp[i0 + i1]);
                break;

            case  EO_ENTER :
                /* Exit handling. */
                break;

            case  EO_RESET :
                Tmp[0] = 0;
                break;

            case  EO_INSERTCHAR :
                if (strlen(Tmp) > 96)
                    Obj->Actions &= ~SGA_USE;
                i0 = Obj->StringInfo->BufferPos;
                Obj->WorkBuffer[i0] = '*';
                i1 = strlen(Tmp) + 1;
                while (i1-- != i0)
                    Tmp[i1 + 1] = Tmp[i1];
                Tmp[i0] = Obj->Code;
                break;

            case  EO_CLEAR :
                Tmp[0] = 0;
                break;

            case  EO_BIGCHANGE : /* Not Allowed */
            case  EO_UNDO :
            case  EO_SPECIAL : 
                Obj->Actions &= ~SGA_USE;
                break;
        }
        return ~0;
    }
    else
    {
        Obj->Actions &= ~ SGA_USE;
        return 0;
    }
    
    return 0;

    AROS_USERFUNC_EXIT
}
