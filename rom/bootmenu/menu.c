#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <devices/keyboard.h>
#include <devices/rawkeycodes.h>
#include <devices/timer.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <libraries/bootmenu.h>
#include <aros/symbolsets.h>
#include <string.h>
#include "devs_private.h"
#define DEBUG 0
#include <aros/debug.h>

#include "bootmenu_intern.h"

#include LC_LIBDEFS_FILE

#define BUFSIZE 100

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

BOOL initScreen(STRPTR gfxhiddname, struct BootMenuBase *bootmenubase) {
struct TagItem modetags[] =
{
	{BIDTAG_Depth, 2},
	{BIDTAG_DesiredWidth, 640},
	{BIDTAG_DesiredHeight, 200},
	{TAG_DONE, 0UL}
};
ULONG modeid;

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

static BOOL init_gfx(STRPTR gfxclassname, struct BootMenuBase *bootmenubase)
{
	BOOL success = FALSE; 
	EnterFunc(bug("init_gfx(hiddbase=%s)\n", gfxclassname));
    
	/*  Call private gfx.library call to init the HIDD.
	    Gfx library is responsable for closing the HIDD
	    library (although it will probably not be neccesary).
	*/

	D(bug("calling private gfx LateGfxInit()\n"));
	if (LateGfxInit(gfxclassname))
	{
	    D(bug("success\n"));
	    if (IntuitionBase)
	    {
	    	if (LateIntuiInit(NULL))
	    	{
	    	    success = TRUE;
		}
	    }
	}
	ReturnBool ("init_gfxhidd", success);
}


static BOOL init_device( STRPTR hiddclassname, STRPTR devicename,  struct BootMenuBase *bootmenubase)
{
	BOOL success = FALSE;
	struct MsgPort *mp;

	EnterFunc(bug("init_device(classname=%s)\n", hiddclassname));

	mp = CreateMsgPort();
	if (mp)
	{
		struct IORequest *io;
		io = CreateIORequest(mp, sizeof ( struct IOStdReq));
		if (io)
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

BOOL initHidds(struct BootConfig *bootcfg, struct BootMenuBase *bootmenubase) {

	OpenLibrary(bootcfg->defaultgfx.libname, 0);
	init_gfx(bootcfg->defaultgfx.hiddname, bootmenubase);
	OpenLibrary(bootcfg->defaultmouse.libname, 0);
	init_device(bootcfg->defaultmouse.hiddname, "gameport.device", bootmenubase);
	return TRUE;
}

struct Gadget *createGadgets(struct BootMenuBase_intern *bootmenubase) {
struct Gadget *first;
struct ButtonGadget *last;

	last = bootmenubase->maingadgets.boot = createButton(16, 190, 280, 14, (struct Gadget *)&first, "Boot", BUTTON_BOOT, bootmenubase);
	if (last == NULL)
		return NULL;
	last = bootmenubase->maingadgets.bootnss = createButton(344, 190, 280, 14, last->gadget, "Boot With No Startup-Sequence", BUTTON_BOOT_WNSS, bootmenubase);
	if (last == NULL)
		return NULL;
	last = bootmenubase->maingadgets.bootopt = createButton(180, 63, 280, 14, last->gadget, "Boot Options...", BUTTON_BOOT_OPTIONS, bootmenubase);
	if (last == NULL)
		return NULL;
	last = bootmenubase->maingadgets.displayopt = createButton(180, 84, 280, 14, last->gadget, "Display Options...", BUTTON_DISPLAY_OPTIONS, bootmenubase);
	if (last == NULL)
		return NULL;
	last = bootmenubase->maingadgets.expboarddiag = createButton(180, 105, 280, 14, last->gadget, "Expansion Board Diagnostic...", BUTTON_EXPBOARDDIAG, bootmenubase);
	if (last == NULL)
		return NULL;
	return first;
}

void freeGadgets(struct BootMenuBase_intern *bootmenubase) {

	if (bootmenubase->maingadgets.boot != NULL)
		freeButtonGadget(bootmenubase->maingadgets.boot, bootmenubase);
	if (bootmenubase->maingadgets.bootnss != NULL);
		freeButtonGadget(bootmenubase->maingadgets.bootnss, bootmenubase);
	if (bootmenubase->maingadgets.bootopt != NULL)
		freeButtonGadget(bootmenubase->maingadgets.bootopt, bootmenubase);
	if (bootmenubase->maingadgets.displayopt != NULL)
		freeButtonGadget(bootmenubase->maingadgets.displayopt, bootmenubase);
	if (bootmenubase->maingadgets.expboarddiag != NULL)
		freeButtonGadget(bootmenubase->maingadgets.expboarddiag, bootmenubase);
}

void msgLoop(struct BootMenuBase *bootmenubase, struct Window *win, struct BootConfig *bcfg) {
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
					bcfg->startup_sequence = TRUE;
					in = FALSE;
					break;
				case BUTTON_BOOT_WNSS:
					bcfg->startup_sequence = FALSE;
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

BOOL initScreen(struct BootMenuBase_intern *bootmenubase, struct BootConfig *bcfg) {
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

	bootmenubase->scr = OpenScreenTagList(NULL, scrtags);
	if (bootmenubase->scr != NULL)
	{
		first = createGadgets(bootmenubase);
		if (first != NULL)
		{
			wintags[2].ti_Data = bootmenubase->scr->Width;
			wintags[3].ti_Data = bootmenubase->scr->Height;
			wintags[4].ti_Data = (IPTR)bootmenubase->scr;
			wintags[5].ti_Data = (IPTR)first;
			bootmenubase->win = OpenWindowTagList(NULL, wintags);
			if (bootmenubase->win != NULL)
			{
				SetAPen(bootmenubase->win->RPort, 2);
				Move(bootmenubase->win->RPort, 215, 20);
				Text(bootmenubase->win->RPort, "AROS Early Startup Control", 26);
				SetAPen(bootmenubase->win->RPort, 1);
				Move(bootmenubase->win->RPort, 225, 40);
				Text(bootmenubase->win->RPort, "(what is PAL and NTSC?)", 23);
				msgLoop(bootmenubase, bootmenubase->win, bcfg);
				return TRUE;
			}
			else
				Alert(AT_DeadEnd | AN_OpenWindow);
			CloseWindow(bootmenubase->win);
			freeGadgets(bootmenubase);
		}
		else
			Alert(AT_DeadEnd | AN_BadGadget);
		CloseScreen(bootmenubase->scr);
	}
	else
		Alert(AT_DeadEnd | AN_OpenScreen);
	return FALSE;
}

void delay(struct BootMenuBase *bootmenubase, LONG secs, LONG micro) {
struct MsgPort *mp;

	mp = CreateMsgPort();
	if (mp)
	{
		struct timerequest *tr;
		tr = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest));
		if (tr)
		{
			if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0) == 0)
			{
				#define ioStd(x) ((struct IOStdReq *)x)
				ioStd(tr)->io_Command = TR_ADDREQUEST;
				tr->tr_time.tv_secs = secs;
				tr->tr_time.tv_micro = micro;
				DoIO((struct IORequest *)tr);
				CloseDevice((struct IORequest *)tr);
			}
			DeleteIORequest((struct IORequest *)tr);
		}
		DeleteMsgPort(mp);
	}
}

