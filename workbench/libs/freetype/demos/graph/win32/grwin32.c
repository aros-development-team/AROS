/*******************************************************************
 *
 *  grwin32.c  graphics driver for Win32 platform
 *
 *  This is the driver for displaying inside a window under Win32,
 *  used by the graphics utility of the FreeType test suite.
 *
 *  Written by Antoine Leca.
 *  Copyright 1999-2000, 2001, 2002 by Antoine Leca, David Turner
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <grobjs.h>
#include <grdevice.h>


/* logging facility */
#include <stdarg.h>

#define  DEBUGxxx

#ifdef DEBUG
#define LOG(x)  LogMessage##x
#else
#define LOG(x)  /* rien */
#endif

#ifdef DEBUG
  static void  LogMessage( const char*  fmt, ... )
  {
    va_list  ap;

    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
  }
#endif
/*-------------------*/

/*  Size of the window. */
#define WIN_WIDTH   640u
#define WIN_HEIGHT  450u

/* These values can be changed, but WIN_WIDTH should remain for now a  */
/* multiple of 32 to avoid padding issues.                             */

  typedef struct  _Translator
  {
    ULONG   winkey;
    grKey   grkey;

  } Translator;

  static
  Translator  key_translators[] =
  {
    { VK_BACK,      grKeyBackSpace },
    { VK_TAB,       grKeyTab       },
    { VK_RETURN,    grKeyReturn    },
    { VK_ESCAPE,    grKeyEsc       },
    { VK_HOME,      grKeyHome      },
    { VK_LEFT,      grKeyLeft      },
    { VK_UP,        grKeyUp        },
    { VK_RIGHT,     grKeyRight     },
    { VK_DOWN,      grKeyDown      },
    { VK_PRIOR,     grKeyPageUp    },
    { VK_NEXT,      grKeyPageDown  },
    { VK_END,       grKeyEnd       },
    { VK_F1,        grKeyF1        },
    { VK_F2,        grKeyF2        },
    { VK_F3,        grKeyF3        },
    { VK_F4,        grKeyF4        },
    { VK_F5,        grKeyF5        },
    { VK_F6,        grKeyF6        },
    { VK_F7,        grKeyF7        },
    { VK_F8,        grKeyF8        },
    { VK_F9,        grKeyF9        },
    { VK_F10,       grKeyF10       },
    { VK_F11,       grKeyF11       },
    { VK_F12,       grKeyF12       }
  };

  static
  Translator  syskey_translators[] =
  {
    { VK_F1,        grKeyF1        }
  };

  static ATOM  ourAtom;

  typedef struct grWin32SurfaceRec_
  {
    grSurface     root;
    HWND          window;
    int           window_width;
    int           window_height;
    int           title_set;
    const char*   the_title;
    LPBITMAPINFO  pbmi;
    char          bmi[ sizeof(BITMAPINFO) + 256*sizeof(RGBQUAD) ];
    HBITMAP       hbm;
    grEvent       ourevent;
    int           eventToProcess;

  } grWin32Surface;


/* destroys the surface*/
static void
gr_win32_surface_done( grWin32Surface*  surface )
{
  /* The graphical window has perhaps already destroyed itself */
  if ( surface->window )
  {
    DestroyWindow ( surface->window );
    PostMessage( surface->window, WM_QUIT, 0, 0 );
  }
  grDoneBitmap( &surface->root.bitmap );
}


