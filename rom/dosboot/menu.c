/*
   Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

   Desc: Main bootmenu code
*/

#include <aros/config.h>
#include <aros/debug.h>

#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/expansion.h>
#include <proto/oop.h>

#include <devices/keyboard.h>
#include <devices/rawkeycodes.h>
#include <devices/timer.h>
#include <exec/memory.h>
#include <graphics/driver.h>
#include <libraries/expansionbase.h>
#include <libraries/configvars.h>
#include <libraries/gadtools.h>
#include <dos/filehandler.h>
#include <devices/trackdisk.h>
#include <exec/rawfmt.h>
#include <aros/bootloader.h>
#include <aros/symbolsets.h>

#include <string.h>

#include LC_LIBDEFS_FILE

#include "dosboot_intern.h"
#include "menu.h"

#define PAGE_MAIN 1
#define PAGE_BOOT 2
#define PAGE_DISPLAY 3
#define PAGE_EXPANSION 4
#define EXIT_BOOT 5
#define EXIT_BOOT_WNSS 6

#define BUTTON_BOOT            1
#define BUTTON_BOOT_WNSS       2
#define BUTTON_BOOT_OPTIONS    3
#define BUTTON_DISPLAY_OPTIONS 4
#define BUTTON_EXPBOARDDIAG    5
#define BUTTON_USE             6
#define BUTTON_CANCEL          7
#define BUTTON_CONTINUE        8
#define BUTTONLIST_BOOT        10
#define BUTTONLIST_DEVICES     11


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
    
    GfxBase = (void *)TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    if (GfxBase)
    {
        struct Library *OOPBase = OpenLibrary("oop.library", 0);
        if (OOPBase)
        {
            gfxclass = OOP_FindClass(gfxclassname);
            if (gfxclass)
            {
                if (!AddDisplayDriver(gfxclass, NULL, DDRV_BootMode, bootmode, TAG_DONE))
                    success = TRUE;
            }
            CloseLibrary(OOPBase);
        }
        CloseLibrary(&GfxBase->LibNode);
    }
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

static LONG centerx(LIBBASETYPEPTR DOSBootBase, LONG width)
{
    return (DOSBootBase->bm_Screen->Width - width) / 2;
}

static LONG rightto(LIBBASETYPEPTR DOSBootBase, LONG width, LONG right)
{
    return DOSBootBase->bm_Screen->Width - width - right;
}




static void centertext(LIBBASETYPEPTR DOSBootBase, BYTE pen, WORD y, const char *text)
{
    struct Window *win = DOSBootBase->bm_Window;
    SetAPen(win->RPort, pen);
    Move(win->RPort, win->Width / 2 - TextLength(win->RPort, text, strlen(text)) / 2, y);
    Text(win->RPort, text, strlen(text));
}


//////////////////////////////////////////////////////////////////////////


static BOOL populateGadgets_PageMain(LIBBASETYPEPTR DOSBootBase, struct Gadget *gadget)
{
    struct NewGadget ng;

    LONG cx = centerx((struct DOSBootBase *)DOSBootBase, 280);

    ng.ng_Width = 280;
    ng.ng_Height = 15;
    ng.ng_TextAttr = NULL;
    ng.ng_Flags = 0;
    ng.ng_VisualInfo = DOSBootBase->bm_VisualInfo;
    ng.ng_UserData = 0;

    if (gadget != NULL)
    {
        ng.ng_GadgetText = "Boot Options...";
        ng.ng_GadgetID = BUTTON_BOOT_OPTIONS;
        ng.ng_LeftEdge = cx;
        ng.ng_TopEdge = 63;
        gadget = CreateGadgetA(BUTTON_KIND, gadget, &ng, NULL);
    }

    if (gadget != NULL)
    {
        ng.ng_GadgetText = "Display Options...";
        ng.ng_GadgetID = BUTTON_DISPLAY_OPTIONS;
        ng.ng_LeftEdge = cx;
        ng.ng_TopEdge = 84;
        gadget = CreateGadgetA(BUTTON_KIND, gadget, &ng, NULL);
    }

    if (gadget != NULL)
    {
        ng.ng_GadgetText = "Expansion Board Diagnostic...";
        ng.ng_GadgetID = BUTTON_EXPBOARDDIAG;
        ng.ng_LeftEdge = cx;
        ng.ng_TopEdge = 105;
        gadget = CreateGadgetA(BUTTON_KIND, gadget, &ng, NULL);
    }

    return (gadget != NULL);
}



