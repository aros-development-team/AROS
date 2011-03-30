/*
   Copyright © 1995-2010, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Main bootmenu code
   Lang: english
*/

#include <aros/debug.h>

#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/oop.h>
#include <devices/keyboard.h>
#include <devices/rawkeycodes.h>
#include <devices/timer.h>
#include <exec/memory.h>
#include <graphics/driver.h>
#include <libraries/expansionbase.h>
#include <aros/bootloader.h>
#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

#include "dosboot_intern.h"
#include "menu.h"

static BOOL init_gfx(STRPTR gfxclassname, BOOL bootmode, LIBBASETYPEPTR DOSBootBase)
{
    OOP_Object *gfxhidd;
    BOOL success = FALSE;

    D(bug("[BootMenu] init_gfx('%s')\n", gfxclassname));
    
    GfxBase = (void *)OpenLibrary("graphics.library", 41);
    if (!GfxBase)
        return FALSE;

    gfxhidd = OOP_NewObject(NULL, gfxclassname, NULL);
    if (gfxhidd) {
        if (AddDisplayDriver(gfxhidd, DDRV_BootMode, bootmode, TAG_DONE))
	    OOP_DisposeObject(gfxhidd);
	else
	    success = TRUE;
    }

    CloseLibrary(&GfxBase->LibNode);

    ReturnBool ("init_gfxhidd", success);
}

static BOOL initHidds(LIBBASETYPEPTR DOSBootBase)
{
    struct BootConfig *bootcfg = &DOSBootBase->bm_BootConfig;

    D(bug("[BootMenu] initHidds()\n"));

    if (bootcfg->gfxhidd) {
	if (!OpenLibrary(bootcfg->gfxlib, 0))
	    return FALSE;

        if (!init_gfx(bootcfg->gfxhidd, bootcfg->bootmode, DOSBootBase))
	    return FALSE;
    }

    D(bug("[BootMenu] initHidds: Hidds initialised\n"));
    return TRUE;
}

static struct Gadget *createGadgets(LIBBASETYPEPTR DOSBootBase) 
{
    /* FIXME: This is very unclean! free-up resources if we fail! */

    /* Create Option Gadgets */
    DOSBootBase->bm_MainGadgets.bootopt = createButton(
                                                        180, 63, 280, 14,
                                                        NULL, "Boot Options...",
                                                        BUTTON_BOOT_OPTIONS, (struct DOSBootBase *)DOSBootBase);
    if (DOSBootBase->bm_MainGadgets.bootopt == NULL)
            return NULL;

    DOSBootBase->bm_MainGadgets.displayopt = createButton(
                                                        180, 84, 280, 14,
                                                        DOSBootBase->bm_MainGadgets.bootopt->gadget, "Display Options...",
                                                        BUTTON_DISPLAY_OPTIONS, (struct DOSBootBase *)DOSBootBase);
    if (DOSBootBase->bm_MainGadgets.displayopt == NULL)
            return NULL;

    DOSBootBase->bm_MainGadgets.expboarddiag = createButton(
                                                        180, 105, 280, 14, 
                                                        DOSBootBase->bm_MainGadgets.displayopt->gadget, "Expansion Board Diagnostic...",
                                                        BUTTON_EXPBOARDDIAG, (struct DOSBootBase *)DOSBootBase);
    if (DOSBootBase->bm_MainGadgets.expboarddiag == NULL)
            return NULL;

    /* Create BOOT Gadgets */
    DOSBootBase->bm_MainGadgets.boot = createButton(
                                                        16, 190, 280, 14,
                                                        DOSBootBase->bm_MainGadgets.expboarddiag->gadget, "Boot",
                                                        BUTTON_BOOT, (struct DOSBootBase *)DOSBootBase);
    if (DOSBootBase->bm_MainGadgets.boot == NULL)
            return NULL;

    DOSBootBase->bm_MainGadgets.bootnss = createButton(
                                                        344, 190, 280, 14,
                                                        DOSBootBase->bm_MainGadgets.boot->gadget, "Boot With No Startup-Sequence",
                                                        BUTTON_BOOT_WNSS, (struct DOSBootBase *)DOSBootBase);
    if (DOSBootBase->bm_MainGadgets.bootnss == NULL)
            return NULL;

    return DOSBootBase->bm_MainGadgets.bootopt->gadget;
}

