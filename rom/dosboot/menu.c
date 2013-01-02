/*
   Copyright © 1995-2012, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Main bootmenu code
   Lang: english
*/

#include <aros/config.h>
#include <aros/debug.h>

#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/expansion.h>
#include <proto/oop.h>
#include <devices/keyboard.h>
#include <devices/rawkeycodes.h>
#include <devices/timer.h>
#include <exec/memory.h>
#include <graphics/driver.h>
#include <libraries/expansionbase.h>
#include <libraries/configvars.h>
#include <dos/filehandler.h>
#include <devices/trackdisk.h>
#include <exec/rawfmt.h>
#include <aros/bootloader.h>
#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

#include "dosboot_intern.h"
#include "menu.h"

#define PAGE_MAIN 1
#define PAGE_BOOT 2
#define PAGE_DISPLAY 3
#define PAGE_EXPANSION 4
#define EXIT_BOOT 5
#define EXIT_BOOT_WNSS 6

#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)
#ifdef __ppc__
#define INITHIDDS_KLUDGE
#endif
#endif

#ifdef INITHIDDS_KLUDGE

/*
 * This is an extremely obsolete kludge.
 * It's still needed for ATI driver on PowerPC native.
 */

static BOOL init_gfx(STRPTR gfxclassname, BOOL bootmode, LIBBASETYPEPTR DOSBootBase)
{
    OOP_Class *gfxclass;
    BOOL success = FALSE;

    D(bug("[BootMenu] init_gfx('%s')\n", gfxclassname));
    
    GfxBase = (void *)OpenLibrary("graphics.library", 41);
    if (!GfxBase)
        return FALSE;

    gfxclass = OOP_FindClass(gfxclassname);
    if (gfxclass)
    {
        if (!AddDisplayDriver(gfxclass, NULL, DDRV_BootMode, bootmode, TAG_DONE))
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

#endif

static struct Gadget *createGadgetsBoot(LIBBASETYPEPTR DOSBootBase) 
{
    /* Create Option Gadgets */
    DOSBootBase->bm_MainGadgets.bootopt = createButton(
                                                        180, 63, 280, 14,
                                                        NULL, "Boot Options...",
                                                        BUTTON_BOOT_OPTIONS, (struct DOSBootBase *)DOSBootBase);
    DOSBootBase->bm_MainGadgets.displayopt = createButton(
                                                        180, 84, 280, 14,
                                                        DOSBootBase->bm_MainGadgets.bootopt->gadget, "Display Options...",
                                                        BUTTON_DISPLAY_OPTIONS, (struct DOSBootBase *)DOSBootBase);
    DOSBootBase->bm_MainGadgets.expboarddiag = createButton(
                                                        180, 105, 280, 14, 
                                                        DOSBootBase->bm_MainGadgets.displayopt->gadget, "Expansion Board Diagnostic...",
                                                        BUTTON_EXPBOARDDIAG, (struct DOSBootBase *)DOSBootBase);
    /* Create BOOT Gadgets */
    DOSBootBase->bm_MainGadgets.boot = createButton(
                                                    16, DOSBootBase->bottomY, 280, 14,
                                                    DOSBootBase->bm_MainGadgets.expboarddiag->gadget, "Boot",
                                                    BUTTON_BOOT, (struct DOSBootBase *)DOSBootBase);
    DOSBootBase->bm_MainGadgets.bootnss = createButton(
                                                    344, DOSBootBase->bottomY, 280, 14,
                                                    DOSBootBase->bm_MainGadgets.boot->gadget, "Boot With No Startup-Sequence",
                                                    BUTTON_BOOT_WNSS, (struct DOSBootBase *)DOSBootBase);
    if (!DOSBootBase->bm_MainGadgets.bootopt ||
        !DOSBootBase->bm_MainGadgets.displayopt ||
        !DOSBootBase->bm_MainGadgets.expboarddiag ||
        !DOSBootBase->bm_MainGadgets.boot ||
        !DOSBootBase->bm_MainGadgets.bootnss)
        return NULL;
    return DOSBootBase->bm_MainGadgets.bootopt->gadget;
}

static struct Gadget *createGadgetsUseCancel(LIBBASETYPEPTR DOSBootBase)
{
    DOSBootBase->bm_MainGadgets.use = createButton(
                                                16, DOSBootBase->bottomY, 280, 14,
                                                NULL, "Use",
                                                BUTTON_USE, (struct DOSBootBase *)DOSBootBase);
    DOSBootBase->bm_MainGadgets.cancel = createButton(
                                                344, DOSBootBase->bottomY, 280, 14,
                                                DOSBootBase->bm_MainGadgets.use->gadget, "Cancel",
                                                BUTTON_CANCEL, (struct DOSBootBase *)DOSBootBase);
    if (!DOSBootBase->bm_MainGadgets.use ||
        !DOSBootBase->bm_MainGadgets.cancel)
        return NULL;
    return DOSBootBase->bm_MainGadgets.use->gadget;
}


static void freeGadgetsBoot(LIBBASETYPEPTR DOSBootBase)
{
    freeButtonGadget(DOSBootBase->bm_MainGadgets.boot, (struct DOSBootBase *)DOSBootBase);
    freeButtonGadget(DOSBootBase->bm_MainGadgets.bootnss, (struct DOSBootBase *)DOSBootBase);
    freeButtonGadget(DOSBootBase->bm_MainGadgets.bootopt, (struct DOSBootBase *)DOSBootBase);
    freeButtonGadget(DOSBootBase->bm_MainGadgets.displayopt, (struct DOSBootBase *)DOSBootBase);
    freeButtonGadget(DOSBootBase->bm_MainGadgets.expboarddiag, (struct DOSBootBase *)DOSBootBase);
}

static void freeGadgetsUseCancel(LIBBASETYPEPTR DOSBootBase)
{
    freeButtonGadget(DOSBootBase->bm_MainGadgets.use, (struct DOSBootBase *)DOSBootBase);
    freeButtonGadget(DOSBootBase->bm_MainGadgets.cancel, (struct DOSBootBase *)DOSBootBase);
}

static struct Gadget *createGadgets(LIBBASETYPEPTR DOSBootBase, WORD page) 
{
    if (page == PAGE_MAIN)
        return createGadgetsBoot(DOSBootBase);
    else
        return createGadgetsUseCancel(DOSBootBase);
}
static void freeGadgets(LIBBASETYPEPTR DOSBootBase, WORD page)
{
    if (page == PAGE_MAIN)
        freeGadgetsBoot(DOSBootBase);
    else
        freeGadgetsUseCancel(DOSBootBase);
}

static void toggleMode(LIBBASETYPEPTR DOSBootBase)
{
#ifdef mc68000
    /*
     * On m68k we may have ciaa.resource (if running on classic Amiga HW)
     */
    if (OpenResource("ciaa.resource")) {
        volatile UWORD *beamcon0 = (UWORD*)0xdff1dc;
        GfxBase->DisplayFlags ^= PAL | NTSC;
        *beamcon0 = (GfxBase->DisplayFlags & PAL) ? 0x0020 : 0x0000;
    }
#endif
}

static UWORD msgLoop(LIBBASETYPEPTR DOSBootBase, struct Window *win)
{
    WORD exit = -1;
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
                if (msg->Class == IDCMP_VANILLAKEY) {
                    if (msg->Code == 27)
                            exit = PAGE_MAIN;
                    else if (msg->Code >= '1' && msg->Code <= '3')
                            exit = PAGE_MAIN + msg->Code - '0';
                    else if (msg->Code >= 'a' && msg->Code <='j') {
                        BYTE pos = msg->Code - 'a', i = 0;
                        struct BootNode *bn;
                        DOSBootBase->db_BootNode = NULL;

                        Forbid(); /* .. access to ExpansionBase->MountList */
                        ForeachNode(&DOSBootBase->bm_ExpansionBase->MountList, bn)
                        {
                            if (i++ == pos)
                            {
                                DOSBootBase->db_BootNode = bn;
                                break;
                            }
                        }
                        Permit();

                        if (DOSBootBase->db_BootNode != NULL)
                        {
                            /* Refresh itself */
                            exit = PAGE_BOOT;
                            break;
                        }
                    }
                    else
                        toggleMode(DOSBootBase);
                } else if (msg->Class == IDCMP_GADGETUP)
                {
                    g = msg->IAddress;
                    switch (g->GadgetID)
                    {
                    case BUTTON_BOOT:
                        DOSBootBase->db_BootFlags &= ~BF_NO_STARTUP_SEQUENCE;
                        exit = EXIT_BOOT;
                        break;
                    case BUTTON_BOOT_WNSS:
                        DOSBootBase->db_BootFlags |= BF_NO_STARTUP_SEQUENCE;
                        exit = EXIT_BOOT_WNSS;
                        break;
                    case BUTTON_CANCEL:
                        DOSBootBase->db_BootNode = NULL;
                        /* Fallthrough */
                    case BUTTON_USE:
                    case BUTTON_CONTINUE:
                        exit = PAGE_MAIN;
                        break;
                    case BUTTON_BOOT_OPTIONS:
						exit = PAGE_BOOT;
						break;
                    case BUTTON_EXPBOARDDIAG:
						exit = PAGE_EXPANSION;
						break;
                    case BUTTON_DISPLAY_OPTIONS:
						exit = PAGE_DISPLAY;
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
    } while (exit < 0);

    while ((msg=(struct IntuiMessage *)GetMsg(win->UserPort)))
        ReplyMsg(&msg->ExecMessage);

    return exit;
}

static void initPageExpansion(LIBBASETYPEPTR DOSBootBase)
{
    struct Window *win = DOSBootBase->bm_Window;
    struct ExpansionBase *ExpansionBase = DOSBootBase->bm_ExpansionBase;
    struct ConfigDev *cd;
    WORD y = 50, cnt;
    char text[100];

    SetAPen(win->RPort, 1);
    cd = NULL;
    cnt = 0;
    while ((cd = FindConfigDev(cd, -1, -1))) {
        NewRawDoFmt("%2d: %08lx - %08lx (%08lx) %5d %3d %08lx", RAWFMTFUNC_STRING, text,
                ++cnt,
                cd->cd_BoardAddr, cd->cd_BoardAddr + cd->cd_BoardSize - 1, cd->cd_BoardSize,
                cd->cd_Rom.er_Manufacturer, cd->cd_Rom.er_Product, cd->cd_Rom.er_SerialNumber);
        if ((cd->cd_Rom.er_Type & ERT_TYPEMASK) == ERT_ZORROIII)
            strcat(text, " Z3");
        else if ((cd->cd_Rom.er_Type & ERT_TYPEMASK) == ERT_ZORROII)
            strcat(text, " Z2");
        else
            strcat(text, "   ");
        if (cd->cd_Rom.er_Type & ERTF_DIAGVALID)
            strcat(text, " ROM");
        if (cd->cd_Rom.er_Type & ERTF_MEMLIST)
            strcat(text, " RAM");
         Move(win->RPort, 20, y);
        Text(win->RPort, text, strlen(text));
        y += 16;
   }
}

static BOOL bstreqcstr(BSTR bstr, CONST_STRPTR cstr)
{
    int clen;
    int blen;

    clen = strlen(cstr);
    blen = AROS_BSTR_strlen(bstr);
    if (clen != blen)
        return FALSE;

    return (memcmp(AROS_BSTR_ADDR(bstr),cstr,clen) == 0);
}

void selectBootDevice(LIBBASETYPEPTR DOSBootBase)
{
    struct BootNode *bn = NULL;

    if (DOSBootBase->db_BootDevice == NULL &&
        DOSBootBase->db_BootNode != NULL)
        return;

    Forbid(); /* .. access to ExpansionBase->MountList */

    if (DOSBootBase->db_BootNode == NULL && DOSBootBase->db_BootDevice == NULL)
    {
        bn = (APTR)GetHead(&DOSBootBase->bm_ExpansionBase->MountList);
    }
    else
    {
        struct BootNode *i;
        ForeachNode(&DOSBootBase->bm_ExpansionBase->MountList, i)
        {
            struct DeviceNode *dn;

            dn = i->bn_DeviceNode;
            if (dn == NULL || dn->dn_Name == BNULL)
                continue;

            if (bstreqcstr(dn->dn_Name, DOSBootBase->db_BootDevice))
            {
                bn = i;
                break;
            }
        }
    }

    Permit();

    DOSBootBase->db_BootNode = bn;
    DOSBootBase->db_BootDevice = NULL;
}

/* This makes the selected boot device the actual
 * boot device. It also updates the boot flags.
 */
static void setBootDevice(LIBBASETYPEPTR DOSBootBase)
{
    struct BootNode *bn;

    bn = DOSBootBase->db_BootNode;

    if (bn != NULL)
    {
        Remove((struct Node *)bn);
        bn->bn_Node.ln_Type = NT_BOOTNODE;
        bn->bn_Node.ln_Pri = 127;
        /* We use AddHead() instead of Enqueue() here
         * to *insure* that this gets to the front of
         * the boot list.
         */
        AddHead(&DOSBootBase->bm_ExpansionBase->MountList, (struct Node *)bn);
    }

    DOSBootBase->bm_ExpansionBase->eb_BootFlags = DOSBootBase->db_BootFlags;
}

static void initPageBoot(LIBBASETYPEPTR DOSBootBase)
{
    struct Window *win = DOSBootBase->bm_Window;
    struct BootNode *bn;
    WORD y = 50;
    char text[100], *textp;

    SetAPen(win->RPort, 1);

    ForeachNode(&DOSBootBase->bm_ExpansionBase->MountList, bn)
    {
        struct DeviceNode *dn = bn->bn_DeviceNode;
        struct FileSysStartupMsg *fssm = BADDR(dn->dn_Startup);
        struct DosEnvec *de = NULL;
        struct IOStdReq *io;
        struct MsgPort *port;
        char dostype[5];
        UBYTE i;
        ULONG size;
        BOOL devopen, ismedia;

        if (y >= DOSBootBase->bottomY - 20)
            break;
        if (!fssm || !fssm->fssm_Device)
            continue;
        if (fssm->fssm_Environ > (BPTR)0x64) {
            de = BADDR(fssm->fssm_Environ);
            if (de->de_TableSize < 15)
                de = NULL;
        }

        NewRawDoFmt("%c%10s: %4d %s-%ld", RAWFMTFUNC_STRING, text,
            (DOSBootBase->db_BootNode == bn) ? '*' : IsBootableNode(bn) ? '+' : ' ',
            AROS_BSTR_ADDR(dn->dn_Name),
            bn->bn_Node.ln_Pri,
            AROS_BSTR_ADDR(fssm->fssm_Device),
            fssm->fssm_Unit);
        Move(win->RPort, 20, y);
        Text(win->RPort, text, strlen(text));

        textp = NULL;
        devopen = ismedia = FALSE;
        if ((port = (struct MsgPort*)CreateMsgPort())) {
            if ((io = (struct IOStdReq*)CreateIORequest(port, sizeof(struct IOStdReq)))) {
                if (!OpenDevice(AROS_BSTR_ADDR(fssm->fssm_Device), fssm->fssm_Unit, (struct IORequest*)io, fssm->fssm_Flags)) {
                    devopen = TRUE;
                    io->io_Command = TD_CHANGESTATE;
                    io->io_Actual = 1;
                    DoIO((struct IORequest*)io);
                    if (!io->io_Error && io->io_Actual == 0)
                        ismedia = TRUE;
                    CloseDevice((struct IORequest*)io);
                }
                DeleteIORequest((struct IORequest*)io);
            }
            DeleteMsgPort(port);
        }

        if (de && ismedia) {
            for (i = 0; i < 4; i++) {
                dostype[i] = (de->de_DosType >> ((3 - i) * 8)) & 0xff;
                if (dostype[i] < 9)
                    dostype[i] += '0';
                else if (dostype[i] < 32)
                    dostype[i] = '.';
            }
            dostype[4] = 0;

           size = (de->de_HighCyl - de->de_LowCyl + 1) * de->de_Surfaces * de->de_BlocksPerTrack;
           /* try to prevent ULONG overflow */
           if (de->de_SizeBlock <= 128)
                   size /= 2;
            else
                    size *= de->de_SizeBlock / 256;

            NewRawDoFmt("%s [%08lx] %ldk", RAWFMTFUNC_STRING, text,
                dostype, de->de_DosType,
                size);
            textp = text;
        } else if (!devopen) {
            textp = "[device open error]";
        } else if (!ismedia) {
            textp = "[no media]";
        }
        if (textp) {
            Move(win->RPort, 400, y);
            Text(win->RPort, textp, strlen(textp));
        }

        y += 16;
        
        
    }
}

static void centertext(LIBBASETYPEPTR DOSBootBase, BYTE pen, WORD y, const char *text)
{
    struct Window *win = DOSBootBase->bm_Window;
    SetAPen(win->RPort, pen);
    Move(win->RPort, win->Width / 2 - TextLength(win->RPort, text, strlen(text)) / 2, y);
    Text(win->RPort, text, strlen(text));
}

static void initPage(LIBBASETYPEPTR DOSBootBase, WORD page)
{
    UBYTE *text;

    if (page == PAGE_DISPLAY)
            text = "Display Options";
    else if (page == PAGE_EXPANSION)
            text = "Expansion Board Diagnostic";
    else if (page == PAGE_BOOT)
        text = "Boot Options";
    else
        text = "AROS Early Startup Control";
    centertext(DOSBootBase, 2, 10, text);
    
    if (page == PAGE_BOOT)
        initPageBoot(DOSBootBase);
    else if (page == PAGE_EXPANSION)
        initPageExpansion(DOSBootBase);

    if (page == PAGE_MAIN && (GfxBase->DisplayFlags & (NTSC | PAL))) {
            ULONG modeid = GetVPModeID(&DOSBootBase->bm_Screen->ViewPort);
            if (modeid != INVALID_ID && (((modeid & MONITOR_ID_MASK) == NTSC_MONITOR_ID) || ((modeid & MONITOR_ID_MASK) == PAL_MONITOR_ID))) {
            centertext(DOSBootBase, 1, 30, "(press a key to toggle the display between PAL and NTSC)");
        }
    }

    if (page == PAGE_BOOT)
        centertext(DOSBootBase, 1, 30, "Press A-J to select boot device");
}

static WORD initWindow(LIBBASETYPEPTR DOSBootBase, struct BootConfig *bcfg, WORD page)
{
    struct Gadget *first = NULL;
    WORD newpage = -1;

    if ((first = createGadgets(DOSBootBase, page)) != NULL)
    {
        struct NewWindow nw =
        {
            0, 0,                            /* Left, Top */
            DOSBootBase->bm_Screen->Width,   /* Width, Height */
            DOSBootBase->bm_Screen->Height,
            0, 1,                            /* DetailPen, BlockPen */
            IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_VANILLAKEY | IDCMP_GADGETUP | IDCMP_GADGETDOWN, /* IDCMPFlags */
            WFLG_SMART_REFRESH | WFLG_BORDERLESS | WFLG_ACTIVATE, /* Flags */
            first,                            /* FirstGadget */
            NULL,                            /* CheckMark */
            NULL,                            /* Title */
            DOSBootBase->bm_Screen,             /* Screen */
            NULL,                            /* BitMap */
            0, 0,                                /* MinWidth, MinHeight */
            0, 0,                            /* MaxWidth, MaxHeight */
            CUSTOMSCREEN,                    /* Type */
        };

        D(bug("[BootMenu] initPage: Gadgets created @ %p\n", first));

        if ((DOSBootBase->bm_Window = OpenWindow(&nw)) != NULL)
        {
            D(bug("[BootMenu] initScreen: Window opened @ %p\n", DOSBootBase->bm_Window));
            D(bug("[BootMenu] initScreen: Window RastPort @ %p\n", DOSBootBase->bm_Window->RPort));
            D(bug("[BootMenu] initScreen: Window UserPort @ %p\n", DOSBootBase->bm_Window->UserPort));
            initPage(DOSBootBase, page);
            newpage = msgLoop(DOSBootBase, DOSBootBase->bm_Window);
        }
        CloseWindow(DOSBootBase->bm_Window);
    }
    freeGadgets(DOSBootBase, page);
    
    return newpage;
}

static BOOL initScreen(LIBBASETYPEPTR DOSBootBase, struct BootConfig *bcfg)
{
    WORD page;

    D(bug("[BootMenu] initScreen()\n"));

    page = -1;
    DOSBootBase->bm_Screen = OpenBootScreen(DOSBootBase);
    if (DOSBootBase->bm_Screen)
    {
        DOSBootBase->bottomY = DOSBootBase->bm_Screen->Height - (DOSBootBase->bm_Screen->Height > 256 ? 32 : 16);
        D(bug("[BootMenu] initScreen: Screen opened @ %p\n",  DOSBootBase->bm_Screen));

        page = PAGE_MAIN;
        do {
            page = initWindow(DOSBootBase, bcfg, page);
        } while (page != EXIT_BOOT && page != EXIT_BOOT_WNSS);
        CloseBootScreen(DOSBootBase->bm_Screen, DOSBootBase);
    }
    return page >= 0;
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

#ifdef mc68000
    /*
     * On m68k we may have ciaa.resource (if running on classic Amiga HW)
     * Let's check mouse buttons.
     */
    if (OpenResource("ciaa.resource"))
    {
            volatile UBYTE *cia = (UBYTE*)0xbfe001;
            volatile UWORD *potinp = (UWORD*)0xdff016;

            /* check left + right mouse button state */
            if ((cia[0] & 0x40) == 0 && (potinp[0] & 0x0400) == 0)
                return TRUE;
    }
#endif

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

int bootmenu_Init(LIBBASETYPEPTR LIBBASE, BOOL WantBootMenu)
{
    BOOL bmi_RetVal = FALSE;

    D(bug("[BootMenu] bootmenu_Init()\n"));

#ifdef INITHIDDS_KLUDGE
   /*
    * PCI hardware display drivers still need external initialization.
    * This urgently needs to be fixed. After fixing this kludge
    * will not be needed any more.
    */
    InitBootConfig(&LIBBASE->bm_BootConfig);
    if (!initHidds(LIBBASE))
        return FALSE;
#endif

    /* check keyboard if needed */
    if (!WantBootMenu)
        WantBootMenu = buttonsPressed(LIBBASE);

    /* Bring up early startup menu if requested */
    if (WantBootMenu)
    {
        D(kprintf("[BootMenu] bootmenu_Init: Entering Boot Menu ...\n"));
        bmi_RetVal = initScreen(LIBBASE, &LIBBASE->bm_BootConfig);
    }

    /* Make the user's selection the top boot device */
    setBootDevice(LIBBASE);

    return bmi_RetVal;
}
