#include "graros.h"

#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>

#include <stdio.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *CyberGfxBase;

  typedef struct grArosSurface_
  {
    grSurface      	root;
    grBitmap       	image; 
    struct Window  	*win;
    LONG		bpr;
    BOOL		grays;
    ULONG		cgfxcoltab[256];
  } grArosSurface;

  
  /* close a given window */
  static
  void  done_surface( grArosSurface*  surface )
  {
     if (surface->win) CloseWindow(surface->win);
     grDoneBitmap(&surface->root.bitmap);
  }



  /* close the device, i.e. the display connection */
  static
  void  done_device( void )
  {
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (CyberGfxBase) CloseLibrary(CyberGfxBase);
  }


  static
  int  init_device( void )
  {
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39);    
    if (!IntuitionBase)
    {
      puts("Could not open intuition.library V39!");
      return -1;
    }
    puts("Intuition.library opened.");
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",39);
    if (!GfxBase)
    {
      puts("Could not open graphics.library V39!");          
      return -1;
    }
    puts("Graphics.library opened.");
    
    CyberGfxBase = OpenLibrary("cybergraphics.library",0);
    if (!CyberGfxBase)
    {
      puts("Could not open cybergraphics.library!");
      return -1;
    }
    puts("Cybergraphics.library opened.");
    
    return 0;
  }


  static
  void  refresh_rectangle( grArosSurface*  surface,
                           int          x,
                           int          y,
                           int          w,
                           int          h )
  {
     SetAPen(surface->win->RPort, 1);
     SetDrMd(surface->win->RPort, JAM2);

     if (!surface->grays)
     {
	BltTemplate(surface->root.bitmap.buffer,
     		    0,
		    w / 8,
		    surface->win->RPort,
		    surface->win->BorderLeft,
		    surface->win->BorderTop,
		    surface->bpr * 8,
		    surface->root.bitmap.rows);
     } else {
#if 0
       WritePixelArray8(surface->win->RPort,
     			surface->win->BorderLeft,
			surface->win->BorderTop,
			surface->win->BorderLeft + surface->root.bitmap.width - 1,
			surface->win->BorderTop + surface->root.bitmap.rows - 1,
			surface->root.bitmap.buffer,
			0); 
#endif
       WriteLUTPixelArray(surface->root.bitmap.buffer,
			  0,
			  0,
			  surface->root.bitmap.width,
			  surface->win->RPort,
			  surface->cgfxcoltab,
			  surface->win->BorderLeft,
			  surface->win->BorderTop,
			  surface->root.bitmap.width,
			  surface->root.bitmap.rows,
			  CTABFMT_XRGB8);

     }
  }

  
  static
  void  set_title( grArosSurface*  surface,
                   const char*  title )
  {
    if (surface->win)
    {
      SetWindowTitles(surface->win, (STRPTR)title, (STRPTR)~0);
    }
  }

  static LONG rawkey_to_grkey[][2] =
  {
      { 0x41, 	  grKeyBackSpace },
      { 0x42,     grKeyTab       },
      { 0x44,     grKeyReturn    },
      { 0x45,     grKeyEsc       },
      { 0x70,     grKeyHome      },
      { 0x4F,     grKeyLeft      },
      { 0x4C,     grKeyUp        },
      { 0x4E,     grKeyRight     },
      { 0x4D,     grKeyDown      },
      { 0x48,     grKeyPageUp    },
      { 0x49, 	  grKeyPageDown  },
      { 0x71,     grKeyEnd       },
      { 0x70,     grKeyHome      },
      { 0x50,     grKeyF1        },
      { 0x51,     grKeyF2        },
      { 0x52,     grKeyF3        },
      { 0x53,     grKeyF4        },
      { 0x54,     grKeyF5        },
      { 0x55,     grKeyF6        },
      { 0x56,     grKeyF7        },
      { 0x57,     grKeyF8        },
      { 0x58,     grKeyF9        },
      { 0x59,     grKeyF10       },
      { 0x4B,     grKeyF11       },
      { 0x6F,     grKeyF12       },
      {   -1,	  -1	         }
  };

  static  
  void  listen_event( grArosSurface*  surface,
                      int          event_mask,
                      grEvent*     grevent )
  {
    struct IntuiMessage *msg;
    WORD code, i;
    
    (void)event_mask;
    
    WaitPort(surface->win->UserPort);
    
    if ((msg = (struct IntuiMessage *)GetMsg(surface->win->UserPort)))
    {
      switch(msg->Class)
      {
        case IDCMP_RAWKEY:
	  for(i = 0; (code = rawkey_to_grkey[i][0]) != -1; i++)
	  {
	    if (code == msg->Code)
	    {
	      grevent->type = gr_key_down;
	      grevent->key  = rawkey_to_grkey[i][1];
	      break;
	    }
	  }
	  break;
	  
        case IDCMP_VANILLAKEY:
	  grevent->type = gr_key_down;
	  grevent->key   = msg->Code;
	  break;
      }
      ReplyMsg((struct Message *)msg);
    }
  }




  grArosSurface*  init_surface( grArosSurface*  surface,
                             grBitmap*    bitmap )
  {
    printf("init_surface: width = %d  height = %d\n",
    		bitmap->width,
		bitmap->rows);
    
    bitmap->width = (bitmap->width + 15) & ~15;
    surface->bpr = bitmap->width / 8;
    
    if (grNewBitmap(bitmap->mode,
    		    bitmap->grays,
		    bitmap->width,
		    bitmap->rows,
		    bitmap)) return 0;
		
    surface->win = OpenWindowTags(0, WA_Left, 20,
    				     WA_Top, 20,
				     WA_InnerWidth, bitmap->width,
				     WA_InnerHeight, bitmap->rows,
				     WA_AutoAdjust, TRUE,
				     WA_Title, "Freetype demo",
				     WA_CloseGadget, TRUE,
				     WA_DepthGadget, TRUE,
				     WA_Activate, TRUE,
				     WA_IDCMP, IDCMP_VANILLAKEY | IDCMP_RAWKEY,
				     TAG_DONE);

    if (!surface->win)
    {
      puts("Could not open window!");
      grDoneBitmap(bitmap);
      return 0;
    }					    

    puts("Window opened.");

    surface->grays = ( bitmap->mode == gr_pixel_mode_gray &&
                       bitmap->grays >= 2 );

    if (surface->grays)
    {
      WORD i;
      
      if (GetBitMapAttr(surface->win->RPort->BitMap, BMA_DEPTH <= 8))
      {
        puts("Need hi/true color screen!");
	CloseWindow(surface->win);
	grDoneBitmap(bitmap);
	return 0;
      }
      
      for(i = 0; i < bitmap->grays; i++)
      {
        LONG red, green, blue;
	
	red = green = blue = (bitmap->grays - i) * 255 / bitmap->grays;
	
        surface->cgfxcoltab[i] = (red << 16) | (green << 8) | blue;
      }
    }	          
    surface->root.bitmap       = *bitmap;
    surface->root.done         = (grDoneSurfaceFunc) done_surface;
    surface->root.refresh_rect = (grRefreshRectFunc) refresh_rectangle;
    surface->root.set_title    = (grSetTitleFunc)    set_title;
    surface->root.listen_event = (grListenEventFunc) listen_event;
    
    return surface;
  }
  



  grDevice  gr_aros_device =
  {
    sizeof( grArosSurface ),
    "aros",
    
    init_device,
    done_device,
    
    (grDeviceInitSurfaceFunc) init_surface,
    
    0,
    0
    
  };