static void freeGadgets(LIBBASETYPEPTR DOSBootBase)
{
    if (DOSBootBase->bm_MainGadgets.boot != NULL)
        freeButtonGadget(DOSBootBase->bm_MainGadgets.boot, (struct DOSBootBase *)DOSBootBase);
    if (DOSBootBase->bm_MainGadgets.bootnss != NULL);
        freeButtonGadget(DOSBootBase->bm_MainGadgets.bootnss, (struct DOSBootBase *)DOSBootBase);
    if (DOSBootBase->bm_MainGadgets.bootopt != NULL)
        freeButtonGadget(DOSBootBase->bm_MainGadgets.bootopt, (struct DOSBootBase *)DOSBootBase);
    if (DOSBootBase->bm_MainGadgets.displayopt != NULL)
        freeButtonGadget(DOSBootBase->bm_MainGadgets.displayopt, (struct DOSBootBase *)DOSBootBase);
    if (DOSBootBase->bm_MainGadgets.expboarddiag != NULL)
        freeButtonGadget(DOSBootBase->bm_MainGadgets.expboarddiag, (struct DOSBootBase *)DOSBootBase);
}

static void msgLoop(LIBBASETYPEPTR DOSBootBase, struct Window *win)
{
    BOOL exit = FALSE;
    struct IntuiMessage *msg;
    struct Gadget *g;

    D(bug("[BootMenu] msgLoop(DOSBootBase @ %p, Window @ %p)\n", DOSBootBase, win));

    do
    {
        if (win->UserPort)
        {
            WaitPort(win->UserPort);
            while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
            {
                if (msg->Class == IDCMP_GADGETUP)
                {
                    g = msg->IAddress;
                    switch (g->GadgetID)
                    {
                    case BUTTON_BOOT:
                            DOSBootBase->BootFlags &= ~BF_NO_STARTUP_SEQUENCE;
                            exit = TRUE;
                            break;
                    case BUTTON_BOOT_WNSS:
                            DOSBootBase->BootFlags |= BF_NO_STARTUP_SEQUENCE;
                            exit = TRUE;
                            break;
                    }
                }
                ReplyMsg((struct Message *)msg);
            }
        }
        else
        {
            bug("[BootMenu] msgLoop: Window lacks a userport!\n");
            Wait(0);
        }
    } while (exit == FALSE);
    while ((msg=(struct IntuiMessage *)GetMsg(win->UserPort)))
        ReplyMsg(&msg->ExecMessage);
}

static BOOL initScreen(LIBBASETYPEPTR DOSBootBase, struct BootConfig *bcfg)
{
    struct Screen *bm_Screen;
    struct Gadget *first = NULL;
    BOOL res = FALSE;

    D(bug("[BootMenu] initScreen()\n"));

    bm_Screen = OpenBootScreen(DOSBootBase);
    D(bug("[BootMenu] initScreen: Screen opened @ %p\n", bm_Screen));

    if ((first = createGadgets(DOSBootBase)) != NULL)
    {
        struct NewWindow nw =
        {
            0, 0,                            /* Left, Top */
            bm_Screen->Width,		     /* Width, Height */
            bm_Screen->Height,
            0, 1,                            /* DetailPen, BlockPen */
            IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_VANILLAKEY | IDCMP_GADGETUP | IDCMP_GADGETDOWN, /* IDCMPFlags */
            WFLG_SMART_REFRESH | WFLG_BORDERLESS | WFLG_ACTIVATE, /* Flags */
            first,       	             /* FirstGadget */
            NULL,       	             /* CheckMark */
            NULL,       	             /* Title */
            bm_Screen,			     /* Screen */
            NULL,                            /* BitMap */
            0, 0,                   	     /* MinWidth, MinHeight */
            0, 0,                            /* MaxWidth, MaxHeight */
            CUSTOMSCREEN,                    /* Type */
        };

        D(bug("[BootMenu] initScreen: Gadgets created @ %p\n", first));

        if ((DOSBootBase->bm_Window = OpenWindow(&nw)) != NULL)
        {
            D(bug("[BootMenu] initScreen: Window opened @ %p\n", DOSBootBase->bm_Window));
            D(bug("[BootMenu] initScreen: Window RastPort @ %p\n", DOSBootBase->bm_Window->RPort));
            D(bug("[BootMenu] initScreen: Window UserPort @ %p\n", DOSBootBase->bm_Window->UserPort));
            SetAPen(DOSBootBase->bm_Window->RPort, 2);
            D(bug("[BootMenu] initScreen: SetAPen 2\n"));
            Move(DOSBootBase->bm_Window->RPort, 215, 20);
            D(bug("[BootMenu] initScreen: Move(d) to 215, 20\n"));
            Text(DOSBootBase->bm_Window->RPort, "AROS Early Startup Control", 26);
            D(bug("[BootMenu] initScreen: Early Startup text displayed\n"));
#if  defined(USE_PALNTSC)
#warning "TODO: Check if we are using a PAL/NTSC display mode ..."
            SetAPen(DOSBootBase->bm_Window->RPort, 1);
            Move(DOSBootBase->bm_Window->RPort, 225, 40);
            Text(DOSBootBase->bm_Window->RPort, "(press a key to toggle the display between PAL and NTSC)", 23);
#endif
            msgLoop(DOSBootBase, DOSBootBase->bm_Window);
            res = TRUE;
        }
        CloseWindow(DOSBootBase->bm_Window);
        freeGadgets(DOSBootBase);
    }
    
    CloseBootScreen(bm_Screen, DOSBootBase);

    return res;
}

