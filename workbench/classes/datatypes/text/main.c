#include <datatypes/textclass.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#include "datatype.h"

struct Screen *scr;
struct Window *wnd;
APTR text;

int handle_wnd()
{
	int retval = 0;
	struct IntuiMessage *imsg;
	while((imsg = (struct IntuiMessage *)GetMsg(wnd->UserPort)))
	{
		ULONG cl;
		LONG mx,my;
		UWORD code;
		ULONG secs,mics;

		cl = imsg->Class;
		code = imsg->Code;
		mx = imsg->MouseX;
		my = imsg->MouseY;
		secs = imsg->Seconds;
		mics = imsg->Micros;

		ReplyMsg((struct Message*)imsg);

		switch(cl)
		{
			case	IDCMP_CLOSEWINDOW:
						retval = 1;
						break;

			case	IDCMP_VANILLAKEY:
						switch(code)
						{
							case	'p':
										Text_Print(text);
										break;
						}
						break;

			case	IDCMP_RAWKEY:
						switch(code)
						{
							case	CURSORDOWN:
										Text_SetVisibleTop(text, Text_VisibleTop(text)+1);
										break;

							case	CURSORUP:
										Text_SetVisibleTop(text, Text_VisibleTop(text)-1);
										break;

							case	CURSORRIGHT:
										Text_SetVisibleLeft(text, Text_VisibleHoriz(text)+1);
										break;

							case	CURSORLEFT:
										Text_SetVisibleLeft(text, Text_VisibleHoriz(text)-1);
										break;
						}
						break;

			case	IDCMP_MOUSEBUTTONS:
						if(code == SELECTDOWN)
							Text_HandleMouse(text, mx, my, SELECTDOWN, secs, mics);
						else
							Text_HandleMouse(text, mx, my, SELECTUP, 0, 0);
						break;

			case	IDCMP_MOUSEMOVE:
						Text_HandleMouse(text, mx, my, 0, 0, 0);
						break;
		}
	}
	return retval;
}

void loop(void)
{
	ULONG wnd_mask = 1L << wnd->UserPort->mp_SigBit;
	int ready = FALSE;

	while((ready == FALSE))
	{
		ULONG sigs = Wait(wnd_mask | 4096);
		if(sigs & 4096) ready = TRUE;
		if(sigs & wnd_mask) ready = handle_wnd();
	}
}

void main()
{
	if((scr = LockPubScreen("Workbench")))
	{
		wnd = OpenWindowTags( NULL,
					WA_Title, "Textview",
					WA_CloseGadget, TRUE,
					WA_DragBar, TRUE,
					WA_DepthGadget, TRUE,
					WA_Activate, TRUE,
					WA_InnerHeight, 400,
					WA_IDCMP, IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_MOUSEMOVE|IDCMP_MOUSEBUTTONS|IDCMP_VANILLAKEY,
					WA_SizeGadget, TRUE,
					WA_MinWidth, 100,
					WA_MinHeight, 100,
					WA_MaxWidth, -1,
					WA_MaxHeight, -1,
					WA_ReportMouse, TRUE,
					TAG_DONE);

		if(wnd)
		{
			if((text = Text_Create()))
			{
				Text_SetFrameBox(text,scr,wnd->RPort, (LONG)wnd->BorderLeft, (LONG)wnd->BorderTop, wnd->Width - wnd->BorderLeft - wnd->BorderRight - 1, wnd->Height - wnd->BorderTop - wnd->BorderBottom - 1);
				Text_Load(text,"DH0:");
				Text_Redraw(text);
				loop();
				Text_Free(text);
			}
			CloseWindow(wnd);
		}
		UnlockPubScreen(NULL, scr);
	}
}
