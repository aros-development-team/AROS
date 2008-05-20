#define DEBUG 0
#include <aros/debug.h>

#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <devices/keyboard.h>
#include <devices/rawkeycodes.h>
#include <devices/timer.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <libraries/bootmenu.h>
#include <libraries/expansionbase.h>
#include <aros/bootloader.h>
#include <aros/symbolsets.h>
#include <string.h>
#include "devs_private.h"

#include "bootmenu_intern.h"

#include LC_LIBDEFS_FILE

#define BUFSIZE 100

#undef ExpansionBase
#define ExpansionBase BootMenuBase->bm_ExpansionBase

#if 0
struct GfxBase *GfxBase;
struct RastPort rp;
struct ViewPort vp;

static const ULONG coltab[] = {
    (16L << 16) + 0,    /* 16 colors, loaded at index 0 */

                                        /* X11 color names      */
    0xB3B3B3B3, 0xB3B3B3B3, 0xB3B3B3B3, /* Grey70       */
    0x00000000, 0x00000000, 0x00000000, /* Black        */
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, /* White        */
    0x66666666, 0x88888888, 0xBBBBBBBB, /* AMIGA Blue   */

    0x00000000, 0x00000000, 0xFFFFFFFF, /* Blue         */
    0x00000000, 0xFFFFFFFF, 0x00000000, /* Green        */
    0xFFFFFFFF, 0x00000000, 0x00000000, /* Red          */
    0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, /* Cyan         */

    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, /* Magenta      */
    0xEEEEEEEE, 0x82828282, 0xEEEEEEEE, /* Violet       */
    0xA5A5A5A5, 0x2A2A2A2A, 0x2A2A2A2A, /* Brown        */
    0xFFFFFFFF, 0xE4E4E4E4, 0xC4C4C4C4, /* Bisque       */

    0xE6E6E6E6, 0xE6E6E6E6, 0xFAFAFAFA, /* Lavender     */
    0x00000000, 0x00000000, 0x80808080, /* Navy         */
    0xF0F0F0F0, 0xE6E6E6E6, 0x8C8C8C8C, /* Khaki        */
    0xA0A0A0A0, 0x52525252, 0x2D2D2D2D, /* Sienna       */
    0L          /* Termination */
};

static ULONG pointercoltab[] = {
    0,
    0xE0E0E0E0, 0x40404040, 0x40404040,
    0x00000000, 0x00000000, 0x00000000,
    0xE0E0E0E0, 0xE0E0E0E0, 0xC0C0C0C0,
    0L
};

BOOL initScreen(STRPTR gfxhiddname, struct BootMenuBase *BootMenuBase)
{
    struct TagItem modetags[] =
    {
        {BIDTAG_Depth, 2},
        {BIDTAG_DesiredWidth, 640},
        {BIDTAG_DesiredHeight, 200},
        {TAG_DONE, 0UL}
    };
    ULONG modeid, depth;

    D(bug("[BootMenu] initScreen(gfxhidd='%s')\n", gfxhiddname));

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (GfxBase)
    {
        if (LateGfxInit(gfxhiddname))
        {
            modeid = BestModeIDA(modetags);
            if (modeid != INVALID_ID)
            {
                InitRastPort(&rp);
                InitVPort(&vp);
                rp.BitMap = AllocScreenBitMap(modeid);
                if (rp.BitMap)
                {
                    vp.RasInfo = AllocMem(sizeof(struct RasInfo), MEMF_ANY | MEMF_CLEAR);
                    if (vp.RasInfo)
                    {
                        vp.RasInfo->BitMap = rp.BitMap;
                        vp.ColorMap = GetColorMap(4);
                        vp.ColorMap->VPModeID = modeid;
                        if (AttachPalExtra(vp.ColorMap, &vp) == 0)
                        {
                            LoadRGB32(&vp, (ULONG *)coltab);
                            depth = GetBitMapAttr(rp.BitMap, BMA_DEPTH);
                            if (depth > 4)
                                pointercoltab[0] = (3L << 16) + 17;
                            else
                                pointercoltab[0] = (3L << 16) + (1 << depth) - 3;
                            LoadRGB32(&vp, pointercoltab);
                            rp.BitMap->Flags |= BMF_AROS_HIDD;
                            SetFont(&rp, GfxBase->DefaultFont);
                            SetFrontBitMap(rp.BitMap, TRUE);
                            Forbid();
                            rp.Font->tf_Accessors++;
                            Permit();
                            SetAPen(&rp, 1);
                            Move(&rp, 100, 100);
                            Text(&rp, "Sucker", 6);
kprintf("done\n");
                            return TRUE;
                        }
                        FreeMem(vp.RasInfo, sizeof(struct RasInfo));
                    }
                    FreeBitMap(rp.BitMap);
                }
            }
        }
        CloseLibrary((struct Library *)GfxBase);
    }
kprintf("no screen!\n");
    return FALSE;
}
#else

