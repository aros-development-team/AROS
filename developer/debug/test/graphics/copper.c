/*
    Copyright © 2019-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <graphics/videocontrol.h>
#include <graphics/modeid.h>

#include <stdio.h>

struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

struct copargs
{
    IPTR usercopper;
    LONG *userclip;
    LONG *bblank;
	char *pubscr;
};

int main(int argc, char **argv)
{
    struct copargs args = {0, NULL, NULL, NULL};
    struct RDArgs *rda;
	struct Screen *pubscreen;
	struct TagItem vcTags[] = 
	{
		VTAG_USERCLIP_GET, 0,
		VTAG_BORDERBLANK_GET, 0,
		VTAG_END_CM, 0
	};

	if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 0))) 
    {
		if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0))) 
		{
			char *scrnname = NULL;

			rda = ReadArgs("USERCOPPER/S,USERCLIP/K/N,BORDERBLANK/K/N,PUBSCREEN/K", (IPTR *)&args, NULL);
			if (rda) {
				if (args.userclip)
				{
					if (*args.userclip != 0)
					{
						printf("Enabling USERCLIP\n");
						vcTags[0].ti_Tag = VTAG_USERCLIP_SET;
					}
					else
					{
						printf("Disabling USERCLIP\n");
						vcTags[0].ti_Tag = VTAG_USERCLIP_CLR;
					}
				}
				if (args.bblank)
				{
					if (*args.bblank != 0)
					{
						printf("Enabling BORDERBLANK\n");
						vcTags[1].ti_Tag = VTAG_BORDERBLANK_SET;
					}
					else
					{
						printf("Disabling BORDERBLANK\n");
						vcTags[1].ti_Tag = VTAG_BORDERBLANK_CLR;
					}
				}
				if (args.pubscr)
				{
					scrnname = args.pubscr;
					printf("Using Public Screen '%s'\n", scrnname);
				}
			}

			if ((pubscreen = LockPubScreen(scrnname)) != NULL)
			{
				struct UCopList *uCop = NULL;

				if (args.usercopper)
				{
					if ((uCop = AllocMem(sizeof(struct UCopList), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
					{
						UWORD top = pubscreen->Height;
						if (pubscreen->ViewPort.ColorMap->VPModeID & LORESLACE_KEY)
							top >>= 1;

						printf("Preparing User Copperlist...\n");
						UCopperListInit(uCop, 6 + 1);
						
						CWait(uCop, (top >> 2),0);
						CBump(uCop);
						CMove(uCop, (APTR)0x0180, 0xF00);
						CBump(uCop);

						CWait(uCop, (top >> 1),0);
						CBump(uCop);
						CMove(uCop, (APTR)0x0180, 0xF0);
						CBump(uCop);

						CWait(uCop, (top >> 1) +  (top >> 2),0);
						CBump(uCop);
						CMove(uCop, (APTR)0x0180, 0xF);
						CBump(uCop);

						CWait(uCop, 10000, 0xFF);
						CBump(uCop);
					}
				}

				Forbid();
				/* make sure there is nothing currently attached .. */
				printf("Clearing ViewPorts Copperlist...\n");
				FreeVPortCopLists(&pubscreen->ViewPort);
				if (uCop)
				{
					printf("Setting User Copperlist...\n");
					pubscreen->ViewPort.UCopIns = uCop;
				}
				Permit();

				VideoControl( pubscreen->ViewPort.ColorMap, vcTags);
				UnlockPubScreen(NULL, pubscreen);
				MakeScreen(pubscreen);
				RethinkDisplay();
				printf("Done\n");
			}
			CloseLibrary((struct Library *)GfxBase);
		}
		CloseLibrary((struct Library *)IntuitionBase);
	}
}