static BOOL populateGadgets_PageBoot(LIBBASETYPEPTR DOSBootBase, struct Gadget *gadget)
{
    // backup of devicesEnabled to be used if user CANCEL your changes
    for (int i=0; i<DOSBootBase->devicesCount; i++)
    {
        DOSBootBase->devicesEnabled[DOSBootBase->devicesCount + i] = DOSBootBase->devicesEnabled[i];
    }


    {
        struct List *bootList;
        struct List *devicesList;

        struct Node *listNode;

        UWORD bootNodeSelected = 0;

        struct BootNode *bn;
        UWORD listIndex = 0;

        NEWLIST (&DOSBootBase->bootList);
        NEWLIST (&DOSBootBase->devicesList);

        bootList = &DOSBootBase->bootList;
        devicesList = &DOSBootBase->devicesList;


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


            if (!fssm || !fssm->fssm_Device)
            {
                listIndex++;
                continue;
            }

            if (fssm->fssm_Environ > (BPTR)0x64)
            {
                de = BADDR(fssm->fssm_Environ);

                if (de->de_TableSize < 15)
                {
                    de = NULL;
                }
            }


            if (IsBootableNode(bn))
            {
                if (listNode = AllocVec(sizeof(struct Node), MEMF_FAST) )
                {
                    listNode->ln_Name = AROS_BSTR_ADDR(dn->dn_Name);
                    listNode->ln_Type = 100L;
                    listNode->ln_Pri = 0;
                    AddTail (bootList, listNode);
                }

                if (DOSBootBase->db_BootNode == bn)
                {
                    bootNodeSelected = listIndex;
                }
            }


            if (listNode = AllocVec(sizeof(struct Node) + 96, MEMF_FAST|MEMF_CLEAR) )
            {
                listNode->ln_Name = (char *)(listNode + 1);
                listNode->ln_Type = 100L;
                listNode->ln_Pri = 0;
                AddTail (devicesList, listNode);
            }


            devopen = ismedia = FALSE;
            if ((port = (struct MsgPort*)CreateMsgPort()))
            {
                if ((io = (struct IOStdReq*)CreateIORequest(port, sizeof(struct IOStdReq))))
                {
                    if (!OpenDevice(AROS_BSTR_ADDR(fssm->fssm_Device), fssm->fssm_Unit, (struct IORequest*)io, fssm->fssm_Flags))
                    {
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


            if (de && ismedia)
            {
                STRPTR sunit = "kMGT";

                for (i = 0; i < 4; i++)
                {
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

                while(size > 1024 * 10)    /* Wrap on 10x unit to be more precise in displaying */
                {
                    size /= 1024;
                    sunit++;
                }


                NewRawDoFmt("%s %5s: %s [%08lx]%5d%c %4d %s-%ld", RAWFMTFUNC_STRING, listNode->ln_Name,
                    (DOSBootBase->devicesEnabled[listIndex]? "Enabled: " : "Disabled:"),
                    AROS_BSTR_ADDR(dn->dn_Name),
                    dostype,
                    de->de_DosType,
                    size,
                    (*sunit),
                    bn->bn_Node.ln_Pri,
                    AROS_BSTR_ADDR(fssm->fssm_Device),
                    fssm->fssm_Unit);
            }
            else if (!devopen)
            {
                NewRawDoFmt("%s %5s: [device open error] %s-%ld", RAWFMTFUNC_STRING, listNode->ln_Name,
                    (DOSBootBase->devicesEnabled[listIndex]? "Enabled: " : "Disabled:"),
                    AROS_BSTR_ADDR(dn->dn_Name),
                    AROS_BSTR_ADDR(fssm->fssm_Device),
                    fssm->fssm_Unit);
            }
            else if (!ismedia)
            {
                NewRawDoFmt("%s %5s: [no media] %s-%ld", RAWFMTFUNC_STRING, listNode->ln_Name,
                    (DOSBootBase->devicesEnabled[listIndex]? "Enabled: " : "Disabled:"),
                    AROS_BSTR_ADDR(dn->dn_Name),
                    AROS_BSTR_ADDR(fssm->fssm_Device),
                    fssm->fssm_Unit);
            }


            listIndex++;
        }


        if (gadget != NULL)
        {
            struct TagItem bootTAGS[] =
            {
                { GTLV_Labels, (IPTR)bootList },    // (IPTR)
                { GTLV_ShowSelected, 0 },
                { GTLV_Selected, bootNodeSelected },
                { GTLV_MakeVisible, 0},
                { TAG_DONE }
            };

            struct NewGadget bootGadget;

            bootGadget.ng_LeftEdge = 10;
            bootGadget.ng_TopEdge = 40;
            bootGadget.ng_Width = 100;
            bootGadget.ng_Height = 100;
            bootGadget.ng_GadgetText = "Boot from:";
            bootGadget.ng_TextAttr = NULL;
            bootGadget.ng_GadgetID = BUTTONLIST_BOOT;
            bootGadget.ng_Flags = 0;
            bootGadget.ng_VisualInfo = DOSBootBase->bm_VisualInfo;
            bootGadget.ng_UserData = 0;

            gadget = CreateGadgetA(LISTVIEW_KIND, gadget, &bootGadget, bootTAGS);
        }


        if (gadget != NULL)
        {
            struct TagItem devicesTAGS[] =
            {
                { GTLV_Labels, (IPTR)devicesList },    // (IPTR)
//                { GTLV_ShowSelected, 0 },
//                { GTLV_Selected, 0 },
//                { GTLV_MakeVisible, 0},
                { TAG_DONE }
            };

            struct NewGadget devicesGadget;

            devicesGadget.ng_LeftEdge = 120;
            devicesGadget.ng_TopEdge = 40;
            devicesGadget.ng_Width = 510;
            devicesGadget.ng_Height = 100;
            devicesGadget.ng_GadgetText = "Devices List";
            devicesGadget.ng_TextAttr = NULL;
            devicesGadget.ng_GadgetID = BUTTONLIST_DEVICES;
            devicesGadget.ng_Flags = 0;
            devicesGadget.ng_VisualInfo = DOSBootBase->bm_VisualInfo;
            devicesGadget.ng_UserData = 0;

            gadget = CreateGadgetA(LISTVIEW_KIND, gadget, &devicesGadget, devicesTAGS);
        }
    }

    return (gadget != NULL);
}


static void freeGadgets_PageBoot(LIBBASETYPEPTR DOSBootBase)
{
    struct Node *node;

    while ((node = RemHead(&DOSBootBase->bootList)) != NULL)
    {
        FreeVec(node);
    }


    while ((node = RemHead(&DOSBootBase->devicesList)) != NULL)
    {
        FreeVec(node);
    }
}



static BOOL populateGadgets(LIBBASETYPEPTR DOSBootBase, struct Gadget *gadget, WORD page)
{
    struct NewGadget ng;

    ng.ng_Width = 280;
    ng.ng_Height = 15;
    ng.ng_TextAttr = NULL;
    ng.ng_Flags = 0;
    ng.ng_VisualInfo = DOSBootBase->bm_VisualInfo;
    ng.ng_UserData = 0;


    if (gadget != NULL)
    {
        ng.ng_GadgetText = (page == PAGE_MAIN ? "Boot" : "Use");
        ng.ng_GadgetID = (page == PAGE_MAIN ? BUTTON_BOOT : BUTTON_USE);
        ng.ng_LeftEdge = 16;
        ng.ng_TopEdge = DOSBootBase->bottomY;
        gadget = CreateGadgetA(BUTTON_KIND, gadget, &ng, NULL);
    }

    if (gadget != NULL)
    {
        ng.ng_GadgetText = (page == PAGE_MAIN ? "Boot With No Startup-Sequence" : "Cancel");
        ng.ng_GadgetID = (page == PAGE_MAIN ? BUTTON_BOOT_WNSS : BUTTON_CANCEL);
        ng.ng_LeftEdge = rightto((struct DOSBootBase *)DOSBootBase, 280, 16);
        ng.ng_TopEdge = DOSBootBase->bottomY;
        gadget = CreateGadgetA(BUTTON_KIND, gadget, &ng, NULL);
    }


    switch (page)
    {
    case PAGE_MAIN:            populateGadgets_PageMain(DOSBootBase, gadget); break;
    case PAGE_BOOT:            populateGadgets_PageBoot(DOSBootBase, gadget); break;
//    case PAGE_DISPLAY:        populateGadgets_PageDisplay(DOSBootBase, gadget); break;
//    case PAGE_EXPANSION:    populateGadgets_PageExpansion(DOSBootBase, gadget); break;
    }

    return (gadget != NULL);
}
static void freeGadgets(LIBBASETYPEPTR DOSBootBase, WORD page)
{
    switch (page)
    {
//    case PAGE_MAIN:            freeGadgets_PageMain(DOSBootBase); break;
    case PAGE_BOOT:            freeGadgets_PageBoot(DOSBootBase); break;
//    case PAGE_DISPLAY:        freeGadgets_PageDisplay(DOSBootBase); break;
//    case PAGE_EXPANSION:    freeGadgets_PageExpansion(DOSBootBase); break;
    }
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

static UWORD msgLoop(LIBBASETYPEPTR DOSBootBase, struct Window *win, WORD page)
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
                if (msg->Class == IDCMP_VANILLAKEY)
                {
                    if (msg->Code == 27)
                            exit = PAGE_MAIN;
                    else if (msg->Code >= '1' && msg->Code <= '3')
                            exit = PAGE_MAIN + msg->Code - '0';
/*                  else if (msg->Code >= 'a' && msg->Code <='j')
                    {
                        BYTE pos = msg->Code - 'a', i = 0;
                        struct BootNode *bn;
                        DOSBootBase->bm_BootNode = NULL;

                        Forbid(); // .. access to ExpansionBase->MountList
                        ForeachNode(&DOSBootBase->bm_ExpansionBase->MountList, bn)
                        {
                            if (i++ == pos)
                            {
                                DOSBootBase->bm_BootNode = bn;
                                break;
                            }
                        }
                        Permit();

                        if (DOSBootBase->bm_BootNode != NULL)
                        {
                            // Refresh itself
                            exit = PAGE_BOOT;
                            break;
                        }
                    }
*/                  else
                    {
                        toggleMode(DOSBootBase);
                    }
                }
                else if (msg->Class == IDCMP_GADGETUP)
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
                        if (page == PAGE_BOOT)
                        {
                            DOSBootBase->bm_BootNode = NULL;

                            for (int i=0; i<DOSBootBase->devicesCount; i++)
                            {
                                DOSBootBase->devicesEnabled[i] = DOSBootBase->devicesEnabled[DOSBootBase->devicesCount + i];
                            }
                        }
                        exit = PAGE_MAIN;
                        break;

                    case BUTTON_USE:
                        /* Preserve selected value */
                        if (page == PAGE_BOOT)
                            if (DOSBootBase->bm_BootNode != NULL)
                                DOSBootBase->db_BootNode = DOSBootBase->bm_BootNode;
                        /* Fallthrough */
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

                    case BUTTONLIST_BOOT:
                    {
                        struct BootNode *bn;
                        BYTE i = 0;
                        BYTE pos = msg->Code;
                        Forbid(); /* .. access to ExpansionBase->MountList */
                        ForeachNode(&DOSBootBase->bm_ExpansionBase->MountList, bn)
                        {
                            if (pos == i++)
                            {
                                DOSBootBase->bm_BootNode = bn;
                                break;
                            }
                        }
                        Permit();
                    }
                    break;

                    case BUTTONLIST_DEVICES:
                    {
                        UWORD pos = msg->Code;

                        DOSBootBase->devicesEnabled[pos] = !DOSBootBase->devicesEnabled[pos];

                        struct Node * node = (&DOSBootBase->devicesList)->lh_Head;
                        while (pos-- > 0)
                        {
                            node = node->ln_Succ;
                        }

                        strncpy(node->ln_Name, (DOSBootBase->devicesEnabled[msg->Code]? "Enabled: " : "Disabled:"), 9);

                        //GT_RefreshWindow(win, NULL);

                        RefreshGList(g, win, NULL, 1);

                    }
                    break;

                    }
                }
                ReplyMsg((struct Message *)msg);
            }
        }
        else
        {
            D(bug("[BootMenu] msgLoop: Window lacks a userport!\n"));
            Wait(0);
        }
    }
    while (exit < 0);


    while ((msg=(struct IntuiMessage *)GetMsg(win->UserPort)))
    {
        ReplyMsg(&msg->ExecMessage);
    }

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
    {
        /* Set the default */
        if (DOSBootBase->bm_BootNode == NULL)
            DOSBootBase->bm_BootNode = DOSBootBase->db_BootNode;
    }
    else if (page == PAGE_EXPANSION)
        initPageExpansion(DOSBootBase);

    if (page == PAGE_MAIN && (GfxBase->DisplayFlags & (NTSC | PAL))) {
            ULONG modeid = GetVPModeID(&DOSBootBase->bm_Screen->ViewPort);
            if (modeid != INVALID_ID && (((modeid & MONITOR_ID_MASK) == NTSC_MONITOR_ID) || ((modeid & MONITOR_ID_MASK) == PAL_MONITOR_ID))) {
            centertext(DOSBootBase, 1, 30, "(press a key to toggle the display between PAL and NTSC)");
        }
    }

}