/*****************
**  init_gfx()  **
*****************/

static BOOL init_gfx(STRPTR gfxclassname, struct BootMenuBase *BootMenuBase)
{
    BOOL success = FALSE; 
    D(bug("[BootMenu] init_gfx(hiddbase='%s')\n", gfxclassname));
    
    /*  Call private gfx.library call to init the HIDD.
        Gfx library is responsable for closing the HIDD
        library (although it will probably not be neccesary).
    */

    D(bug("[BootMenu] init_gfx: calling private gfx LateGfxInit() .."));
    if (LateGfxInit(gfxclassname))
    {
        D(bug("Success\n"));
        if (IntuitionBase)
        {
            if (LateIntuiInit(NULL))
            {
                success = TRUE;
        }
        }
    }
    else
    {
        D(bug("Failed\n"));
    }
    ReturnBool ("init_gfxhidd", success);
}


static BOOL init_device( STRPTR hiddclassname, STRPTR devicename,  struct BootMenuBase *BootMenuBase)
{
    BOOL success = FALSE;
    struct MsgPort *mp = NULL;

    D(bug("[BootMenu] init_device(classname='%s', devicename='%s')\n", hiddclassname, devicename));

    if ((mp = CreateMsgPort()) != NULL)
    {
        struct IORequest *io = NULL;
        if ((io = CreateIORequest(mp, sizeof ( struct IOStdReq))) != NULL)
        {
            if (0 == OpenDevice(devicename, 0, io, 0))
            {
                #define ioStd(x) ((struct IOStdReq *)x)
                ioStd(io)->io_Command = CMD_HIDDINIT;
                ioStd(io)->io_Data = hiddclassname;
                ioStd(io)->io_Length = strlen(hiddclassname);

                /* Let the device init the HIDD */
                DoIO(io);
                if (0 == io->io_Error)
                {
                    success = TRUE;
                }
                CloseDevice(io);
            }
            DeleteIORequest(io); 
        }
        DeleteMsgPort(mp);
    } 
    ReturnBool("init_device", success);
}

static BOOL initHidds(struct BootConfig *bootcfg, struct BootMenuBase *BootMenuBase) 
{
    D(bug("[BootMenu] initHidds()\n"));

    OpenLibrary(bootcfg->defaultgfx.libname, 0);
    init_gfx(bootcfg->defaultgfx.hiddname, BootMenuBase);

    OpenLibrary(bootcfg->defaultmouse.libname, 0);
    init_device(bootcfg->defaultmouse.hiddname, "gameport.device", BootMenuBase);

    return TRUE;
}

static struct Gadget *createGadgets(struct BootMenuBase_intern *BootMenuBase) 
{
    struct Gadget *first;
    struct ButtonGadget *last;

    last = BootMenuBase->bm_MainGadgets.boot = createButton(16, 190, 280, 14, (struct Gadget *)&first, "Boot", BUTTON_BOOT, BootMenuBase);
    if (last == NULL)
        return NULL;
    last = BootMenuBase->bm_MainGadgets.bootnss = createButton(344, 190, 280, 14, last->gadget, "Boot With No Startup-Sequence", BUTTON_BOOT_WNSS, BootMenuBase);
    if (last == NULL)
        return NULL;
    last = BootMenuBase->bm_MainGadgets.bootopt = createButton(180, 63, 280, 14, last->gadget, "Boot Options...", BUTTON_BOOT_OPTIONS, BootMenuBase);
    if (last == NULL)
        return NULL;
    last = BootMenuBase->bm_MainGadgets.displayopt = createButton(180, 84, 280, 14, last->gadget, "Display Options...", BUTTON_DISPLAY_OPTIONS, BootMenuBase);
    if (last == NULL)
        return NULL;
    last = BootMenuBase->bm_MainGadgets.expboarddiag = createButton(180, 105, 280, 14, last->gadget, "Expansion Board Diagnostic...", BUTTON_EXPBOARDDIAG, BootMenuBase);
    if (last == NULL)
        return NULL;
    return first;
}

