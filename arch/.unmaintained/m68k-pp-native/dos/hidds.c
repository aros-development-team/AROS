/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Begining of AROS kernel
    Lang: English
*/

#define AROS_USE_OOP
#define AROS_ALMOST_COMPATIBLE 1

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

#define ioStd(x) ((struct IOStdReq *)x)

void hidd_demo(struct ExecBase * SysBase)
{
//    kprintf("graphics.hidd = %08.8lx\n",OpenLibrary("graphics.hidd",0));
//    kprintf("display.hidd = %08.8lx\n",OpenLibrary("display.hidd",0));

    /* absolutely necessary!!!*/
    OpenLibrary("graphics.hidd",0); // runs through and seems to get init. ok.
// *(ULONG *)0x12347=1;
    OpenLibrary("display.hidd",0);  // seems not to get where it should.
// *(ULONG *)0x1234F=1;
    OpenLibrary("hidd.gfx.display",0);
    {
	struct GfxBase *GfxBase;
	BOOL success = FALSE;
    
//        kprintf("init_gfx(hiddbase=%s)\n", "hidd.gfx.display");

// *(ULONG *)0xc0debadf=0;
        GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
        if (GfxBase)
        {

	    /*  Call private gfx.library call to init the HIDD.
	        Gfx library is responsable for closing the HIDD
	        library (although it will probably not be neccesary).
	    */

	    kprintf("calling private gfx LateGfxInit()\n");
	    if (LateGfxInit("hidd.gfx.display"))
	    {
		
	        struct IntuitionBase *IntuitionBase;
// *(ULONG *)0xc0debad3 = 0;
//	        kprintf("success\n");
			    
// *(ULONG *)0x1235F=1;

    	    	kprintf("lategfxinit okay\n");
 
	        /* Now that gfx. is guaranteed to be up & working, let intuition open WB screen */
	        IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
kprintf("ibase = %lx\n", IntuitionBase);
	        if (IntuitionBase)
	        {
	    	    if (LateIntuiInit(NULL))
	    	    {
			success = TRUE;
		    }
		    CloseLibrary((struct Library *)IntuitionBase);
		}
	    }
	    kprintf("Closing gfx\n");
	
	    CloseLibrary((struct Library *)GfxBase);

	    if (success == FALSE)
	    {
//	    	kprintf("There is something wrong with hidd subsystem...");
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
	kprintf("Result of opening device %d\n",
	    OpenDevice("gameport.device",0,io,0));
	kprintf("Doing CMD_HIDDINIT...\n");
	{
	    UBYTE *data;
	    data = AllocMem(100, MEMF_PUBLIC);
	    strcpy(data, "hidd.bus.mouse");
	    ioStd(io)->io_Command=32000;
	    ioStd(io)->io_Data=data;
	    ioStd(io)->io_Length=strlen(data);
	    DoIO(io);
	    kprintf("Got io_ERROR=%d",io->io_Error);
	}
    }
#endif
    {
        struct IntuitionBase *IntuitionBase;
        struct GfxBase *GfxBase;
        struct Window * win;
        int x = 100;
        int y = 100;


	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
        GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	if (IntuitionBase)
	{

	    struct TagItem tags[] = {
		{WA_Width,			100},
		{WA_Height,			100},
		{WA_Left,			50},
		{WA_Top,			50},
		{WA_MinWidth,                   320},
		{WA_MinHeight,                  240},
		{WA_MaxWidth,                   640},
		{WA_MaxHeight,                  480},
		{WA_Title,  (ULONG)"AROS Dream :-)"},
		{WA_Activate,			  1},
		{WA_SizeGadget,                TRUE},
		{WA_DepthGadget,               TRUE},
		{TAG_DONE,			  0}};
kprintf("Opening window\n");
	    win = OpenWindowTagList(0, tags);
	}

kprintf("Window okay: win = %x\n", win);

        DrawEllipse(win->RPort,160,120,80,80);
        DrawEllipse(win->RPort,185,90,15,15);
        DrawEllipse(win->RPort,135,90,15,15);
        
        Move(win->RPort,125,140);
        Draw(win->RPort,140,150);
        Draw(win->RPort,180,150);
        Draw(win->RPort,195,140);

#if !AROS_BOCHS_HACK
	/* This is slow like hell under Bochs */
	        	
	if (win)
	{
	  while (x < 200)
	  {
	    MoveWindow(win,1,0);
	    x++;
	  }
	  
	  while (y < 200)
	  {
	    MoveWindow(win,0,1);
	    y++;
	  }
	  
	  while (x >= 100)
	  {
	    MoveWindow(win,-1,0);
	    x--;
	  }
	  
	  while (y >= 100)
	  {
	    MoveWindow(win,0,-1);
	    y--;
	  }
	}
#endif

#if 0
	if (IntuitionBase)
	{
	    ULONG i, dummy;

	    SetSpkFreq (400);
	    SpkOn();
	    for (i=0; i<100000000; dummy = i*i, i++);  
	    SpkOff();
	    for (i=0; i< 50000000; dummy = i*i, i++); 
	    
	    SetSpkFreq (500);
	    SpkOn();
	    for (i=0; i<100000000; dummy = i*i, i++);
	    SpkOff();
	    for (i=0; i< 50000000; dummy = i*i, i++);
	    
	    SetSpkFreq (592);
	    SpkOn();
	    for (i=0; i<100000000; dummy = i*i, i++); 
	    SpkOff();
	    for (i=0; i< 50000000; dummy = i*i, i++); 
	    
	    SetSpkFreq (788);
	    SpkOn();
	    for (i=0; i<300000000; dummy = i*i, i++);  
	    SpkOff();
	}
#endif

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
		{WA_Top,			379},
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

			  sprintf(s, "Mouse: %4ld, %4ld", mx, my);

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
    }
}

void BlackPrint(struct RastPort *RPort, char *String, UWORD height, struct GfxBase * GfxBase)
{
        SetAPen(RPort, 2);
        SetBPen(RPort, 1);
        SetDrMd(RPort, JAM2);
        Move(RPort,20,height);
        Text(RPort, String, strlen(String));
}

void WhitePrint(struct RastPort *RPort, char *String, UWORD height, struct GfxBase * GfxBase)
{
        SetAPen(RPort, 1);
        SetBPen(RPort, 2);
        SetDrMd(RPort, JAM2);
        Move(RPort,20,height);
        Text(RPort, String, strlen(String));
}
