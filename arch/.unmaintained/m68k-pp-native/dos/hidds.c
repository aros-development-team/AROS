/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Begining of AROS kernel
    Lang: English
*/

#define AROS_USE_OOP

#define  DEBUG 1
#include <aros/debug.h>

#include <intuition/intuition.h>

#include <exec/memory.h>
#include <exec/types.h>

#include <devices/trackdisk.h>

#include <proto/exec.h>
#include <graphics/gfx.h>
#include <utility/tagitem.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <hidd/hidd.h>
#include <hidd/serial.h>

#include <stdio.h>
#include <string.h>

#include <clib/arossupport_protos.h>

#define ioStd(x) ((struct IOStdReq *)x)

static void BlackPrint(struct RastPort *RPort, char *String, UWORD height, struct GfxBase * GfxBase);
static void WhitePrint(struct RastPort *RPort, char *String, UWORD height, struct GfxBase * GfxBase);


void hidd_demo(struct ExecBase * SysBase)
{
    D(bug("graphics.hidd = %08.8lx\n",OpenLibrary("graphics.hidd",0)));
    D(bug("display.hidd = %08.8lx\n",OpenLibrary("display.hidd",0)));

    OpenLibrary("hidd.gfx.display",0);
    {
	struct GfxBase *GfxBase;
	BOOL success = FALSE;
    
        D(bug("init_gfx(hiddbase=%s)\n", "hidd.gfx.display"));

        GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
        if (GfxBase)
        {

	    /*  Call private gfx.library call to init the HIDD.
	        Gfx library is responsable for closing the HIDD
	        library (although it will probably not be neccesary).
	    */

	    D(bug("calling private gfx LateGfxInit()\n"));
	    if (LateGfxInit("hidd.gfx.display"))
	    {
	        struct IntuitionBase *IntuitionBase;

    	    	D(bug("lategfxinit okay\n"));
 
	        /* Now that gfx. is guaranteed to be up & working, let intuition open WB screen */
	        IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
		D(bug("ibase = %lx\n", IntuitionBase));
	        if (IntuitionBase)
	        {
	    	    if (LateIntuiInit(NULL))
	    	    {
			success = TRUE;
		    }
		    CloseLibrary((struct Library *)IntuitionBase);
		}
	    }
	    D(bug("Closing gfx\n"));
	
	    CloseLibrary((struct Library *)GfxBase);

	    if (success == FALSE)
	    {
	    	D(bug("There is something wrong with hidd subsystem..."));
		while(1) {};
	    }
	
	}
    }

#if 0
    {
	struct IORequest *io;
	struct MsgPort *mp;

	mp=CreateMsgPort();
	io=CreateIORequest(mp,sizeof(struct IOStdReq));
	D(bug("Result of opening device %d\n",
	    OpenDevice("gameport.device",0,io,0)));
	D(bug("Doing CMD_HIDDINIT...\n"));
	{
	    UBYTE *data;
	    data = AllocMem(100, MEMF_PUBLIC);
	    strcpy(data, "hidd.bus.mouse");
	    ioStd(io)->io_Command=32000;
	    ioStd(io)->io_Data=data;
	    ioStd(io)->io_Length=strlen(data);
	    DoIO(io);
	    D(bug("Got io_ERROR=%d",io->io_Error));
	}
    }
#endif
    {
        struct IntuitionBase *IntuitionBase;
        struct GfxBase *GfxBase;
        struct Window * win = NULL;
#define XSTART 50
#define YSTART 50
        int x = XSTART;
        int y = YSTART;


	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
        GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	if (IntuitionBase)
	{

	    struct TagItem tags[] = {
		{WA_Width,			100},
		{WA_Height,			100},
		{WA_Left,			x},
		{WA_Top,			y},
		{WA_MinWidth,                   100},
		{WA_MinHeight,                  100},
		{WA_MaxWidth,                   120},
		{WA_MaxHeight,                  120},
		{WA_Title,  (ULONG)"AROS Dream :-)"},
		{WA_Activate,			  1},
		{WA_SizeGadget,                TRUE},
		{WA_DepthGadget,               TRUE},
		{TAG_DONE,			  0}};
	    win = OpenWindowTagList(0, tags);
	}

        DrawEllipse(win->RPort,160/2-35,120/2-4,80/2,80/2);
        DrawEllipse(win->RPort,185/2-35,90/2-4,15/2,15/2);
        DrawEllipse(win->RPort,135/2-35,90/2-4,15/2,15/2);
        
        Move(win->RPort,125/2-35,140/2-4);
        Draw(win->RPort,140/2-35,150/2-4);
        Draw(win->RPort,180/2-35,150/2-4);
        Draw(win->RPort,195/2-35,140/2-4);

#if 0
	if (win)
	{
	  while (x < 100)
	  {
	    MoveWindow(win,1,0);
	    x++;
	  }
	  while (y < 100)
	  {
	    MoveWindow(win,0,1);
	    y++;
	  }
	  while (x > XSTART)
	  {
	    MoveWindow(win,-1,0);
	    x--;
	  }
	  while (y > YSTART)
	  {
	    MoveWindow(win,0,-1);
	    y--;
	  }
	}
#endif
#if 0
	if (IntuitionBase)
	{
	  struct Screen	 *screen;
	  struct Window	 *win2;
	  struct IntuiMessage *msg;
	  char ScreenInfo[40];

	  if ((screen = LockPubScreen(NULL)))
	  {
	      struct TagItem tags[] = {
		{WA_Width,			640},
		{WA_Height,			100},
		{WA_Left,			  0},
		{WA_Top,			 79},
		{WA_MinWidth,                   200},
		{WA_MinHeight,                  100},
		{WA_MaxWidth,                   640},
		{WA_MaxHeight,                  480},
		{WA_Title,  (ULONG)"AROS !ext"     },
		{WA_Activate,			  1},
		{WA_CloseGadget,               TRUE},
		{WA_SizeGadget,                TRUE},
		{WA_DepthGadget,               TRUE},
		{WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MOUSEMOVE | IDCMP_RAWKEY},
		{WA_ReportMouse,	       TRUE},
		{TAG_DONE,			 0}};
	      win2 = OpenWindowTagList(0, tags);

	      sprintf(ScreenInfo,"ScreenWidth: %d, ScreenHeight: %d", screen->Width, screen->Height);

	      if (win2)
	      {
		BlackPrint(win2->RPort, ScreenInfo, 40, GfxBase);

		for(;;)
		{
		  BOOL quitme = FALSE;
		  
		  WaitPort(win2->UserPort);
		  while((msg = ((struct IntuiMessage *)GetMsg(win2->UserPort))))
		  {
		    switch(msg->Class)
		    {
		      case IDCMP_RAWKEY:
		        {
			  static char hex[] = "0123456789ABCDEF";
			  char s[9];
			  
			  s[0] = 'K';
			  s[1] = 'e';
			  s[2] = 'y';
			  s[3] = ' ';
			  s[4] = hex[(msg->Code >> 12) & 0xF];
			  s[5] = hex[(msg->Code >> 8) & 0xF];
			  s[6] = hex[(msg->Code >> 4) & 0xF];
			  s[7] = hex[(msg->Code >> 0) & 0xF];
			  s[8] = '\0';
			  
			  BlackPrint(win2->RPort, s, 60, GfxBase);
			  
			  if (msg->Code == 0x45) quitme = TRUE;
			}
			break;
			
		      case IDCMP_MOUSEMOVE:
		        {
			  WORD mx = win2->WScreen->MouseX;
			  WORD my = win2->WScreen->MouseY;

			  char s[20];

			  sprintf(s, "Mouse: %4d, %4d", mx, my);

			  WhitePrint(win2->RPort, s, 80, GfxBase);
#if 0
			  mx &= 511;
			  my &= 255;
			  
			  SetAPen(&win2->WScreen->RastPort, 1);
			  SetDrMd(&win2->WScreen->RastPort, JAM2);
			  WritePixel(&win2->WScreen->RastPort, mx, my);
#endif
			}
			break;
			
		      case IDCMP_CLOSEWINDOW:
		        {
			  quitme = TRUE;
			}
			break;
					      
		    }
		    ReplyMsg((struct Message *)msg);
		  }
		  
		  if (quitme) break;
		}
		CloseWindow(win2);
	    }
	    UnlockPubScreen(NULL,screen);
	  }
	}
#endif
    }
}

static void BlackPrint(struct RastPort *RPort, char *String, UWORD height, struct GfxBase * GfxBase)
{
        SetAPen(RPort, 2);
        SetBPen(RPort, 1);
        SetDrMd(RPort, JAM2);
        Move(RPort,20,height);
        Text(RPort, String, strlen(String));
}

static void WhitePrint(struct RastPort *RPort, char *String, UWORD height, struct GfxBase * GfxBase)
{
        SetAPen(RPort, 1);
        SetBPen(RPort, 2);
        SetDrMd(RPort, JAM2);
        Move(RPort,20,height);
        Text(RPort, String, strlen(String));
}
