/*******************************************************************
 *
 *  grmac.c  graphics driver for MacOS platform.
 *
 *  This is the driver for displaying inside a window under MacOS,
 *  used by the graphics utility of the FreeType test suite.
 *
 *  Largely written by Just van Rossum, but derived from grwin32.c.
 *  Copyright 1999-2018 by
 *  Just van Rossum, Antoine Leca,
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  Borrowing liberally from the other FreeType drivers.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT. By continuing to use, modify or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

/* ANSI C */
#include <string.h>

/* Mac Toolbox */
#include <MacWindows.h>

/* FT graphics subsystem */
#include "grobjs.h"
#include "grdevice.h"

#ifdef __MWERKS__
/* CodeWarrior's poor excuse for a console */
#include <SIOUX.h>
#endif

/*  Mac function key definitions. The 0x100 is a kludge, see listen_event(). */
#define        KEY_F1        (0x7A | 0x100)
#define        KEY_F2        (0x78 | 0x100)
#define        KEY_F3        (0x63 | 0x100)
#define        KEY_F4        (0x76 | 0x100)
#define        KEY_F5        (0x60 | 0x100)
#define        KEY_F6        (0x61 | 0x100)
#define        KEY_F7        (0x62 | 0x100)
#define        KEY_F8        (0x64 | 0x100)
#define        KEY_F9        (0x65 | 0x100)
#define        KEY_F10       (0x6D | 0x100)
#define        KEY_F11       (0x67 | 0x100)
#define        KEY_F12       (0x6F | 0x100)
#define        KEY_F13       (0x69 | 0x100)
#define        KEY_F14       (0x6B | 0x100)
#define        KEY_F15       (0x71 | 0x100)