static void freeGadgets(struct BootMenuBase_intern *BootMenuBase)
{
    if (BootMenuBase->bm_MainGadgets.boot != NULL)
        freeButtonGadget(BootMenuBase->bm_MainGadgets.boot, BootMenuBase);
    if (BootMenuBase->bm_MainGadgets.bootnss != NULL);
        freeButtonGadget(BootMenuBase->bm_MainGadgets.bootnss, BootMenuBase);
    if (BootMenuBase->bm_MainGadgets.bootopt != NULL)
        freeButtonGadget(BootMenuBase->bm_MainGadgets.bootopt, BootMenuBase);
    if (BootMenuBase->bm_MainGadgets.displayopt != NULL)
        freeButtonGadget(BootMenuBase->bm_MainGadgets.displayopt, BootMenuBase);
    if (BootMenuBase->bm_MainGadgets.expboarddiag != NULL)
        freeButtonGadget(BootMenuBase->bm_MainGadgets.expboarddiag, BootMenuBase);
}

static void msgLoop(struct BootMenuBase *BootMenuBase, struct Window *win, struct BootConfig *bcfg)
{
    BOOL in=TRUE;
    struct IntuiMessage *msg;
    struct Gadget *g;

    do
    {
        WaitPort(win->UserPort);
        while ((msg=(struct IntuiMessage *)GetMsg(win->UserPort)))
        {
            if (msg->Class == IDCMP_GADGETUP)
            {
                g = msg->IAddress;
                switch (g->GadgetID)
                {
                case BUTTON_BOOT:
                    ExpansionBase->Flags &= ~EBF_DOSFLAG;
                    in = FALSE;
                    break;
                case BUTTON_BOOT_WNSS:
                    ExpansionBase->Flags |= EBF_DOSFLAG;
                    in = FALSE;
                    break;
                }
            }
            ReplyMsg(&msg->ExecMessage);
        }
    } while (in);
    while ((msg=(struct IntuiMessage *)GetMsg(win->UserPort)))
        ReplyMsg(&msg->ExecMessage);
}

static BOOL initScreen(struct BootMenuBase_intern *BootMenuBase, struct BootConfig *bcfg)
{
    UWORD pens[] = {~0};
    struct TagItem scrtags[] =
    {
        {SA_Width,       640},
        {SA_Height,      256},
        {SA_Depth,         4},
        {SA_Pens, (IPTR)pens},
        {TAG_DONE,       0UL}
    };
    struct TagItem wintags[] =
    {
        {WA_Left,          0}, /* 0 */
        {WA_Top,           0}, /* 1 */
        {WA_Width,       640}, /* 2 */
        {WA_Height,      256}, /* 3 */
        {WA_CustomScreen,  0}, /* 4 */
        {WA_Gadgets,    NULL}, /* 5 */
        {WA_IDCMP, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_VANILLAKEY | IDCMP_GADGETUP | IDCMP_GADGETDOWN},
        {WA_Borderless, TRUE},
        {WA_RMBTrap,    TRUE},
        {TAG_DONE,       0UL}
    };
    struct Gadget *first = NULL;

    BootMenuBase->bm_Screen = OpenScreenTagList(NULL, scrtags);
    if (BootMenuBase->bm_Screen != NULL)
    {
        first = createGadgets(BootMenuBase);
        if (first != NULL)
        {
            wintags[2].ti_Data = BootMenuBase->bm_Screen->Width;
            wintags[3].ti_Data = BootMenuBase->bm_Screen->Height;
            wintags[4].ti_Data = (IPTR)BootMenuBase->bm_Screen;
            wintags[5].ti_Data = (IPTR)first;
            BootMenuBase->bm_Window = OpenWindowTagList(NULL, wintags);
            if (BootMenuBase->bm_Window != NULL)
            {
                SetAPen(BootMenuBase->bm_Window->RPort, 2);
                Move(BootMenuBase->bm_Window->RPort, 215, 20);
                Text(BootMenuBase->bm_Window->RPort, "AROS Early Startup Control", 26);
                SetAPen(BootMenuBase->bm_Window->RPort, 1);
                Move(BootMenuBase->bm_Window->RPort, 225, 40);
                Text(BootMenuBase->bm_Window->RPort, "(what is PAL and NTSC?)", 23);
                msgLoop(BootMenuBase, BootMenuBase->bm_Window, bcfg);
                return TRUE;
            }
            else
                Alert(AT_DeadEnd | AN_OpenWindow);
            CloseWindow(BootMenuBase->bm_Window);
            freeGadgets(BootMenuBase);
        }
        else
            Alert(AT_DeadEnd | AN_BadGadget);
        CloseScreen(BootMenuBase->bm_Screen);
    }
    else
        Alert(AT_DeadEnd | AN_OpenScreen);
    return FALSE;
}

