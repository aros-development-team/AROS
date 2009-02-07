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

	D(bug("[BootMenu] init_gfx: calling private LateGfxInit() ..\n"));
	if (LateGfxInit(gfxclassname))
	{
		D(bug("[BootMenu] init_gfx: calling private LateGfxInit Succeeded\n"));
		if (LateIntuiInit(NULL))
		{
			D(bug("[BootMenu] init_gfx: calling private LateIntuiInit Succeeded\n"));
			success = TRUE;
		}
		else
		{
			D(bug("[BootMenu] init_gfx: calling private LateIntuiInit Failed!\n"));
		}
	}
	else
	{
		D(bug("[BootMenu] init_gfx: calling private LateGfxInit Failed!\n"));
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

	if ((OpenLibrary(bootcfg->defaultgfx.libname, 0)) != NULL)
        {
            if ((init_gfx(bootcfg->defaultgfx.hiddname, BootMenuBase)) == TRUE)
            {

                if (!bootcfg->defaultmouse.hiddname[0])
                {
                        return TRUE;
                }

                if ((OpenLibrary(bootcfg->defaultmouse.libname, 0)) != NULL)
                {
                    if ((init_device(bootcfg->defaultmouse.hiddname, "gameport.device", BootMenuBase)) == TRUE)
                    {

                        D(bug("[BootMenu] initHidds: Hidds initialised\n"));

                        return TRUE;
                    }
                }
            }
        }
        return FALSE;
}

static struct Gadget *createGadgets(struct BootMenuBase_intern *BootMenuBase) 
{
	struct Gadget *first;
	struct ButtonGadget *last;

#warning "TOD: This is very unclean! free-up resources if we fail!"

	last = BootMenuBase->bm_MainGadgets.boot = createButton(16, 190, 280, 14, NULL, "Boot", BUTTON_BOOT, BootMenuBase);
	if (last == NULL)
		return NULL;

        first = (struct Gadget *)&last->gadget;;
        
	last = BootMenuBase->bm_MainGadgets.bootnss = createButton(344, 190, 280, 14, &last->gadget, "Boot With No Startup-Sequence", BUTTON_BOOT_WNSS, BootMenuBase);
	if (last == NULL)
		return NULL;

	last = BootMenuBase->bm_MainGadgets.bootopt = createButton(180, 63, 280, 14, &last->gadget, "Boot Options...", BUTTON_BOOT_OPTIONS, BootMenuBase);
	if (last == NULL)
		return NULL;

	last = BootMenuBase->bm_MainGadgets.displayopt = createButton(180, 84, 280, 14, &last->gadget, "Display Options...", BUTTON_DISPLAY_OPTIONS, BootMenuBase);
	if (last == NULL)
		return NULL;

	last = BootMenuBase->bm_MainGadgets.expboarddiag = createButton(180, 105, 280, 14, &last->gadget, "Expansion Board Diagnostic...", BUTTON_EXPBOARDDIAG, BootMenuBase);
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
	BOOL exit = FALSE;
	struct IntuiMessage *msg;
	struct Gadget *g;

