/*******************************************************************
 *
 *  ft2demos/graph/allegro/gralleg.c
 *
 *  Allegro driver for MiGS (minimalist graphics subsystem). Allegro
 *  (Allegro Low LEvel Gaming ROutines) is a library for sound,
 *  graphics, timers, etc., and is available on 32-bit DOS, Windows,
 *  UNIX (X11, DGA, DGA2), Linux (svgalib, framebuffer) and BeOS.
 *
 *    http://www.talula.demon.co.uk/allegro/
 *    http://sourceforge.net/projects/alleg/
 *
 ******************************************************************/

/* FT graphics subsystem */
#include "grobjs.h"
#include "grdevice.h"

/* Allegro header */
#include <allegro.h>

static void set_graypalette()
{
    PALETTE pal;
    int i = 0;

    for(; i < 256; i++) {
        pal[i].r = i >> 2;
        pal[i].g = i >> 2;
        pal[i].b = i >> 2;
    }

    set_palette(pal);
}

static int almodes[] = {
    32,
    24,
    16,
    15,
    8,
    0
};

static int gray[256];

static int init_device(void)
{
    int* almode = almodes;

    if(allegro_init()) return -1;
    if(install_keyboard()) return -1;

    while(*almode) {
        set_color_depth(*almode);
        if(*almode == 8) set_graypalette();
        if(set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) == 0) {
            int i = 0;
            for(; i < 256; i++) gray[i] = makecol(i, i, i);

            clear(screen);
            return 0;
        }
        almode++;
    }

    return -1;
}

static void done_device(void)
{
    allegro_exit();
}

static void alset_title(grSurface* surface, const char* title_string)
{
    set_window_title(title_string);
}

static void alrefresh_rect(grSurface* surface, int x, int y, int width, int height)
{
    unsigned char* bufptr = surface->bitmap.buffer;
    int cx = 0, cy = 0;

    switch(surface->bitmap.mode) {
        case gr_pixel_mode_mono:
            for(; cy < height; cy++) {
                for(cx = 0; cx < width; cx++) {
                    putpixel(screen, cx + x, cy + y, (*bufptr++ ? gray[255] : gray[0]));
                }
            }
            break;

        case gr_pixel_mode_gray:
            for(; cy < height; cy++) {
                for(cx = 0; cx < width; cx++) {
                    putpixel(screen, cx + x, cy + y, gray[*bufptr++]);
                }
            }
            break;

        default:
            break;
    }
}

static void aldone(grSurface* surface)
{
    grDoneBitmap(&(surface->bitmap));
}

static int key_translator[] = {
    KEY_F1, grKeyF1,
    KEY_F2, grKeyF2,
    KEY_F3, grKeyF3,
    KEY_F4, grKeyF4,
    KEY_F5, grKeyF5,
    KEY_F6, grKeyF6,
    KEY_F7, grKeyF7,
    KEY_F8, grKeyF8,
    KEY_F9, grKeyF9,
    KEY_F10, grKeyF10,
    KEY_F11, grKeyF11,
    KEY_F12, grKeyF12,
    KEY_LEFT, grKeyLeft,
    KEY_RIGHT, grKeyRight,
    KEY_UP, grKeyUp,
    KEY_DOWN, grKeyDown,
    KEY_INSERT, grKeyIns,
    KEY_DEL, grKeyDel,
    KEY_HOME, grKeyHome,
    KEY_END, grKeyEnd,
    KEY_PGUP, grKeyPageUp,
    KEY_PGDN, grKeyPageDown,
    KEY_ESC, grKeyEsc,
    KEY_TAB, grKeyTab,
    KEY_BACKSPACE, grKeyBackSpace,
    KEY_ENTER, grKeyReturn,
    0, 0
};

static int translateKey(int scancode)
{
    int* trans = key_translator;

    while(*trans) {
        if(scancode == *trans++) {
            return *trans;
        }
        trans++;
    }

    return 0;
}

static int allisten_event(grSurface* surface, int event_mode, grEvent* event)
{
    int ch = 0, ascii = 0, scancode = 0, shifts = 0;

    event->type = gr_event_key;

    while(1) {
        ch = readkey();

        shifts = 0;

        if(key_shifts & KB_SHIFT_FLAG) shifts |= grKeyShift;
        if(key_shifts & KB_CTRL_FLAG) shifts |= grKeyCtrl;
        if(key_shifts & KB_ALT_FLAG) shifts |= grKeyAlt;

        ascii = ch & 0xFF;
        scancode = ch >> 8;

        if(ascii > 31 && ascii < 127) {
            event->key = ascii | shifts;
            return 1;
        }

        if( (ch = translateKey(scancode)) ) {
            event->key = ch | shifts;
            return 1;
        }
    }

    return 0;
}

static int init_surface(grSurface* surface, grBitmap* bitmap)
{
    if(grNewBitmap(bitmap->mode, bitmap->grays, bitmap->width, bitmap->rows, bitmap)) return 0;

    surface->device = &gr_alleg_device;
    surface->bitmap = *bitmap;
    surface->refresh = 0;
    surface->owner = 0;
    surface->saturation = 0;
    surface->blit_mono = 0;

    surface->refresh_rect = alrefresh_rect;
    surface->set_title = alset_title;
    surface->listen_event = allisten_event;
    surface->done = aldone;

    return 1;
}

grDevice gr_alleg_device =
{
    sizeof(grSurface),
    "Allegro",

    init_device,
    done_device,

    init_surface,

    0,
    0
};