/* Mac to FT key mapping */

  typedef struct  Translator_
  {
    short   mackey;
    grKey   grkey;

  } Translator;

  static
  Translator  key_translators[] =
  {
    { kBackspaceCharCode,   grKeyBackSpace },
    { kTabCharCode,         grKeyTab       },
    { kReturnCharCode,      grKeyReturn    },
    { kEscapeCharCode,      grKeyEsc       },
    { kHomeCharCode,        grKeyHome      },
    { kLeftArrowCharCode,   grKeyLeft      },
    { kUpArrowCharCode,     grKeyUp        },
    { kRightArrowCharCode,  grKeyRight     },
    { kDownArrowCharCode,   grKeyDown      },
    { kPageUpCharCode,      grKeyPageUp    },
    { kPageDownCharCode,    grKeyPageDown  },
    { kEndCharCode,         grKeyEnd       },
    { kHelpCharCode,        grKeyF1        }, /* map Help key to F1... */
    { KEY_F1,               grKeyF1        },
    { KEY_F2,               grKeyF2        },
    { KEY_F3,               grKeyF3        },
    { KEY_F4,               grKeyF4        },
    { KEY_F5,               grKeyF5        },
    { KEY_F6,               grKeyF6        },
    { KEY_F7,               grKeyF7        },
    { KEY_F8,               grKeyF8        },
    { KEY_F9,               grKeyF9        },
    { KEY_F10,              grKeyF10       },
    { KEY_F11,              grKeyF11       },
    { KEY_F12,              grKeyF12       }
  };


  /* This is a minimalist driver, it is only able to display */
  /* a _single_ window. Moreover, only monochrome and gray   */
  /* bitmaps are supported..                                 */

  /* pointer to the window. */
  static WindowPtr   theWindow = NULL;

  /* the pixmap */
  static PixMap thePixMap;


  /* destroys the window */
  static
  void done_window(  )
  {
    if ( theWindow )
    {
      DisposeWindow ( theWindow );
    }
    theWindow = NULL;
  }

  /* destroys the surface*/
  static
  void done_surface( grSurface*  surface )
  {
    /* ick! this never gets called... */
    done_window();
    grDoneBitmap( &surface->bitmap );
  }

  static
  void  refresh_rectangle( grSurface*  surface,
                           int         x,
                           int         y,
                           int         w,
                           int         h )
  {
    Rect bounds;
    SetRect( &bounds, x, y, x+w, y+h );
    if ( theWindow ) {
      GrafPtr windowPort = GetWindowPort(theWindow);
      RgnHandle visRgn = GetPortVisibleRegion(windowPort, NewRgn());
      SetPort(windowPort);
      CopyBits( (BitMap*)&thePixMap, GetPortBitMapForCopyBits(windowPort),
                &bounds, &bounds, srcCopy, visRgn);
      DisposeRgn(visRgn);
  }
  }

  static
  void set_title( grSurface* surface, const char* title )
  {
    Str255 pTitle;
    strcpy( (char*)pTitle+1, title );
    pTitle[0] = strlen( title );
    if ( theWindow )
        SetWTitle( theWindow, pTitle );
  }

  static
  void listen_event( grSurface*  surface,
                     int         event_mask,
                     grEvent*    grevent )
  {
    grEvent  our_grevent;
    EventRecord mac_event;

    our_grevent.type = gr_event_none;
    our_grevent.key = grKeyNone;

    for ( ;; )
    /* The event loop. Sorry, but I'm too lazy to split the various events
       to proper event handler functions. This whole app is rather ad hoc
       anyway, so who cares ;-) */
    {
      if ( WaitNextEvent( everyEvent, &mac_event, 10, NULL ) )
      {
        switch ( mac_event.what )
        {
        case autoKey:
        case keyDown:
          {
            int           count = sizeof( key_translators ) / sizeof( key_translators[0] );
            Translator*   trans = key_translators;
            Translator*   limit = trans + count;
            short         char_code;

            char_code = mac_event.message & charCodeMask;
            if ( char_code == kFunctionKeyCharCode )
                /* Le kluge. Add a flag to differentiate the F-keys from normal keys. */
                char_code = 0x100 | ((mac_event.message & keyCodeMask) >> 8);

            our_grevent.key = char_code;

            for ( ; trans < limit; trans++ )
              /* see if the key maps to a special "gr" key */
              if ( char_code == trans->mackey )
              {
                our_grevent.key = trans->grkey;
              }
              our_grevent.type = gr_event_key;
            }
            if ( our_grevent.key == grKEY('q') || our_grevent.key == grKeyEsc )
              /* destroy the window here, since done_surface() doesn't get called */
              done_window();
            *grevent = our_grevent;
            return;
          case updateEvt:
            if ( theWindow && (WindowPtr)mac_event.message == theWindow )
            {
              SetPortWindowPort( theWindow );
              BeginUpdate( theWindow );
              refresh_rectangle( surface,
                                 0, 0,
                                 thePixMap.bounds.right, thePixMap.bounds.bottom );
              EndUpdate( theWindow );
            }
#ifdef __MWERKS__
            else
            {
              SIOUXHandleOneEvent( &mac_event );
            }
#endif
            break;
          case mouseDown:
            {
              short part;
              WindowPtr wid;

              part = FindWindow( mac_event.where, &wid );
              if ( wid == theWindow )
              {
                if ( theWindow && part == inDrag) {
#if TARGET_API_MAC_CARBON
#define screenBounds NULL
#else
                  Rect *screenBounds = &qd.screenBits.bounds;
#endif
                  DragWindow( wid, mac_event.where, screenBounds);
                }
                else if (part == inGoAway)
                {
                  if ( TrackGoAway( theWindow, mac_event.where ) )
                  {
                    /* The user clicked the window away, emulate quit event */
                    done_window();
                    our_grevent.type = gr_event_key;
                    our_grevent.key = grKeyEsc;
                    *grevent = our_grevent;
                    return;
                  }
                }
                else if (part == inContent)
                {
                  SelectWindow( theWindow );
                }
              }
#ifdef __MWERKS__
              else
              {
                SIOUXHandleOneEvent( &mac_event );
              }
#endif
            }
            break;
          default:
            InitCursor();
            break;
          }
        }
    }
  }


