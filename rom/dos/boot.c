/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Implements AROS's generic/amiga-like boot sequence.
*/

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <dos/dosextens.h>
#include <dos/cliinit.h>
#include <dos/stdio.h>
#include <utility/tagitem.h>
#include <libraries/expansionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#include "dos_intern.h"
#include "../dosboot/bootflags.h"

extern char *generate_banner(void);

#ifdef __mc68000
/*
 * Load DEVS:system-configuration only on m68k.
 * Setup pre-2.0 boot disk colors and mouse cursors (for example)
 */
#define USE_SYSTEM_CONFIGURATION

#endif

#include <intuition/screens.h>
#include <graphics/layers.h>
#include <utility/hooks.h>
#include <aros/asmcall.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#ifdef USE_SYSTEM_CONFIGURATION

static void load_system_configuration(struct DosLibrary *DOSBase)
{
    BPTR fh;
    ULONG len;
    struct Preferences prefs;
    struct Library *IntuitionBase;

    fh = Open("DEVS:system-configuration", MODE_OLDFILE);
    if (!fh)
        return;
    len = Read(fh, &prefs, sizeof prefs);
    Close(fh);
    if (len != sizeof prefs)
        return;
    IntuitionBase = TaggedOpenLibrary(TAGGEDOPEN_INTUITION);
    if (IntuitionBase)
        SetPrefs(&prefs, len, FALSE);
    CloseLibrary(IntuitionBase);
}

#else

#define load_system_configuration(DOSBase) do { } while (0)

#endif

extern void BCPL_cliInit(void);


/*
 * Display for S:Security-Startup: a black, title-less public screen "SYSTEM".
 * It becomes the default public screen, so the boot console and the login
 * window (and its requesters) open on it instead of forcing Workbench open.
 * The black comes from a backdrop window with a backfill hook - the screen's
 * pens are left alone.
 */
struct BootScrData
{
    struct Library *bsd_GfxBase;
    LONG            bsd_Pen;
};

AROS_UFH3(static void, BootScreenBackFillFunc,
    AROS_UFHA(struct Hook *,            hook,   A0),
    AROS_UFHA(struct RastPort *,        rp,     A2),
    AROS_UFHA(struct BackFillMessage *, msg,    A1))
{
    AROS_USERFUNC_INIT

    struct BootScrData *bsd = (struct BootScrData *)hook->h_Data;
    struct Library *GfxBase = bsd->bsd_GfxBase;
    struct RastPort rpc = *rp;

    rpc.Layer = NULL;
    SetAPen(&rpc, bsd->bsd_Pen);
    RectFill(&rpc, msg->Bounds.MinX, msg->Bounds.MinY, msg->Bounds.MaxX, msg->Bounds.MaxY);

    AROS_USERFUNC_EXIT
}

static struct Screen *OpenBootScreen(struct IntuitionBase *IntuitionBase, struct Library *GfxBase,
                                     struct Window **bdwin, struct Hook *hook, struct BootScrData *bsd)
{
    struct Screen *scr = OpenScreenTags(NULL,
                                        SA_PubName,       (IPTR)"SYSTEM",
                                        SA_Type,          PUBLICSCREEN,
                                        SA_LikeWorkbench, TRUE,
                                        SA_ShowTitle,     FALSE,
                                        SA_Quiet,         TRUE,
                                        TAG_DONE);
    if (scr == NULL)
        return NULL;

    bsd->bsd_GfxBase = GfxBase;
    bsd->bsd_Pen = ObtainBestPenA(scr->ViewPort.ColorMap, 0, 0, 0, NULL);
    hook->h_Entry = (HOOKFUNC)BootScreenBackFillFunc;
    hook->h_Data  = bsd;
    *bdwin = OpenWindowTags(NULL,
                            WA_CustomScreen,  (IPTR)scr,
                            WA_Left,          0,
                            WA_Top,           0,
                            WA_Width,         scr->Width,
                            WA_Height,        scr->Height,
                            WA_Borderless,    TRUE,
                            WA_Backdrop,      TRUE,
                            WA_Activate,      FALSE,
                            WA_SimpleRefresh, TRUE,
                            WA_NoCareRefresh, TRUE,
                            WA_BackFill,      (IPTR)hook,
                            TAG_DONE);
    PubScreenStatus(scr, 0);
    SetDefaultPubScreen("SYSTEM");
    return scr;
}

