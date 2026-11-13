/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Display driver loader for Picasso96 '.card' board drivers.
*/

/*
    p96gfx.hidd scans exec's library list for opened '.card' drivers when it
    is initialised. In ROM it is initialised long before dos.library exists,
    so a card sitting in LIBS: can never be seen. Card drivers additionally
    tend to need dos.library and intuition.library in their FindCard() vector,
    so they cannot be brought up any earlier either.

    This monitor file closes the gap the same way the SAGA one does: it runs
    from DEVS:Monitors once DOS is up, opens the requested '.card' by name and
    only then opens p96gfx.hidd from DEVS:Drivers, whose card scan now finds
    it and registers a display driver for it.

    Both libraries are deliberately left open - the display driver keeps
    referencing them for as long as the machine is up.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <dos/dosextens.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <oop/oop.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/oop.h>
#include <string.h>

#define P96GFX_NAME  "p96gfx.hidd"
#define P96GFX_CLASS "hidd.gfx.p96gfx"
#define P96GFX_CARDDIR "Picasso96/"
#define P96GFX_CARDEXT ".card"

struct MyArgs
{
    STRPTR card;
};

extern struct WBStartup *WBenchMsg;

int __nocommandline = 1;

int main(void)
{
    BPTR olddir = BNULL;
    STRPTR myname;
    TEXT cardname[128];
    BOOL deduced = FALSE;
    struct DiskObject *icon = NULL;
    struct RDArgs *rdargs = NULL;
    struct Library *cardbase;
    struct MyArgs args = {NULL};
    int res = RETURN_OK;

    if (WBenchMsg)
    {
        olddir = CurrentDir(WBenchMsg->sm_ArgList[0].wa_Lock);
        myname = WBenchMsg->sm_ArgList[0].wa_Name;
    }
    else
    {
        struct Process *me = (struct Process *)FindTask(NULL);

        if (me->pr_CLI)
        {
            struct CommandLineInterface *cli = BADDR(me->pr_CLI);

            myname = AROS_BSTR_ADDR(cli->cli_CommandName);
        }
        else
            myname = me->pr_Task.tc_Node.ln_Name;
    }
    D(bug("[P96Monitor] command name: %s\n", myname));

    icon = GetDiskObject(myname);
    if (icon)
    {
        args.card = FindToolType(icon->do_ToolTypes, "CARD");
    }

    if (!WBenchMsg)
    {
        /* An absent /K leaves the array alone, so the icon still wins */
        rdargs = ReadArgs("CARD/K", (IPTR *)&args, NULL);
    }

    /*
        Nothing said which card to use, so take it from our own file name:
        a copy of this file renamed to Z3660 loads Picasso96/Z3660.card with
        nothing to configure. FilePart() because a shell may hand us the path
        it was invoked with, not just the name.
    */
    if (!args.card)
    {
        STRPTR base = FilePart(myname);

        if (strlen(base) + sizeof(P96GFX_CARDDIR P96GFX_CARDEXT) <= sizeof(cardname))
        {
            strcpy(cardname, P96GFX_CARDDIR);
            strcat(cardname, base);
            strcat(cardname, P96GFX_CARDEXT);
            args.card = cardname;
            deduced = TRUE;
        }
    }

    D(bug("[P96Monitor] CARD=%s%s\n", args.card ? (char *)args.card : "<none>",
          deduced ? " (from file name)" : ""));

    /*
        A board reached through the Picasso96 boot ROM vector is already
        driven by the ROM resident copy of p96gfx, which registers the class
        as it goes. Opening a '.card' on top of that would put a second
        driver on the same hardware, so leave a live class well alone.
    */
    if (OOP_FindClass(P96GFX_CLASS))
    {
        D(bug("[P96Monitor] %s already registered, nothing to do\n", P96GFX_CLASS));
        args.card = NULL;
    }

    if (args.card)
    {
        cardbase = OpenLibrary(args.card, 0);
        D(bug("[P96Monitor] '%s' 0x%p\n", args.card, cardbase));

        if (cardbase)
        {
            /* Card is on the library list now, so the scan will pick it up */
            if (!OpenLibrary(P96GFX_NAME, 0))
            {
                CloseLibrary(cardbase);
                res = RETURN_ERROR;
                PrintFault(IoErr(), myname);
            }
        }
        else if (!deduced)
        {
            /*
                A card asked for by name and not there is worth complaining
                about; a guess that missed is not. Under its shipped name
                this file has no card of its own, which is how it sits in
                DEVS:Monitors doing nothing until it is copied and renamed.
            */
            res = RETURN_ERROR;
            PrintFault(IoErr(), myname);
        }
    }

    if (rdargs)
        FreeArgs(rdargs);
    if (icon)
        FreeDiskObject(icon);
    if (olddir)
        CurrentDir(olddir);

    return res;
}
