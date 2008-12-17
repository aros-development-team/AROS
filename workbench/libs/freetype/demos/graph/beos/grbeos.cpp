/*******************************************************************
 *
 *  grbeos.c  graphics driver for BeOS platform.              0.1
 *
 *  This is the driver for displaying inside a window under BeOS,
 *  used by the graphics utility of the FreeType test suite.
 *
 *  Written by Michael Pfeiffer.
 *  Copyright 2001 by Michael Pfeiffer
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT. By continuing to use, modify or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

#include <be/app/Application.h>
#include <be/app/MessageQueue.h>
#include <be/interface/InterfaceDefs.h>
#include <be/interface/Window.h>
#include <be/interface/View.h>
#include <be/interface/Bitmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grobjs.h>
#include <grdevice.h>

#include "grbeos.h"


/* logging facility */
#include <stdarg.h>

#define  DEBUG

#ifdef DEBUG
#define LOG(x)  LogMessage##x
#else
#define LOG(x)  /* empty */
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

 typedef struct  _Translator
 {
   int32   beoskey;
grKey   grkey;

 } Translator;

 static
 Translator  key_translators[] =
 {
   { B_BACKSPACE,   grKeyBackSpace },
   { B_TAB,         grKeyTab       },
   { B_ENTER,       grKeyReturn    },
   { B_ESCAPE,      grKeyEsc       },
   { B_HOME,        grKeyHome      },
   { B_LEFT_ARROW,  grKeyLeft      },
   { B_UP_ARROW,    grKeyUp        },
   { B_RIGHT_ARROW, grKeyRight     },
   { B_DOWN_ARROW,  grKeyDown      },
   { B_PAGE_UP,     grKeyPageUp    },
   { B_PAGE_DOWN,   grKeyPageDown  },
   { B_END,         grKeyEnd       }
 };

 static
 Translator  fkey_translators[] =
 {
   { B_F1_KEY,      grKeyF1        },
   { B_F2_KEY,      grKeyF2        },
   { B_F3_KEY,      grKeyF3        },
   { B_F4_KEY,      grKeyF4        },
   { B_F5_KEY,      grKeyF5        },
   { B_F6_KEY,      grKeyF6        },
   { B_F7_KEY,      grKeyF7        },
   { B_F8_KEY,      grKeyF8        },
   { B_F9_KEY,      grKeyF9        },
   { B_F10_KEY,     grKeyF10       },
   { B_F11_KEY,     grKeyF11       },
   { B_F12_KEY,     grKeyF12       }
 };

 static Translator* find_key(int32 key, Translator t[], int size) {
   for (int i = 0; i < size; i++) {
     if (t[i].beoskey == key) {
	return &t[i];
     }
   }
   return NULL;
 }

static const grPixelMode  pixel_modes[] =
{
	gr_pixel_mode_mono,
	gr_pixel_mode_gray
//	gr_pixel_mode_rgb565,
//	gr_pixel_mode_rgb32,
};

class Window;

typedef struct grBeOSSurface_
{
  grSurface root;
  Window*   window;
} grBeOSSurface;

class Window : public BWindow {
 private:
  grBeOSSurface* _surface;
  BBitmap*       _offscreen;
  BBitmap*       _bitmap;
  BMessageQueue  _event_queue;
  sem_id         _locker;

  class View : public BView {
    BBitmap*       _offscreen;
    BMessageQueue* _event_queue;
    sem_id         _locker;

    public:
     View(BBitmap* offscreen, BMessageQueue* event_queue, sem_id locker, BRect r);
     void Draw(BRect r);
     void KeyDown(const char *bytes, int32 numBytes);
  };
  View* _view;

 public:
  Window(grBeOSSurface* surface, grBitmap* bitmap);
  ~Window();

  void Refresh(int x, int y, int w, int h);
  int listen_event(int event_mask, grEvent* grevent);

  static grSurface*     init_surface(grSurface* surface, grBitmap* bitmap);
  static void           done_surface(grSurface* surface);
  static void           refresh_rectangle(grSurface* surface, int x, int y, int w, int h);
  static void           set_title(grSurface* surface, const char* title);
  static int            listen_event(grSurface* surface, int event_mask, grEvent* grevent);
};

static int init_device();
static void done_device(void);

grDevice  gr_beos_device =
{
  sizeof( grBeOSSurface ),
  "beos",

  init_device,
  done_device,

  (grDeviceInitSurfaceFunc) Window::init_surface,

  0,
  0
};

static int init_device() {
  gr_beos_device.num_pixel_modes = 2;
  gr_beos_device.pixel_modes     = (grPixelMode*) pixel_modes;
  new BApplication("application/x.mp.freetype.test");
  return 0;
}

static void done_device(void) {
  if (be_app) {
    delete be_app;
    be_app = NULL;
  }
}


