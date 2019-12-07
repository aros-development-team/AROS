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
    LONG *userclip;
    LONG *bblank;
    LONG *usercopper;
};

int main(int argc, char **argv)
{
    struct copargs args = {NULL};
    struct RDArgs *rda;
	struct Screen *pubscreen;
	struct TagItem vcTags[] = 
	{
		VTAG_USERCLIP_SET, 0,
		VTAG_BORDERBLANK_SET, 0,
		VTAG_END_CM, 0
	};

	if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 0))) 
    {
		if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0))) 
		{
			rda = ReadArgs("USERCLIP/K/N,BORDERBLANK/K/N,USERCOPPER/K/N", (IPTR *)&args, NULL);
			if (rda) {
				if (args.userclip)
					vcTags[0].ti_Data = 1;
				if (args.bblank)
					vcTags[1].ti_Data = 1;
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
					uCop = pubscreen->ViewPort.UCopIns;
					pubscreen->ViewPort.UCopIns = NULL;
					Permit();
				}
				VideoControl( pubscreen->ViewPort.ColorMap, vcTags);
				UnlockPubScreen(NULL, pubscreen);
				MakeScreen(pubscreen);
				RethinkDisplay();
			}
		}
	}
}