	D(bug("[BootMenu] msgLoop(BootMenuBase @ %p, Window @ %p, Cfg @ %p)\n", BootMenuBase, win, bcfg));

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
					ExpansionBase->Flags &= ~EBF_DOSFLAG;
					exit = TRUE;
					break;
				case BUTTON_BOOT_WNSS:
					ExpansionBase->Flags |= EBF_DOSFLAG;
					exit = TRUE;
					break;
				}
			}
			ReplyMsg((struct Message *)msg);
		}
            }
            else
            {
                D(bug("[BootMenu] msgLoop: Window lacks a userport!\n"));
            }
	} while (exit == FALSE);
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

	D(bug("[BootMenu] initScreen()\n"));

	if ((BootMenuBase->bm_Screen = OpenScreenTagList(NULL, scrtags)) != NULL)
	{
		D(bug("[BootMenu] initScreen: Screen opened @ %p\n", BootMenuBase->bm_Screen));
		if ((first = createGadgets(BootMenuBase)) != NULL)
		{
			D(bug("[BootMenu] initScreen: Gadgets created @ %p\n", first));
			wintags[2].ti_Data = BootMenuBase->bm_Screen->Width;
			wintags[3].ti_Data = BootMenuBase->bm_Screen->Height;
			wintags[4].ti_Data = (IPTR)BootMenuBase->bm_Screen;
			wintags[5].ti_Data = (IPTR)first;
			if ((BootMenuBase->bm_Window = OpenWindowTagList(NULL, wintags)) != NULL)
			{
				D(bug("[BootMenu] initScreen: Window opened @ %p\n", first));
                                D(bug("[BootMenu] initScreen: Window RastPort @ %p\n", BootMenuBase->bm_Window->RPort));
				SetAPen(BootMenuBase->bm_Window->RPort, 2);
                                D(bug("[BootMenu] initScreen: SetAPen 2\n"));
				Move(BootMenuBase->bm_Window->RPort, 215, 20);
                                D(bug("[BootMenu] initScreen: Move(d) to 215, 20\n"));
				Text(BootMenuBase->bm_Window->RPort, "AROS Early Startup Control", 26);
                                D(bug("[BootMenu] initScreen: Early Startup text displayed\n"));
#warning "TODO: Remove the the following text?"
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

/* From keyboard.device/keyboard_intern.h */
#define KB_MAXKEYS     256
#define KB_MATRIXSIZE  (KB_MAXKEYS/(sizeof(UBYTE)*8))
#define ioStd(x) ((struct IOStdReq *)x)

static BOOL buttonsPressed(struct BootMenuBase *BootMenuBase, struct DefaultHidd *kbd) 
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
						for (i = 0; i < sizeof(matrix); i ++)
						{
							bug("%2x ", matrix[i]);
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
	struct DefaultHidd *kbd = &LIBBASE->bm_BootConfig.defaultkbd;
        int bmi_RetVal = (int)FALSE;
	D(bug("[BootMenu] bootmenu_Init()\n"));

	LIBBASE->bm_Force = FALSE; /* Set FALSE here to be sure .. */

	if ((ExpansionBase = OpenLibrary("expansion.library", 0)) != NULL)
	{
		if ((GfxBase = OpenLibrary("graphics.library", 37)) != NULL)
		{
			if ((IntuitionBase = OpenLibrary("intuition.library", 37)) != NULL)
			{
				BootLoaderBase = OpenResource("bootloader.resource");
				InitBootConfig(&LIBBASE->bm_BootConfig, BootLoaderBase);
				if (BootLoaderBase)
				{
					struct List *list = NULL;
					struct Node *node = NULL;

					if ((list = (struct List *)GetBootInfo(BL_Args)) != NULL)
					{
						ForeachNode(list,node)
						{
							if (0 == strcmp(node->ln_Name,"bootmenu"))
							{
								D(bug("[BootMenu] bootmenu_Init: Forced with bootloader argument\n"));
								LIBBASE->bm_Force = TRUE;
							}
						}
					}
				}

				if (!kbd->hiddname[0]) {
					D(bug("[BootMenu] bootmenu_Init: This system uses no keyboard HIDD\n"));
					bmi_RetVal = (int)TRUE;
				}
				if (OpenLibrary(kbd->libname, 0) != NULL)
				{
					if (init_device(kbd->hiddname, "keyboard.device", LIBBASE))
					{
						bmi_RetVal = (int)TRUE;
					}
				}
			}
		}
	}
	/* check keyboard */
	if ((bmi_RetVal) && (LIBBASE->bm_Force || buttonsPressed(LIBBASE, &LIBBASE->bm_BootConfig.defaultkbd)))
	{
		D(kprintf("[BootMenu] bootmenu_Init: Entering Boot Menu ...\n"));
		/* init mouse + gfx */
		if (initHidds(&LIBBASE->bm_BootConfig, LIBBASE))
		{
			D(bug("[BootMenu] bootmenu_Init: Hidds Initialised\n"));
			initScreen(LIBBASE, &LIBBASE->bm_BootConfig);
		}
                else
                {
                    D(bug("[BootMenu] bootmenu_Init: Hidds Failed to initialise!\n"));
                }
	}
        else
        {
            D(bug("[BootMenu] bootmenu_Init: Menu not requested ..\n"));
        }
	return bmi_RetVal;
}

ADD2INITLIB(bootmenu_Init, 0)
