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
#include <exec/types.h>

#include <devices/trackdisk.h>

#include <proto/exec.h>
#include <graphics/gfx.h>
#include <utility/tagitem.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <hidd/hidd.h>
#include <hidd/serial.h>

#include "../speaker.h"
#include "msdos_di.h"
 
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
        struct GfxBase *GfxBase;
        struct Window * win;
        int x = 100;
        int y = 100;


	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
        GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	if (IntuitionBase)
	{
	    struct TagItem tags[] = {
		{WA_Width,			320},
		{WA_Height,			240},
		{WA_Left,			100},
		{WA_Top,			100},
		{WA_MinWidth,                   320},
		{WA_MinHeight,                  240},
		{WA_MaxWidth,                   640},
		{WA_MaxHeight,                  480},
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

#if !AROS_BOCHS_HACK

	if(win)
	{
        	struct DriveGeometry MyDG;
        	struct IOExtTD *MyIO;
        	struct MsgPort *trackport;
        	struct fat_boot_sector *fb;
        	UBYTE *buf;

        	trackport=CreateMsgPort();

        	MyIO = (struct IOExtTD *)CreateExtIO(trackport, sizeof(struct IOExtTD));

        	if (OpenDevice("trackdisk.device", 0, (struct IORequest*)MyIO,0))
        	{
			BlackPrint(&win->WScreen->RastPort, "Can't open trackdisk.device unit 0 !", 40);
        	}
        	else
        	{
			char TempString[60];

                	buf = AllocVec (2 * 512, MEMF_CHIP);

                	MyIO->iotd_Req.io_Command=TD_GETGEOMETRY;
                	MyIO->iotd_Req.io_Data= &MyDG;
                	MyIO->iotd_Req.io_Length=sizeof(struct DriveGeometry);
                	DoIO((struct IORequest *)MyIO);

//                	kprintf("SectorSize=%ld,\nTotalSectors=%ld,\nCylinders=%ld,\nCylSectors=%ld,\nHeads=%ld,\nTrackSectors=%ld,\nBufMemType=%ld,\nDeviceType=%ld,\n",\
//                                MyDG.dg_SectorSize,MyDG.dg_TotalSectors,MyDG.dg_Cylinders,\
//                                MyDG.dg_CylSectors,MyDG.dg_Heads,MyDG.dg_TrackSectors,MyDG.dg_BufMemType,MyDG.dg_DeviceType);

                	MyIO->iotd_Req.io_Command=CMD_READ;
                	MyIO->iotd_Req.io_Offset=0;
                	MyIO->iotd_Req.io_Data= buf;
                	MyIO->iotd_Req.io_Length=512;
                	DoIO((struct IORequest *)MyIO);

                	fb = (struct fat_boot_sector *)buf;

                	sprintf(TempString, "system_id  : %s", fb->system_id);
                        BlackPrint(&win->WScreen->RastPort, TempString, 40);
                 	sprintf(TempString, "fats       : %d", fb->fats);
                        BlackPrint(&win->WScreen->RastPort, TempString, 50);
                	sprintf(TempString, "dir_entries: %d", fb->dir_entries[1]);
                        BlackPrint(&win->WScreen->RastPort, TempString, 60);
			sprintf(TempString, "sectors    : %d", fb->sectors[1]);
                        BlackPrint(&win->WScreen->RastPort, TempString, 70);
                	sprintf(TempString, "fat_length : %d", fb->fat_length);
                        BlackPrint(&win->WScreen->RastPort, TempString, 80);
                	sprintf(TempString, "secs_track : %d", fb->secs_track);
                        BlackPrint(&win->WScreen->RastPort, TempString, 90);
                	sprintf(TempString, "total_sect : %d", fb->total_sect);
                        BlackPrint(&win->WScreen->RastPort, TempString, 100);

                	FreeVec(buf);
        	}
        	CloseDevice((struct IORequest *)MyIO);
        	DeleteExtIO((struct IORequest *)MyIO);
        	DeleteMsgPort(trackport);
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
		BlackPrint(win2->RPort, ScreenInfo, 40);

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
			  
			  BlackPrint(win2->RPort, s, 60);
			  
			  if (msg->Code == 0x45) quitme = TRUE;
			}
			break;
			
		      case IDCMP_MOUSEMOVE:
		        {
			  WORD mx = win2->WScreen->MouseX;
			  WORD my = win2->WScreen->MouseY;

			  char s[20];

			  sprintf(s, "Mouse: %4ld, %4ld", mx, my);

			  WhitePrint(win2->RPort, s, 80);
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

void BlackPrint(struct RastPort *RPort, char *String, UWORD height)
{
        SetAPen(RPort, 2);
        SetBPen(RPort, 1);
        SetDrMd(RPort, JAM2);
        Move(RPort,20,height);
        Text(RPort, String, strlen(String));
}

void WhitePrint(struct RastPort *RPort, char *String, UWORD height)
{
        SetAPen(RPort, 1);
        SetBPen(RPort, 2);
        SetDrMd(RPort, JAM2);
        Move(RPort,20,height);
        Text(RPort, String, strlen(String));
}