BOOL buttonsPressed(struct BootMenuBase *bootmenubase, struct DefaultHidd *kbd) {
BOOL success = FALSE;
struct MsgPort *mp;
UBYTE matrix[16];

	if (OpenLibrary(kbd->libname, 0) != NULL)
	{
		if (init_device(kbd->hiddname, "keyboard.device", bootmenubase))
		{
			delay(bootmenubase, 1, 0);
			mp = CreateMsgPort();
			if (mp)
			{
				struct IORequest *io;
				io = CreateIORequest(mp, sizeof ( struct IOStdReq));
				if (io)
				{
					if (0 == OpenDevice("keyboard.device", 0, io, 0))
					{
						#define ioStd(x) ((struct IOStdReq *)x)
						ioStd(io)->io_Command = KBD_READMATRIX;
						ioStd(io)->io_Data = matrix;
						ioStd(io)->io_Length = 16;
						DoIO(io);
						if (0 == io->io_Error)
						{
							if (matrix[RAWKEY_SPACE/8] & (1<<(RAWKEY_SPACE%8)))
								success = TRUE;
						}
						CloseDevice(io);
					}
					DeleteIORequest(io); 
				}
				DeleteMsgPort(mp);
			}
		}
	}
	return success;
}

#endif


AROS_SET_LIBFUNC(CheckAndDisplay, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    static struct BootConfig bootcfg =
    {
	&bootcfg,
	{"vgah.hidd", "hidd.gfx.vga"},
	{"kbd.hidd", "hidd.kbd.hw"},
	{"mouse.hidd", "hidd.bus.mouse"},
	NULL,
	TRUE
    };

    LIBBASE->bcfg = bootcfg;

    /* init keyboard + check */
    if (buttonsPressed(LIBBASE, &LIBBASE->bcfg.defaultkbd))
    {
	kprintf("Entering Boot Menu ...\n");
	/* init mouse + gfx */
	if (initHidds(&LIBBASE->bcfg, LIBBASE))
	{
	    initScreen(LIBBASE, &LIBBASE->bcfg);
	}
    }

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(CheckAndDisplay, 0)
