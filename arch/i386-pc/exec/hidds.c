/*
    (C) 1997-1999 AROS - The Amiga Research OS
    $Id$

    Desc: Begining of AROS kernel
    Lang: English
*/

#define AROS_USE_OOP
#define AROS_ALMOST_COMPATIBLE 1

#include <intuition/intuition.h>

#include <exec/memory.h>

#include <proto/exec.h>
#include <graphics/gfx.h>
#include <utility/tagitem.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <hidd/hidd.h>
#include <hidd/serial.h>

#define ioStd(x) ((struct IOStdReq *)x)

void hidd_demo()
{
    kprintf("graphics.hidd = %08.8lx\n",OpenLibrary("graphics.hidd",0));
    kprintf("vga.hidd = %08.8lx\n",OpenLibrary("vga.hidd",0));
    
    {
	struct GfxBase *GfxBase;
	BOOL success = FALSE;
    
        kprintf("init_gfx(hiddbase=%s)\n", "hidd.gfx.vga");
    
        GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
        if (GfxBase)
        {

	    /*  Call private gfx.library call to init the HIDD.
	        Gfx library is responsable for closing the HIDD
	        library (although it will probably not be neccesary).
	    */

	    kprintf("calling private gfx LateGfxInit()\n");
	    if (LateGfxInit("hidd.gfx.vga"))
	    {
	        struct IntuitionBase *IntuitionBase;
	        kprintf("success\n");
			    
	        /* Now that gfx. is guaranteed to be up & working, let intuition open WB screen */
	        IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
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
	    	kprintf("There is something wrong with hidd subsystem...");
		while(1) {};
	    }
	
	}
    }

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
	    strcpy(data, "hidd.mouse.hw");
	    ioStd(io)->io_Command=32000;
	    ioStd(io)->io_Data=data;
	    ioStd(io)->io_Length=strlen(data);
	    DoIO(io);
	    kprintf("Got io_ERROR=%d",io->io_Error);
	}
    }

    {
        struct IntuitionBase *IntuitionBase;
        struct Window * win;
        int x = 100;
        int y = 100;


	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
	if (IntuitionBase)
	{
	    struct TagItem tags[] = {
		{WA_Width,			320},
		{WA_Height,			240},
		{WA_Left,			100},
		{WA_Top,			100},
		{WA_Title,  (ULONG)"AROS Dream :-)"},
		{WA_Activate,			  1},
		{WA_SizeGadget,                TRUE},
		{WA_DepthGadget,               TRUE},
		{TAG_DONE,			  0}};
	    win = OpenWindowTagList(0, tags);
	}

        DrawEllipse(win->RPort,160,120,80,80);
        DrawEllipse(win->RPort,185,90,15,15);
        DrawEllipse(win->RPort,135,90,15,15);
        
        Move(win->RPort,125,140);
        Draw(win->RPort,140,150);
        Draw(win->RPort,180,150);
        Draw(win->RPort,195,140);
        	
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

	if (IntuitionBase)
	{
	  struct Screen	 *screen;
	  struct DrawInfo  *drawinfo;
	  struct Window	 *win2;
	  struct IntuiMessage *msg;
	  struct IntuiText  myIText;
	  struct TextAttr   myTextAttr;
	  char MyText[512];

	  ULONG myTEXTPEN;
	  ULONG myBACKGROUNDPEN;

	  if ((screen = LockPubScreen(NULL)))
	  {
	    if ((drawinfo = GetScreenDrawInfo(screen)))
	    {
	      struct TagItem tags[] = {
		{WA_Width,			640},
		{WA_Height,			100},
		{WA_Left,			  0},
		{WA_Top,			379},
		{WA_Title,  (ULONG)"AROS !ext"     },
		{WA_Activate,			  1},
		{WA_SizeGadget,                TRUE},
		{WA_DepthGadget,               TRUE},
		{WA_IDCMP, IDCMP_MOUSEMOVE | IDCMP_RAWKEY},
		{WA_ReportMouse,	       TRUE},
		{TAG_DONE,			 0}};
	      win2 = OpenWindowTagList(0, tags);

	      myTEXTPEN = drawinfo->dri_Pens[TEXTPEN];
	      myBACKGROUNDPEN = drawinfo->dri_Pens[BACKGROUNDPEN];

	      myTextAttr.ta_Name  = drawinfo->dri_Font->tf_Message.mn_Node.ln_Name;
	      myTextAttr.ta_YSize = drawinfo->dri_Font->tf_YSize;
	      myTextAttr.ta_Style = drawinfo->dri_Font->tf_Style;
	      myTextAttr.ta_Flags = drawinfo->dri_Font->tf_Flags;

	      sprintf(MyText,"ScreenWidth: %d, ScreenHeight: %d", screen->Width, screen->Height);

	      if (win2)
	      {
		myIText.FrontPen    = myTEXTPEN;
		myIText.BackPen     = myBACKGROUNDPEN;
		myIText.DrawMode    = JAM2;
		myIText.LeftEdge    = 0;
		myIText.TopEdge     = 0;
		myIText.ITextFont   = &myTextAttr;
		myIText.IText	    = MyText;
		myIText.NextText    = NULL;

		PrintIText(win2->RPort,&myIText,10,30);

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
			  char s[8];
			  
			  s[0] = 'K';
			  s[1] = 'e';
			  s[2] = 'y';
			  s[3] = ' ';
			  s[4] = hex[(msg->Code >> 12) & 0xF];
			  s[5] = hex[(msg->Code >> 8) & 0xF];
			  s[6] = hex[(msg->Code >> 4) & 0xF];
			  s[7] = hex[(msg->Code >> 0) & 0xF];
			  
			  Move(win2->RPort, 20, 60);
			  SetAPen(win2->RPort, 2);
			  SetBPen(win2->RPort, 1);
			  SetDrMd(win2->RPort, JAM2);
			  Text(win2->RPort, s, 8);
			  
			  if (msg->Code == 0x45) quitme = TRUE;
			}
			break;
			
		      case IDCMP_MOUSEMOVE:
		        {
			  WORD mx = win2->WScreen->MouseX;
			  WORD my = win2->WScreen->MouseY;

			  static char hex[] = "0123456789ABCDEF";
			  char s[15];
			  
			  s[0] = 'M';
			  s[1] = 'o';
			  s[2] = 'u';
			  s[3] = 's';
			  s[4] = 'e';
			  s[5] = ' ';
			  s[6] = hex[(mx >> 12) & 0xF];
			  s[7] = hex[(mx >> 8) & 0xF];
			  s[8] = hex[(mx >> 4) & 0xF];
			  s[9] = hex[(mx >> 0) & 0xF];
			  s[10] = ',';
			  s[11] = hex[(my >> 12) & 0xF];
			  s[12] = hex[(my >> 8) & 0xF];
			  s[13] = hex[(my >> 4) & 0xF];
			  s[14] = hex[(my >> 0) & 0xF];
			  
			  Move(win2->RPort, 20, 80);
			  SetAPen(win2->RPort, 1);
			  SetBPen(win2->RPort, 2);
			  SetDrMd(win2->RPort, JAM2);
			  Text(win2->RPort, s, 15);
			  
			  mx &= 511;
			  my &= 255;
			  
			  SetAPen(&win2->WScreen->RastPort, 1);
			  SetDrMd(&win2->WScreen->RastPort, JAM2);
			  WritePixel(&win2->WScreen->RastPort, mx, my);
			}
			break;
		      
		    }
		    ReplyMsg((struct Message *)msg);
		  }
		  
		  if (quitme) break;
		}
		CloseWindow(win2);
	      }
	      FreeScreenDrawInfo(screen,drawinfo);
	    }
	    UnlockPubScreen(NULL,screen);
	  }
	}
    }
}