Window::View::View(BBitmap* offscreen, BMessageQueue* event_queue, sem_id locker, BRect r) :
 BView(r, "", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
 _offscreen(offscreen),
 _event_queue(event_queue),
 _locker(locker) {
 SetViewColor(B_TRANSPARENT_COLOR);
}

void Window::View::Draw(BRect r) {
  DrawBitmap(_offscreen);
}

void Window::View::KeyDown(const char *bytes, int32 numBytes) {
  BMessage* m = Window()->CurrentMessage();
  int32 key;
  if (B_OK == m->FindInt32("key", &key)) {
    Translator* t = NULL;
    if (numBytes == 1) {
	  if (*bytes == B_FUNCTION_KEY) {
        t = find_key(key, fkey_translators, sizeof(fkey_translators)/sizeof(Translator));
      } else {
        t = find_key(*bytes, key_translators, sizeof(key_translators)/sizeof(Translator));
      }
    }
    if (t || numBytes == 1) {
	  _event_queue->Lock();
	  if (_event_queue->IsEmpty()) release_sem(_locker);
	  _event_queue->AddMessage(new BMessage(t ? t->grkey : *bytes));
	  _event_queue->Unlock();
	  return;
	}
  }
  BView::KeyDown(bytes, numBytes);
}

int Window::listen_event(int event_mask, grEvent* grevent) {
  acquire_sem(_locker);
  _event_queue.Lock();
  BMessage* m = _event_queue.NextMessage();
  if (!_event_queue.IsEmpty()) release_sem(_locker);
  _event_queue.Unlock();
  grevent->type = gr_key_down;
  grevent->key = (grKey)m->what;
  delete m;
  return 0;
}

Window::Window(grBeOSSurface* surface, grBitmap* bitmap) :
 BWindow(BRect(20, 20, 100, 100), "", B_TITLED_WINDOW, B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE) {
  _locker = create_sem(0, "event_locker");
  _surface = surface;
  surface->root.done         = done_surface;
  surface->root.refresh_rect = refresh_rectangle;
  surface->root.set_title    = set_title;
  surface->root.listen_event = listen_event;
  surface->window = this;
  int w = bitmap->width;
  int h = bitmap->rows;
  BRect r(0, 0, w, h);
  switch (bitmap->mode) {
	case gr_pixel_mode_mono:
	  _bitmap = new BBitmap(r, B_GRAY1);
	  break;
	case gr_pixel_mode_gray:
	  _bitmap = new BBitmap(r, B_CMAP8);
	  break;
	case gr_pixel_mode_rgb565:
	  _bitmap = new BBitmap(r, B_RGB16);
	  break;
	case gr_pixel_mode_rgb32:
	  _bitmap = new BBitmap(r, B_RGB32);
	  break;
	default:
	  LOG(("unsupported mode"));
	  exit(-1);
  }

  bitmap->buffer = (unsigned char*)_bitmap->Bits();
  bitmap->pitch  = _bitmap->BytesPerRow();
  _surface->root.bitmap = *bitmap;
  _offscreen = new BBitmap(r, B_RGB32);

  _view = new View(_offscreen, &_event_queue, _locker, r);
  AddChild(_view);
  _view->MakeFocus(true);
  ResizeTo(w, h);
  Show();
}

Window::~Window() {
  delete_sem(_locker);
  delete _offscreen; delete _bitmap;
}

void Window::Refresh(int x, int y, int w, int h) {
  int32* d = (int32*)_offscreen->Bits();
  int32* dl = d;
  uint8* s = (uint8*)_bitmap->Bits();
  uint8* sl = s;
  if (Lock()) {
    switch(_surface->root.bitmap.mode) {
	case gr_pixel_mode_mono:
	  for (y = 0; y < _surface->root.bitmap.rows; y++) {
	    sl = s;
	    dl = d;
	    for (x = 0; x < _surface->root.bitmap.width; x++) {
	      *dl = *sl ? -1 : 0;
	      sl++; dl++;
	    }
	    s += _bitmap->BytesPerRow();
	    d = (int32*)(((char*)d) + _offscreen->BytesPerRow());
	  }
	  break;
	case gr_pixel_mode_gray:
	  for (y = 0; y < _surface->root.bitmap.rows; y++) {
	    sl = s;
	    int8* dx = (int8*)d;
	    for (x = 0; x < _surface->root.bitmap.width; x++) {
	      *dx = *sl; dx++;
	      *dx = *sl; dx++;
	      *dx = *sl; dx++;
	      *dx = *sl; dx++;
	      sl++;
	    }
	    s += _bitmap->BytesPerRow();
	    d = (int32*)(((char*)d) + _offscreen->BytesPerRow());
	  }
	  break;
	default:
	  fprintf(stderr, "unsupported mode: %d\n", _surface->root.bitmap.mode);
	  break;
    }
    _view->Invalidate();
    Unlock();
  }
}

grSurface* Window::init_surface( grSurface* surface, grBitmap* bitmap) {
  new Window((grBeOSSurface*) surface, bitmap);
  return surface;
}

void Window::done_surface(grSurface* surface) {
  Window* w = ((grBeOSSurface*)surface)->window;
  if (w->Lock()) {
    w->PostMessage(B_QUIT_REQUESTED);
    w->Unlock();
  }
}

void Window::refresh_rectangle(grSurface* surface, int x, int y, int w, int h) {
  Window* win = ((grBeOSSurface*)surface)->window;
  win->Refresh(x, y, w, h);
}

void Window::set_title(grSurface* surface, const char* title) {
  Window* win = ((grBeOSSurface*)surface)->window;
  if (win->Lock()) {
    win->SetTitle(title);
    win->Unlock();
  }
}

int Window::listen_event(grSurface* surface, int event_mask, grEvent* grevent) {
  Window* win = ((grBeOSSurface*)surface)->window;
  return win->listen_event(event_mask, grevent);
}

/* End */

