#ifndef _HOSTINTERFACE_H
#define _HOSTINTERFACE_H

#include <stdint.h>

#define HOSTINTERFACE_VERSION 6

struct HostInterface
{
    char *System;
    unsigned int Version;

    void *(*hostlib_Open)(const char *, char **);
    int   (*hostlib_Close)(void *, char **);
    void *(*hostlib_GetPointer)(void *, const char *, char **);
    int   (*KPutC)(int chr);
    int   (*host_GetTime)(int, uint64_t *, uint64_t *);
    struct MinList **ModListPtr;
    void  (*jit_write_protect)(int enabled);  /* W^X toggle for MAP_JIT (NULL if not needed) */

    /* Cocoa display (darwin-aarch64 only, NULL otherwise) */
    void *(*cocoa_display_init)(int w, int h);
    int   (*cocoa_display_get_pitch)(void);
    void  (*cocoa_display_refresh)(void);
    void  (*cocoa_runloop_step)(void);
    void  *cocoa_fb_base;   /* Framebuffer pixel base address */
    int    cocoa_fb_width;
    int    cocoa_fb_height;
    int    cocoa_fb_pitch;

    /* Input event ring buffer (written by Cocoa, read by AROS) */
    #define COCOA_EVENT_RING_SIZE 64
    #define COCOA_EVENT_MOUSE_MOVE   1
    #define COCOA_EVENT_MOUSE_PRESS  2
    #define COCOA_EVENT_MOUSE_RELEASE 3
    #define COCOA_EVENT_KEY_PRESS    4
    #define COCOA_EVENT_KEY_RELEASE  5
    volatile int cocoa_event_write;
    volatile int cocoa_event_read;
    struct { int type; int x, y; int button; int keycode; } cocoa_events[COCOA_EVENT_RING_SIZE];
};

#endif /* !_HOSTINTERFACE_H */