/* From keyboard.device/keyboard_intern.h */
#define KB_MAXKEYS     256
#define KB_MATRIXSIZE  (KB_MAXKEYS/(sizeof(UBYTE)*8))
#define ioStd(x) ((struct IOStdReq *)x)

static BOOL buttonsPressed(LIBBASETYPEPTR DOSBootBase) 
{
    BOOL success = FALSE;
    struct MsgPort *mp = NULL;
    UBYTE matrix[KB_MATRIXSIZE];

    if ((mp = CreateMsgPort()) != NULL)
    {
        struct IORequest *io = NULL;
        if ((io = CreateIORequest(mp, sizeof ( struct IOStdReq))) != NULL)
        {
            if (0 == OpenDevice("keyboard.device", 0, io, 0))
            {
                D(bug("[BootMenu] buttonsPressed: Checking KBD_READMATRIX\n"));
                ioStd(io)->io_Command = KBD_READMATRIX;
                ioStd(io)->io_Data = matrix;
                ioStd(io)->io_Length = sizeof(matrix);
                DoIO(io);
                if (0 == io->io_Error)
                {
                    D(
                        int i;
                        bug("[BootMenu] buttonsPressed: Matrix : ");
                        for (i = 0; i < ioStd(io)->io_Actual; i ++)
                        {
                                bug("%02x ", matrix[i]);
                        }
                        bug("\n");
                    );
                    if (matrix[RAWKEY_SPACE/8] & (1<<(RAWKEY_SPACE%8)))
                    {
                            D(bug("[BootMenu] SPACEBAR pressed\n"));
                            success = TRUE;
                    }
                }
                CloseDevice(io);
            }
            DeleteIORequest(io); 
        }
        DeleteMsgPort(mp);
    }
    return success;
}

int bootmenu_Init(LIBBASETYPEPTR LIBBASE)
{
    struct BootLoaderBase *BootLoaderBase = NULL;
    BOOL bmi_RetVal = FALSE;
    BOOL WantBootMenu = FALSE;

    D(bug("[BootMenu] bootmenu_Init()\n"));

    BootLoaderBase = OpenResource("bootloader.resource");
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
   /* Hosted ports have their HIDDs rewritten. Native still don't,
      and native drivers still need external initialization. This is
      going to change. */
    InitBootConfig(&LIBBASE->bm_BootConfig, BootLoaderBase);
#endif

    /* Check for command line argument */
    if (BootLoaderBase)
    {
        struct List *list = NULL;
        struct Node *node = NULL;

        if ((list = (struct List *)GetBootInfo(BL_Args)) != NULL)
	{
            ForeachNode(list,node)
	    {
                if (0 == strcmp(node->ln_Name, "bootmenu")) {
                    D(bug("[BootMenu] bootmenu_Init: Forced with bootloader argument\n"));
                    WantBootMenu = TRUE;
                }

		if (0 == strcmp(node->ln_Name, "nomonitors"))
		    LIBBASE->BootFlags |= BF_NO_DISPLAY_DRIVERS;
            }
        }
    }

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT) && defined(mc68000)
    if (1) {
    	volatile UBYTE *cia = (UBYTE*)0xbfe001;
    	volatile UWORD *potinp = (UWORD*)0xdff016;

    	/* check left + right mouse button state */
    	if ((cia[0] & 0x40) == 0 && (potinp[0] & 0x0400) == 0)
    	    WantBootMenu = TRUE;

    	/* native hardware have resident ROM drivers,
    	 * so the following initHidds won't be executed on m68k
    	 */
    } else 
#endif
    /* Initialize default HIDDs */
    if (!initHidds(LIBBASE))
	return FALSE;

    /* check keyboard if needed */
    if (!WantBootMenu)
        WantBootMenu = buttonsPressed(LIBBASE);

    /* Bring up early startup menu if requested */
    if (WantBootMenu)
    {
        bmi_RetVal = FALSE;

        D(kprintf("[BootMenu] bootmenu_Init: Entering Boot Menu ...\n"));
	bmi_RetVal = initScreen(LIBBASE, &LIBBASE->bm_BootConfig);
    }

    return bmi_RetVal;
}