static void CloseBootScreen(struct DosLibrary *DOSBase, struct IntuitionBase *IntuitionBase, struct Library *GfxBase,
                            struct Screen *scr, struct Window *bdwin, struct BootScrData *bsd)
{
    LONG tries;

    SetDefaultPubScreen(NULL);
    if (bdwin)
        CloseWindow(bdwin);
    if (bsd->bsd_Pen != -1)
        ReleasePen(scr->ViewPort.ColorMap, bsd->bsd_Pen);
    for (tries = 0; tries < 50; tries++)
    {
        PubScreenStatus(scr, PSNF_PRIVATE);
        if (CloseScreen(scr))
            break;
        Delay(10);      /* a visitor window is still closing */
    }
}

void __dos_Boot(struct DosLibrary *DOSBase, ULONG BootFlags, UBYTE Flags)
{
    BPTR cis = BNULL;

    /*  We have been created as a process by DOS, we should now
        try and boot the system. */

    D(
        bug("[DOS] %s: ** starting generic boot sequence\n", __func__);
        bug("[DOS] %s: BootFlags 0x%08X Flags 0x%02X\n", __func__, BootFlags, Flags);
        bug("[DOS] %s: DOSBase @ 0x%p\n", __func__, DOSBase);
      )

    /* m68000 uses this to get the default colors and
     * cursors for Workbench
     */
    load_system_configuration(DOSBase);

    D(bug("[DOS] %s: system config loaded\n", __func__);)

    /*
     * If needed, run the display drivers loader.
     * In fact the system must have at least one resident driver,
     * which will be used for bootmenu etc. However, it we somehow happen
     * not to have it, this will be our last chance.
     */
    if ((BootFlags & (BF_NO_DISPLAY_DRIVERS | BF_NO_COMPOSITION)) != (BF_NO_DISPLAY_DRIVERS | BF_NO_COMPOSITION))
    {
        /* Check that it exists first... */
        BPTR seg;

        D(bug("[DOS] %s: initialising displays\n", __func__);)

        if ((seg = LoadSeg("C:AROSMonDrvs")) != BNULL)
        {
            STRPTR args = "";
            BPTR oldin, oldout;

            /*
             * Argument strings MUST contain terminating LF because of ReadItem() bugs.
             * Their absence causes ReadArgs() crash.
             */
            if (BootFlags & BF_NO_COMPOSITION)
                args = "NOCOMPOSITION\n";
            else if (BootFlags & BF_NO_DISPLAY_DRIVERS)
                args = "ONLYCOMPOSITION\n";

            D(bug("[DOS] %s: Running AROSMonDrvs %s\n", __func__, args);)

            /* RunCommand needs a valid Input() handle
             * for passing in its arguments.
             */
            oldin = SelectInput(Open("NIL:", MODE_OLDFILE));
            oldout= SelectOutput(Open("NIL:", MODE_NEWFILE));
            RunCommand(seg, AROS_STACKSIZE, args, strlen(args));
            SelectInput(oldin);
            SelectOutput(oldout);

            /* We don't care about the return code */
            UnLoadSeg(seg);
        }
    }

    /*
     * Multi-user: with security.library in the ROM, S:Security-Startup runs
     * before the Startup-Sequence. It performs the login and prepares the
     * assigns for the per-user settings. It gets a boot console of its own;
     * console and screen are closed again afterwards, so that the
     * Startup-Sequence starts with a fresh display. Without the script (or
     * the library) this is an ordinary single-user boot.
     */
    if (SECURITY_ACTIVE && !(BootFlags & (BF_NO_STARTUP_SEQUENCE | BF_EMERGENCY_CONSOLE | BF_NO_BOOT_REQUESTERS)))
    {
        BPTR sas = Open("S:Security-Startup", MODE_OLDFILE);

        if (sas)
        {
            struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)TaggedOpenLibrary(TAGGEDOPEN_INTUITION);
            struct Library *GfxBase = TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
            struct Screen *sysscr = NULL;
            struct Window *sysbdw = NULL;
            struct Hook sysbfhook;
            struct BootScrData sysbsd;
            BPTR scis, scos;

            /* everything below - console, login window, requesters - opens
             * on the black "SYSTEM" screen instead of forcing Workbench */
            if (IntuitionBase && GfxBase)
                sysscr = OpenBootScreen(IntuitionBase, GfxBase, &sysbdw, &sysbfhook, &sysbsd);

            scis = Open("CON:////AROS/AUTO/CLOSE/SMART/BOOT", MODE_OLDFILE);
            scos = scis ? OpenFromLock(DupLockFromFH(scis)) : BNULL;

            D(bug("[DOS] %s: running Security-Startup\n", __func__);)
            if (scis && scos)
            {
                if (SystemTags(NULL,
                               NP_Name, "Security Startup",
                               SYS_Background, FALSE,
                               SYS_Asynch, FALSE,
                               SYS_Input, scis,
                               SYS_Output, scos,
                               SYS_ScriptInput, sas,
                               TAG_END) == -1)
                {
                    D(bug("[DOS] %s:  .. Security-Startup failed!\n", __func__);)
                    Close(sas);
                }
                Close(scis);
                Close(scos);
            }
            else
            {
                if (scis)
                    Close(scis);
                Close(sas);
            }

            /* the script is done: take the display down again before the
             * Startup-Sequence runs. A console fallback without the SYSTEM
             * screen may have opened Workbench instead. */
            if (sysscr)
                CloseBootScreen(DOSBase, IntuitionBase, GfxBase, sysscr, sysbdw, &sysbsd);
            if (IntuitionBase)
            {
                CloseWorkBench();
                CloseLibrary((struct Library *)IntuitionBase);
            }
            if (GfxBase)
                CloseLibrary(GfxBase);
        }
    }

    D(bug("[DOS] %s: preparing console\n", __func__);)

    if (BootFlags & BF_EMERGENCY_CONSOLE) {
        D(bug("[DOS] %s:     (emergency console)\n", __func__);)
        BootFlags |= BF_NO_STARTUP_SEQUENCE;
        cis = Open("ECON:", MODE_OLDFILE);
    }

    if (cis == BNULL) {
        if (BootFlags & BF_NO_BOOT_REQUESTERS) {
            /* Appliance boot (CD): no boot console window either - the
             * CD32 Kickstart never opens one, and the console's window
             * is what would drag in a Workbench screen.
             */
            cis = Open("NIL:", MODE_OLDFILE);
        } else
            cis = Open("CON:////AROS/AUTO/CLOSE/SMART/BOOT", MODE_OLDFILE);
    }

    if (cis) {
        BPTR cos = OpenFromLock(DupLockFromFH(cis));
        BYTE *C = generate_banner();

        D(bug("[DOS] %s:  handle @ 0x%p (0x%p)\n", __func__, cis, cos);)

        if (cos) {
            BPTR cas = BNULL;

            if (!(BootFlags & BF_NO_STARTUP_SEQUENCE))
                cas = Open("S:Startup-Sequence", MODE_OLDFILE);

            /* Inject the banner */
            if (Flags & EBF_SILENTSTART) {
                if (SetVBuf(cos, NULL, BUF_FULL, sizeof(C)) == 0) {
                    FPuts(cos, C);
                    SetVBuf(cos, NULL, BUF_LINE, -1);
                }
            } else {
                FPuts(cos, C);
            }

            D(bug("[DOS] %s: initialising CLI\n", __func__);)

            if (SystemTags(NULL,
                           NP_Name, "Initial CLI",
                           NP_WindowPtr,
                               (BootFlags & BF_NO_BOOT_REQUESTERS)
                                   ? (IPTR)-1 : (IPTR)0,
                           SYS_Background, FALSE,
                           SYS_Asynch, FALSE,
                           SYS_Input, cis,
                           SYS_Output, cos,
                           SYS_ScriptInput, cas,
                           TAG_END) == -1) {
                D(bug("[DOS] %s:  .. failed!\n", __func__);)
                Alert(AT_DeadEnd | AN_BootStrap);
            }

            Close(cis);
#if (1)
            /* Do not flush cos (show banner) if we got this far, we don't want to
             * see shell window quickly opening and then immediately closing at
             * the end of startup-sequence.
             *
             * There has to be less hacky way..
             */
            struct FileHandle *fh = ((struct FileHandle*)BADDR(cos));
            fh->fh_Flags &= ~0x80000000;
#endif
            Close(cos);
            /* NOTE: 'cas' will already have been closed by the Shell */
        }
        FreeVec(C);
    } else {
        D(bug("[DOS] %s:  .. failed!\n", __func__);)
        Alert(AN_NoWindow);
    }
}
