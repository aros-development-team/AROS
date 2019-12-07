/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <graphics/videocontrol.h>

struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

struct copargs
{
    IPTR usercopper;
    LONG *userclip;
    LONG *bblank;
};

int main(int argc, char **argv)
{
    struct copargs args = {0, NULL, NULL};
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
			rda = ReadArgs("USERCOPPER/S,USERCLIP/K/N,BORDERBLANK/K/N", (IPTR *)&args, NULL);
			if (rda) {
				if (args.userclip)
				{
					if (*args.userclip != 0)
						vcTags[0].ti_Tag = VTAG_USERCLIP_SET;
					else
						vcTags[0].ti_Tag = VTAG_USERCLIP_CLR;
				}
				if (args.bblank)
				{
					if (*args.bblank != 0)
						vcTags[1].ti_Tag = VTAG_BORDERBLANK_SET;
					else
						vcTags[1].ti_Tag = VTAG_BORDERBLANK_CLR;
				}
			}

			if ((pubscreen = LockPubScreen(NULL)) != NULL)
			{
				struct UCopList *uCop;

				if (args.usercopper)
				{
					if ((uCop = AllocMem(sizeof(struct UCopList), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
					{
						UCopperListInit(uCop, 6 + 1);
						
						CWait(uCop, (pubscreen->Height >> 2),0);
						CBump(uCop);
						CMove(uCop, 0x0180, 0xF00);
						CBump(uCop);

						CWait(uCop, (pubscreen->Height >> 1),0);
						CBump(uCop);
						CMove(uCop, 0x0180, 0xF0);
						CBump(uCop);

						CWait(uCop, (pubscreen->Height >> 1) +  (pubscreen->Height >> 2),0);
						CBump(uCop);
						CMove(uCop, 0x0180, 0xF);
						CBump(uCop);

						CWait(uCop, 10000, 0xFF);
						CBump(uCop);
						
						Forbid();
						pubscreen->ViewPort.UCopIns = uCop;
						Permit();

					}
				}
				else
				{
					Forbid();
					FreeVPortCopLists(&pubscreen->ViewPort);
					Permit();
				}
				VideoControl( pubscreen->ViewPort.ColorMap, vcTags);
				UnlockPubScreen(NULL, pubscreen);
				MakeScreen(pubscreen);
				RethinkDisplay();
			}
			CloseLibrary(GfxBase);
		}
		CloseLibrary(IntuitionBase);
	}
}