static BOOL buttonsPressed(struct BootMenuBase *BootMenuBase, struct DefaultHidd *kbd) 
{
    BOOL success = FALSE;
    struct MsgPort *mp = NULL;
    UBYTE matrix[16];

    if ((mp = CreateMsgPort()) != NULL)
    {
        struct IORequest *io = NULL;
        if ((io = CreateIORequest(mp, sizeof ( struct IOStdReq))) != NULL)
        {
            if (0 == OpenDevice("keyboard.device", 0, io, 0))
            {
                D(bug("[BootMenu] buttonsPressed: Checking KBD_READMATRIX\n"));
                #define ioStd(x) ((struct IOStdReq *)x)
                ioStd(io)->io_Command = KBD_READMATRIX;
                ioStd(io)->io_Data = matrix;
                ioStd(io)->io_Length = 16;
                DoIO(io);
                if (0 == io->io_Error)
                {
#if defined(DEBUG)
                    int i;
                    D(bug("[BootMenu] buttonsPressed: Matrix : "));
                    for (i = 0; i < sizeof(matrix); i ++)
                    {
                        D(bug("%2x ", matrix[i]));
                    }
                    D(bug("\n"));
#endif
                    if (matrix[RAWKEY_SPACE/8] & (1<<(RAWKEY_SPACE%8)))
                        success = TRUE;
                }
                CloseDevice(io);
            }
            DeleteIORequest(io); 
        }
        DeleteMsgPort(mp);
    }
    return success;
}

#endif

static struct BootConfig bootcfg =
{
    &bootcfg,
    NULL,
    {"vgah.hidd", "hidd.gfx.vga"},
    {"kbd.hidd", "hidd.kbd.hw"},
    {"mouse.hidd", "hidd.bus.mouse"},
};

static int bootmenu_EarlyPrep(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[BootMenu] bootmenu_EarlyPrep()\n"));

    struct BootLoaderBase *BootLoaderBase = NULL;
    struct VesaInfo *vi = NULL;

    LIBBASE->bm_BootConfig = bootcfg;
    struct DefaultHidd *kbd = &LIBBASE->bm_BootConfig.defaultkbd;

    if ((ExpansionBase = OpenLibrary("expansion.library",0)) != NULL)
    {
        if ((BootLoaderBase = OpenResource("bootloader.resource")) != NULL)
        {
            if ((vi = (struct VesaInfo *)GetBootInfo(BL_Video)) != NULL)
            {
                if (vi->ModeNumber != 3)
                {
                    strcpy(LIBBASE->bm_BootConfig.defaultgfx.libname, "vesagfx.hidd");
                    strcpy(LIBBASE->bm_BootConfig.defaultgfx.hiddname, "hidd.gfx.vesa");
                }
            }
        }
        
        if (OpenLibrary(kbd->libname, 0) != NULL)
        {
            if (init_device(kbd->hiddname, "keyboard.device", LIBBASE))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*****************************************************************************

    NAME */
    AROS_LH0(void, bootmenu_CheckAndDisplay,

/*  SYNOPSIS */

/*  LOCATION */
    LIBBASETYPEPTR, LIBBASE, 1, Bootmenu)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[BootMenu] bootmenu_CheckAndDisplay()\n"));

    /* check keyboard */
    if (buttonsPressed(LIBBASE, &LIBBASE->bm_BootConfig.defaultkbd))
    {
        kprintf("Entering Boot Menu ...\n");
        /* init mouse + gfx */
        if (initHidds(&LIBBASE->bm_BootConfig, LIBBASE))
        {
            initScreen(LIBBASE, &LIBBASE->bm_BootConfig);
        }
    }

    AROS_LIBFUNC_EXIT
} /* bootmenu_CheckAndDisplay */

ADD2INITLIB(bootmenu_EarlyPrep, 0)