static void
gr_win32_surface_refresh_rectangle(
         grWin32Surface*  surface,
         int              x,
         int              y,
         int              w,
         int              h )
{
  HDC           hDC;
  int           row_bytes, delta;
  LPBITMAPINFO  pbmi   = surface->pbmi;
  HANDLE        window = surface->window;

  LOG(( "gr_win32_surface_refresh_rectangle: ( %p, %d, %d, %d, %d )\n",
        (long)surface, x, y, w, h ));

  /* clip update rectangle */

  if ( x < 0 )
  {
    w += x;
    x  = 0;
  }

  delta = x + w - surface->window_width;
  if ( delta > 0 )
    w -= delta;

  if ( y < 0 )
  {
    h += y;
    y  = 0;
  }

  delta = y + h - surface->window_height;
  if ( delta > 0 )
    h -= delta;

  if ( w <= 0 || h <= 0 )
    return;

  /* now, perform the blit */
  row_bytes = surface->root.bitmap.pitch;
  if (row_bytes < 0) row_bytes = -row_bytes;

  if ( row_bytes*8 != pbmi->bmiHeader.biWidth * pbmi->bmiHeader.biBitCount )
    pbmi->bmiHeader.biWidth  = row_bytes * 8 / pbmi->bmiHeader.biBitCount;

  hDC = GetDC ( window );
  SetDIBits ( hDC, surface->hbm,
              0,
              surface->root.bitmap.rows,
              surface->root.bitmap.buffer,
              pbmi,
              DIB_RGB_COLORS );

  ReleaseDC ( window, hDC );

  ShowWindow( window, SW_SHOW );
  InvalidateRect ( window, NULL, FALSE );
  UpdateWindow ( window );
}


static void
gr_win32_surface_set_title( grWin32Surface*  surface,
                            const char*      title )
{
  /* the title will be set on the next listen_event, just */
  /* record it there..                                    */
  surface->title_set = 1;
  surface->the_title = title;
}

static void
gr_win32_surface_listen_event( grWin32Surface*  surface,
                               int              event_mask,
                               grEvent*         grevent )
{
  MSG     msg;
  HANDLE  window = surface->window;

  event_mask=event_mask;  /* unused parameter */

  if ( window && !surface->title_set )
  {
    SetWindowText( window, surface->the_title );
    surface->title_set = 1;
  }

  surface->eventToProcess = 0;
  while (GetMessage( &msg, 0, 0, 0 ))
  {
    TranslateMessage( &msg );
    DispatchMessage( &msg );
    if (surface->eventToProcess)
      break;
  }

  *grevent = surface->ourevent;
}

/*
 * set graphics mode
 * and create the window class and the message handling.
 */