static
grSurface*  init_surface( grSurface*  surface,
                          grBitmap*   bitmap )
{
  PixMapPtr pixMap = &thePixMap;

  Rect bounds;
  SetRect(&bounds, 0, 0, bitmap->width, bitmap->rows);

  switch (bitmap->mode)
  {
  case gr_pixel_mode_rgb24:
    bitmap->mode = gr_pixel_mode_rgb32;
    break;

  default:
    bitmap->mode = gr_pixel_mode_mono;
    bitmap->grays = 0;
    break;
  }

  /* create the bitmap - under MacOS, we support all modes as the GDI */
  /* handles all conversions automatically..                          */
  if ( grNewBitmap( bitmap->mode,
                    bitmap->grays,
                    bitmap->width,
                    bitmap->rows,
                    bitmap ) )
    return 0;

  surface->bitmap = *bitmap;

  /* initialize the PixMap to appropriate values */
  pixMap->baseAddr = (char*)bitmap->buffer;
  pixMap->rowBytes = bitmap->pitch;
  if (pixMap->rowBytes < 0)
     pixMap->rowBytes = -pixMap->rowBytes;
  pixMap->rowBytes |= 0x8000; /* flag indicating it's a PixMap, not a BitMap */
  pixMap->bounds = bounds;
  pixMap->pmVersion = baseAddr32; /* pixmap base address is 32-bit address */
  pixMap->packType = 0;
  pixMap->packSize = 0;
  pixMap->hRes = 72 << 16;
  pixMap->vRes = 72 << 16;
  pixMap->pmTable = 0;
#if OLDPIXMAPSTRUCT
  pixMap->planeBytes = 0;
  pixMap->pmReserved = 0;
#else
  pixMap->pixelFormat = 0;
  pixMap->pmExt = 0;
#endif

  switch ( bitmap->mode )
  {
  case gr_pixel_mode_mono:
  case gr_pixel_mode_gray:

    pixMap->pixelType = 0;   /* indexed color */
    pixMap->cmpCount = 1;

    if (bitmap->mode == gr_pixel_mode_mono)
    {
      pixMap->pixelSize = 1;
    }
    else
    {
      pixMap->pixelSize = 8;
      pixMap->pmTable = GetCTable(256); /* color palette matching FT's idea */
      break;                            /* of grayscale. See ftview.rsrc */
    }
    pixMap->cmpSize = pixMap->pixelSize;
    break;

  case gr_pixel_mode_rgb32:
    pixMap->pixelSize = 32;
    pixMap->pixelType = RGBDirect;
    pixMap->cmpCount = 3;
    pixMap->cmpSize = 8;
    pixMap->baseAddr -= 1;  /*SHH: goofy hack to reinterpret bitmap's RGBx -> xRGB */
    break;

  default:
    return 0;    /* Unknown mode */
  }

  /* create the window */
  OffsetRect(&bounds, 10, 44); /* place it at a decent location */
  theWindow = NewCWindow(NULL, &bounds, "\p???", 1, 0, (WindowRef)-1, 1, 0);

  /* fill in surface interface */
  surface->done         = (grDoneSurfaceFunc) done_surface;
  surface->refresh_rect = (grRefreshRectFunc) refresh_rectangle;
  surface->set_title    = (grSetTitleFunc)    set_title;
  surface->listen_event = (grListenEventFunc) listen_event;

  return surface;
}


  static int init_device( void )
  {
    return 0;
  }

  static void done_device( void )
  {
    /* won't get called either :-( */
  }

  grDevice  gr_mac_device =
  {
    sizeof( grSurface ),
    "mac",

    init_device,
    done_device,

    (grDeviceInitSurfaceFunc) init_surface,

    0,
    0
  };


/* End */
