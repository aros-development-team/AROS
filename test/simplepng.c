#include <cybergraphx/cybergraphics.h>
#include <graphics/gfxmacros.h>

#include <proto/png.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdio.h>

APTR handle;

struct Library *PNGBase;
struct Window *win;

LONG width, height, depth, type;
APTR paldata, gfxdata;

static void showimage(void)
{
    win = OpenWindowTags(0, WA_Title, (IPTR)"SimplePNG",
    	    	    	    WA_InnerWidth, width + 4,
			    WA_InnerHeight, height + 4,
			    WA_CloseGadget, TRUE,
			    WA_DragBar, TRUE,
			    WA_DepthGadget, TRUE,
			    WA_Activate, TRUE,
			    WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_VANILLAKEY,
			    TAG_DONE);
    if (win)
    {
    	if (GetBitMapAttr(win->WScreen->RastPort.BitMap, BMA_DEPTH) >= 15)
	{
	    BOOL quitme = FALSE;
	    
	    {
	    	static UWORD pat[] =
		{
		    0xFF00,
		    0xFF00,
		    0xFF00,
		    0xFF00,
		    0xFF00,
		    0xFF00,
		    0xFF00,
		    0xFF00,
    	    	    0x00FF,
    	    	    0x00FF,
    	    	    0x00FF,
    	    	    0x00FF,
    	    	    0x00FF,
    	    	    0x00FF,
    	    	    0x00FF,
    	    	    0x00FF		    		    
		};
	    	WORD pen1 = ObtainBestPen(win->WScreen->ViewPort.ColorMap,
		    	    	    	  0x6b6b6b6b, 
					  0x6b6b6b6b, 
					  0x6b6b6b6b, 
					  OBP_FailIfBad, FALSE);
	    	WORD pen2 = ObtainBestPen(win->WScreen->ViewPort.ColorMap,
		    	    	    	  0x9c9c9c9c, 
					  0x9c9c9c9c, 
					  0x9c9c9c9c, 
					  OBP_FailIfBad, FALSE);
					  
	    	SetAfPt(win->RPort, pat, 4);
		SetABPenDrMd(win->RPort, pen1, pen2, JAM2);
		RectFill(win->RPort, win->BorderLeft,
		    	    	     win->BorderTop,
				     win->Width - 1 - win->BorderRight,
				     win->Height - 1 - win->BorderBottom);				     
	    	SetAfPt(win->RPort, NULL, 0);
		
		if (pen1 != -1) ReleasePen(win->WScreen->ViewPort.ColorMap, pen1);
		if (pen2 != -1) ReleasePen(win->WScreen->ViewPort.ColorMap, pen2);
	    }
	    
	    WritePixelArrayAlpha(gfxdata,
	    	    	    	 0,
				 0,
				 width * depth / 8,
				 win->RPort,
				 win->BorderLeft + 2,
				 win->BorderTop + 2,
				 width,
				 height,
				 0);
	    while(!quitme)
	    {
	    	struct IntuiMessage *msg;
		
	    	WaitPort(win->UserPort);
		while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
		{
		    switch(msg->Class)
		    {
		    	case IDCMP_CLOSEWINDOW:
			    quitme = TRUE;
			    break;
			    
			case IDCMP_VANILLAKEY:
			    if (msg->Code == 27) quitme = TRUE;
			    break;
			    
		    }
		    ReplyMsg((struct Message *)msg);
		}
	    }
	}
    	CloseWindow(win);
    }
}

int main(int argc, char **argv)
{
    PNGBase = OpenLibrary("datatypes/png.datatype", 0);
    if (!PNGBase)
    {
    	printf("Failed to open png.datatype!\n");
	return 0;
    }
    
    if (argc < 2)
    {
    	printf("Usage: simplepng <filename>\n");
	return 0;
    }
    
    if ((handle = PNG_LoadImage(argv[1], TRUE)))
    {	
    	printf("PNG_LoadImage ok\n");
	
	PNG_GetImageInfo(handle, &width, &height, &depth, &type);
	PNG_GetImageData(handle, &gfxdata, &paldata);
	
	printf("Width %ld    Height %ld    Depth %ld    Type    %ld\n",
	    	width, height, depth, type);
	printf("GfxData %p  PalData %p\n", gfxdata, paldata);
	
	if (gfxdata) showimage();
	
	PNG_FreeImage(handle);
    }
    
    CloseLibrary(PNGBase);
    
    return 0;
}