static grWin32Surface*
gr_win32_surface_init( grWin32Surface*  surface,
                       grBitmap*        bitmap )
{
  static RGBQUAD  black = {    0,    0,    0, 0 };
  static RGBQUAD  white = { 0xFF, 0xFF, 0xFF, 0 };
  LPBITMAPINFO    pbmi;

  /* find some memory for the bitmap header */
  surface->pbmi = pbmi = (LPBITMAPINFO) surface->bmi;

  LOG(( "Win32: init_surface( %p, %p )\n", surface, bitmap ));

  LOG(( "       -- input bitmap =\n" ));
  LOG(( "       --   mode   = %d\n", bitmap->mode ));
  LOG(( "       --   grays  = %d\n", bitmap->grays ));
  LOG(( "       --   width  = %d\n", bitmap->width ));
  LOG(( "       --   height = %d\n", bitmap->rows ));

  /* create the bitmap - under Win32, we support all modes as the GDI */
  /* handles all conversions automatically..                          */
  if ( grNewBitmap( bitmap->mode,
                    bitmap->grays,
                    bitmap->width,
                    bitmap->rows,
                    bitmap ) )
    return 0;

  LOG(( "       -- output bitmap =\n" ));
  LOG(( "       --   mode   = %d\n", bitmap->mode ));
  LOG(( "       --   grays  = %d\n", bitmap->grays ));
  LOG(( "       --   width  = %d\n", bitmap->width ));
  LOG(( "       --   height = %d\n", bitmap->rows ));

  bitmap->pitch        = -bitmap->pitch;
  surface->root.bitmap = *bitmap;

  /* initialize the header to appropriate values */
  memset( pbmi, 0, sizeof ( BITMAPINFO ) + sizeof ( RGBQUAD ) * 256 );

  pbmi->bmiHeader.biSize   = sizeof ( BITMAPINFOHEADER );
  pbmi->bmiHeader.biWidth  = bitmap->width;
  pbmi->bmiHeader.biHeight = bitmap->rows;
  pbmi->bmiHeader.biPlanes = 1;

  switch ( bitmap->mode )
  {
  case gr_pixel_mode_mono:
    pbmi->bmiHeader.biBitCount = 1;
    pbmi->bmiColors[0] = white;
    pbmi->bmiColors[1] = black;
    break;

  case gr_pixel_mode_rgb24:
    pbmi->bmiHeader.biBitCount    = 24;
    pbmi->bmiHeader.biCompression = BI_RGB;
    break;

  case gr_pixel_mode_gray:
    pbmi->bmiHeader.biBitCount = 8;
    pbmi->bmiHeader.biClrUsed  = bitmap->grays;
    {
      int   count = bitmap->grays;
      int   x;
      RGBQUAD*  color = pbmi->bmiColors;

      for ( x = 0; x < count; x++, color++ )
      {
        color->rgbRed   =
        color->rgbGreen =
        color->rgbBlue  = (unsigned char)(((count-x)*255)/count);
        color->rgbReserved = 0;
      }
    }
    break;

  default:
    return 0;         /* Unknown mode */
  }

  surface->window_width  = bitmap->width;
  surface->window_height = bitmap->rows;

  surface->window = CreateWindow(
        /* LPCSTR lpszClassName;    */ "FreeTypeTestGraphicDriver",
        /* LPCSTR lpszWindowName;   */ "FreeType Test Graphic Driver",
        /* DWORD dwStyle;           */  WS_OVERLAPPED | WS_SYSMENU,
        /* int x;                   */  CW_USEDEFAULT,
        /* int y;                   */  CW_USEDEFAULT,
        /* int nWidth;              */  bitmap->width + 2*GetSystemMetrics(SM_CXBORDER),
        /* int nHeight;             */  bitmap->rows  + GetSystemMetrics(SM_CYBORDER)
                                              + GetSystemMetrics(SM_CYCAPTION),
        /* HWND hwndParent;         */  HWND_DESKTOP,
        /* HMENU hmenu;             */  0,
        /* HINSTANCE hinst;         */  GetModuleHandle( NULL ),
        /* void FAR* lpvParam;      */  surface );

  if ( surface->window == 0 )
    return  0;

  surface->root.done         = (grDoneSurfaceFunc) gr_win32_surface_done;
  surface->root.refresh_rect = (grRefreshRectFunc) gr_win32_surface_refresh_rectangle;
  surface->root.set_title    = (grSetTitleFunc)    gr_win32_surface_set_title;
  surface->root.listen_event = (grListenEventFunc) gr_win32_surface_listen_event;

  return surface;
}


/* ---- Windows-specific stuff ------------------------------------------- */


  /* Message processing for our Windows class */
