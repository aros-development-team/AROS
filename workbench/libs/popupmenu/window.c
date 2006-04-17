//
// Popup Menu Window funcs
// ©1996-2002 Henrik Isaksson
//

#include "pmpriv.h"

//
// Allocate memory and copy a RastPort structure
//
struct RastPort *PM_CpyRPort(struct RastPort *rp)
{
#ifdef __AROS__
    	return CloneRastPort(rp);
#else
	struct RastPort *rpc;

	rpc=PM_Mem_Alloc(sizeof(struct RastPort));
	if(rpc) {
		CopyMem(rp, rpc, sizeof(struct RastPort));
	}

	return rpc;
#endif
}

//
// Copy the region beneath a window to the transparency buffer.
//
void PM_TransparencyBfr(struct PM_Window *bw)
{
	int j, bpp;
	ULONG	transparent = FALSE;

#ifndef __AROS__
	GetGUIAttrs(NULL, bw->p->DrawInfo, GUIA_MenuTransparency, &transparent, TAG_DONE);
#endif

	if(CyberGfx && transparent) {
		if(GetCyberMapAttr(bw->Wnd->WScreen->RastPort.BitMap, CYBRMATTR_ISCYBERGFX)) {
			bpp = GetCyberMapAttr(bw->Wnd->WScreen->RastPort.BitMap, CYBRMATTR_BPPIX);
			if(bpp<2)
				return;
		} else return;

		// If we've gotten this far, the screen is CyberGfx and > 15 bpp

		bw->bg.BgArray = PM_Mem_Alloc(3 * bw->Width * bw->Height);
		if(bw->bg.BgArray) {
			ReadPixelArray(bw->bg.BgArray, 0, 0,
				bw->Width * 3, bw->RPort, 0, 0,
				bw->Width, bw->Height, RECTFMT_RGB);
			// Fast and simple way to blend the background to gray...
			for(j = 0; j < bw->Width * bw->Height * 3; j++) {
				bw->bg.BgArray[j] = (bw->bg.BgArray[j]>>2) + 150;
			}
		}
	}
}

//
// Create an offscreen buffer for rendering animation and transition
// effects.
//
void PM_OffScreenBfr(struct PM_Window *bw)
{
	if(PM_Prefs->pmp_Animation) {
	        bw->te.BMap=AllocBitMap(bw->Width, bw->Height, bw->Wnd->WScreen->RastPort.BitMap->Depth, BMF_MINPLANES, bw->Wnd->WScreen->RastPort.BitMap);
        	if(bw->te.BMap) {
			bw->te.RPort=PM_CpyRPort(&bw->Wnd->WScreen->RastPort);
			if(bw->te.RPort) {
				bw->te.RPort->Layer = NULL; /* huuu! */
				bw->te.RPort->BitMap = bw->te.BMap;
			}
		}
	}
}

//
// Open a window
//
BOOL PM_OpenWindow(struct PM_Window *pw, int left, int top, int width, int height, struct Screen *scr)
{
	pw->bg.BgArray = NULL;
	pw->te.RPort = NULL;
	pw->te.BMap = NULL;
	pw->Wnd = OpenWindowTags(NULL,
		WA_Borderless,          TRUE,
                WA_RMBTrap,             TRUE,
                WA_Left,                left,
                WA_Top,                 top,
                WA_Width,               width,
                WA_Height,              height,
                //WA_ReportMouse,         TRUE,
                WA_CustomScreen,        scr,
                //WA_IDCMP,               IDCMP_CLOSEWINDOW,	// Kommer aldrig att inträffa - anv. för resize
                WA_SmartRefresh,        TRUE,
                WA_BackFill,            LAYERS_NOBACKFILL,
                TAG_DONE);

	if(pw->Wnd) {
		pw->RPort = pw->Wnd->RPort;

		/* Transparency/background image */
		PM_TransparencyBfr(pw);

		/* Transition effects */
		PM_OffScreenBfr(pw);

		return TRUE;
	} else {
        	DisplayBeep(NULL);
		return FALSE;
	}
}

//
// Close a window
//
void PM_CloseWindow(struct PM_Window *bw)
{
	if(bw->bg.BgArray) PM_Mem_Free(bw->bg.BgArray);
#ifdef __AROS__
	if(bw->te.RPort) FreeRastPort(bw->te.RPort);
#else	
	if(bw->te.RPort) PM_Mem_Free(bw->te.RPort);
#endif
	if(bw->te.BMap) FreeBitMap(bw->te.BMap);
	if(bw->Wnd) CloseWindow(bw->Wnd);

	bw->bg.BgArray = NULL;
	bw->te.RPort = NULL;
	bw->te.BMap = NULL;
	bw->Wnd = NULL;
}

//
// Resize a window
//
void PM_ResizeWindow(struct PM_Window *bw, int l, int t, int w, int h)
{
	struct Message *msg;

	if(l==bw->Wnd->LeftEdge && t==bw->Wnd->TopEdge && w==bw->Width && h==bw->Height)
		return;		// If no change

	ModifyIDCMP(bw->Wnd, IDCMP_CLOSEWINDOW|IDCMP_NEWSIZE);
	ChangeWindowBox(bw->Wnd, l, t, w, h);
	WaitPort(bw->Wnd->UserPort);
	while((msg=GetMsg(bw->Wnd->UserPort))) ReplyMsg(msg);
	ModifyIDCMP(bw->Wnd, IDCMP_CLOSEWINDOW);

  //     	bw->Width=bw->wnd->Width;
  //     	bw->Height=bw->wnd->Height;
}

//
// Find out if we should close our submenu
//

//
// PM_InsideWindows(px, py, wnd)
//
// px, py - Screen coords
// wnd    - PM_Window
//

BOOL PM_InsideWindows(int px, int py, struct PM_Window *wnd)
{
        int x=px;
        int y=py;
        struct PM_Window *w;

        w = wnd->Prev;

	if(w) {
		if(w->Selected) {
			if(px > w->Wnd->LeftEdge + w->Selected->Left - 2 &&
			   px < w->Wnd->LeftEdge + w->Selected->Left + w->Selected->Width + 2 &&
			   py > w->Wnd->TopEdge + w->Selected->Top - 2 &&
			   py < w->Wnd->TopEdge + w->Selected->Top + w->Selected->Height + 2)
				return FALSE;
		}
	}

        while(w) {
                if(x > w->Wnd->LeftEdge &&
		   y >= w->Wnd->TopEdge &&
		   x < w->Wnd->LeftEdge + w->Width &&
		   y < w->Wnd->TopEdge + w->Height) {
			return TRUE;
		}
                w=w->Prev;
        }

        return FALSE;
}