static WORD initWindow(LIBBASETYPEPTR DOSBootBase, struct BootConfig *bcfg, WORD page)
{
    struct Gadget *gadlist, *firstGadget;
    WORD newpage = -1;

    DOSBootBase->bm_GadToolsBase = TaggedOpenLibrary(TAGGEDOPEN_GADTOOLS);

    DOSBootBase->bm_VisualInfo = GetVisualInfoA(DOSBootBase->bm_Screen, NULL);

    firstGadget = CreateContext(&gadlist);


    if (populateGadgets(DOSBootBase, firstGadget, page))
    {
        struct NewWindow nw =
        {
            0, 0,                            /* Left, Top */
            DOSBootBase->bm_Screen->Width,   /* Width, Height */
            DOSBootBase->bm_Screen->Height,
            0, 1,                            /* DetailPen, BlockPen */
            IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_VANILLAKEY | IDCMP_GADGETUP | IDCMP_GADGETDOWN, /* IDCMPFlags */
            WFLG_SMART_REFRESH | WFLG_BORDERLESS | WFLG_ACTIVATE, /* Flags */
            gadlist,                         /* FirstGadget */
            NULL,                            /* CheckMark */
            NULL,                            /* Title */
            DOSBootBase->bm_Screen,             /* Screen */
            NULL,                            /* BitMap */
            0, 0,                                /* MinWidth, MinHeight */
            0, 0,                            /* MaxWidth, MaxHeight */
            CUSTOMSCREEN,                    /* Type */
        };


        if ((DOSBootBase->bm_Window = OpenWindow(&nw)) != NULL)
        {
            D(bug("[BootMenu] initScreen: Window opened @ %p\n", DOSBootBase->bm_Window));
            D(bug("[BootMenu] initScreen: Window RastPort @ %p\n", DOSBootBase->bm_Window->RPort));
            D(bug("[BootMenu] initScreen: Window UserPort @ %p\n", DOSBootBase->bm_Window->UserPort));

            initPage(DOSBootBase, page);

            newpage = msgLoop(DOSBootBase, DOSBootBase->bm_Window, page);

            freeGadgets(DOSBootBase, page);
        }
        CloseWindow(DOSBootBase->bm_Window);
    }

    FreeGadgets(gadlist);

    FreeVisualInfo(DOSBootBase->bm_VisualInfo);

    CloseLibrary((struct Library *)DOSBootBase->bm_GadToolsBase);
    
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
                        D(bug("[BootMenu] buttonsPressed: Matrix : "));
                        for (i = 0; i < ioStd(io)->io_Actual; i ++)
                        {
                                D(bug("%02x ", matrix[i]));
                        }
                        D(bug("\n"));
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

int bootmenu_Init(LIBBASETYPEPTR DOSBootBase, BOOL WantBootMenu)
{
    BOOL bmi_RetVal = FALSE;

    D(bug("[BootMenu] bootmenu_Init()\n"));

#ifdef INITHIDDS_KLUDGE
   /*
    * PCI hardware display drivers still need external initialization.
    * This urgently needs to be fixed. After fixing this kludge
    * will not be needed any more.
    */
    InitBootConfig(&DOSBootBase->bm_BootConfig);
    if (!initHidds(DOSBootBase))
        return FALSE;
#endif

    /* check keyboard if needed */
    if (!WantBootMenu)
        WantBootMenu = buttonsPressed(DOSBootBase);

    /* Bring up early startup menu if requested */
    if (WantBootMenu)
    {
        D(kprintf("[BootMenu] bootmenu_Init: Entering Boot Menu ...\n"));
        bmi_RetVal = initScreen(DOSBootBase, &DOSBootBase->bm_BootConfig);
    }

    return bmi_RetVal;
}