LRESULT CALLBACK Message_Process( HWND handle, UINT mess,
                                  WPARAM wParam, LPARAM lParam )
  {
    grWin32Surface*  surface = NULL;

    if ( mess == WM_CREATE )
    {
      /* WM_CREATE is the first message sent to this function, and the */
      /* surface handle is available from the 'lParam' parameter. We   */
      /* save its value in a window property..                         */
      /*                                                               */
      surface = ((LPCREATESTRUCT)lParam)->lpCreateParams;

      SetProp( handle, (LPCSTR)(LONG)ourAtom, surface );
    }
    else
    {
      /* for other calls, we retrieve the surface handle from the window */
      /* property.. ugly, isn't it ??                                    */
      /*                                                                 */
      surface = (grWin32Surface*) GetProp( handle, (LPCSTR)(LONG)ourAtom );
    }

    switch( mess )
    {
    case WM_DESTROY:
        /* warn the main thread to quit if it didn't know */
      surface->ourevent.type  = gr_event_key;
      surface->ourevent.key   = grKeyEsc;
      surface->eventToProcess = 1;
      surface->window         = 0;
      PostQuitMessage ( 0 );
      DeleteObject ( surface->hbm );
      return 0;

    case WM_CREATE:
      {
        HDC           hDC;
        LPBITMAPINFO  pbmi = surface->pbmi;

        hDC          = GetDC ( handle );
        surface->hbm = CreateDIBitmap (
          /* HDC hdc;     handle of device context        */ hDC,
          /* BITMAPINFOHEADER FAR* lpbmih;  addr.of header*/ &pbmi->bmiHeader,
          /* DWORD dwInit;  CBM_INIT to initialize bitmap */ 0,
          /* const void FAR* lpvBits;   address of values */ NULL,
          /* BITMAPINFO FAR* lpbmi;   addr.of bitmap data */ pbmi,
          /* UINT fnColorUse;      RGB or palette indices */ DIB_RGB_COLORS);
        ReleaseDC ( handle, hDC );
        break;
      }

    case WM_PAINT:
      {
      HDC           hDC, memDC;
      HANDLE        oldbm;
      PAINTSTRUCT   ps;

      hDC   = BeginPaint ( handle, &ps );
      memDC = CreateCompatibleDC( hDC );
      oldbm = SelectObject( memDC, surface->hbm );

      BitBlt ( hDC, 0, 0, surface->window_width, surface->window_height,
               memDC, 0, 0, SRCCOPY);

      ReleaseDC ( handle, hDC );
      SelectObject ( memDC, oldbm );
      DeleteObject ( memDC );
      EndPaint ( handle, &ps );
      return 0;
      }

    case WM_SYSKEYDOWN:
      {
        int          count = sizeof( syskey_translators )/sizeof( syskey_translators[0] );
        Translator*  trans = syskey_translators;
        Translator*  limit = trans + count;
        for ( ; trans < limit; trans++ )
          if ( wParam == trans->winkey )
          {
            surface->ourevent.key = trans->grkey;
            goto Do_Key_Event;
          }
        return DefWindowProc( handle, mess, wParam, lParam );
      }


    case WM_KEYDOWN:
      switch ( wParam )
      {
      case VK_ESCAPE:
        surface->ourevent.type  = gr_event_key;
        surface->ourevent.key   = grKeyEsc;
        surface->eventToProcess = 1;
        PostQuitMessage ( 0 );
        return 0;

      default:
        /* lookup list of translated keys */
        {
          int          count = sizeof( key_translators )/sizeof( key_translators[0] );
          Translator*  trans = key_translators;
          Translator*  limit = trans + count;
          for ( ; trans < limit; trans++ )
            if ( wParam == trans->winkey )
            {
              surface->ourevent.key = trans->grkey;
              goto Do_Key_Event;
            }
        }

        /* the key isn't found, default processing               */
        /* return DefWindowProc( handle, mess, wParam, lParam ); */
        return DefWindowProc( handle, mess, wParam, lParam );
    }

    case WM_CHAR:
      {
        surface->ourevent.key = wParam;

    Do_Key_Event:
        surface->ourevent.type  = gr_event_key;
        surface->eventToProcess = 1;
      }
      break;

    default:
       return DefWindowProc( handle, mess, wParam, lParam );
    }
    return 0;
  }

  static int
  gr_win32_device_init( void )
  {
    WNDCLASS ourClass = {
      /* UINT    style        */ 0,
      /* WNDPROC lpfnWndProc  */ Message_Process,
      /* int     cbClsExtra   */ 0,
      /* int     cbWndExtra   */ 0,
      /* HANDLE  hInstance    */ 0,
      /* HICON   hIcon        */ 0,
      /* HCURSOR hCursor      */ 0,
      /* HBRUSH  hbrBackground*/ 0,
      /* LPCTSTR lpszMenuName */ NULL,
      /* LPCTSTR lpszClassName*/ "FreeTypeTestGraphicDriver"
    };

    /* register window class */

    ourClass.hInstance    = GetModuleHandle( NULL );
    ourClass.hIcon        = LoadIcon(0, IDI_APPLICATION);
    ourClass.hCursor      = LoadCursor(0, IDC_ARROW);
    ourClass.hbrBackground= GetStockObject(BLACK_BRUSH);

    if ( RegisterClass(&ourClass) == 0 )
      return -1;

    /* add global atom */
    ourAtom = GlobalAddAtom( "FreeType.Surface" );

    return 0;
  }

  static void
  gr_win32_device_done( void )
  {
    GlobalDeleteAtom( ourAtom );
  }


  grDevice  gr_win32_device =
  {
    sizeof( grWin32Surface ),
    "win32",

    gr_win32_device_init,
    gr_win32_device_done,

    (grDeviceInitSurfaceFunc) gr_win32_surface_init,

    0,
    0
  };


/* End */
