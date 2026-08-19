/*
 * cocoa_display.m - Native Cocoa display backend for AROS on macOS Apple Silicon
 *
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 * This file is part of the AROS bootstrap executable.
 */

#import <Cocoa/Cocoa.h>
#import <IOSurface/IOSurface.h>
#import <QuartzCore/QuartzCore.h>
#include <pthread.h>
#include <stdlib.h>

/* C API */
#ifdef __cplusplus
extern "C" {
#endif

void *cocoa_display_init(int width, int height);
int   cocoa_display_get_pitch(void);
void  cocoa_display_refresh(void);
void  cocoa_display_shutdown(void);
void  cocoa_runloop_step(void);
void  cocoa_set_hiface(void *hiface);

#ifdef __cplusplus
}
#endif

/* ---------- Globals ---------- */

static IOSurfaceRef  g_surface    = NULL;
static NSWindow     *g_window     = NULL;
static NSView       *g_view       = NULL;
static int           g_width      = 0;
static int           g_height     = 0;
static int           g_pitch      = 0;
static void         *g_pixels     = NULL;

/* ---------- Input Event Helpers ---------- */

#include "hostinterface.h"

static struct HostInterface *g_hiface = NULL;

static void push_event(int type, int x, int y, int button, int keycode) {
    if (!g_hiface) return;
    int wr = __atomic_load_n(&g_hiface->cocoa_event_write, __ATOMIC_RELAXED);
    int next = (wr + 1) % COCOA_EVENT_RING_SIZE;
    int rd = __atomic_load_n(&g_hiface->cocoa_event_read, __ATOMIC_ACQUIRE);
    if (next == rd) return; /* full, drop */

    g_hiface->cocoa_events[wr].type = type;
    g_hiface->cocoa_events[wr].x = x;
    g_hiface->cocoa_events[wr].y = y;
    g_hiface->cocoa_events[wr].button = button;
    g_hiface->cocoa_events[wr].keycode = keycode;

    __atomic_store_n(&g_hiface->cocoa_event_write, next, __ATOMIC_RELEASE);
}

/* ---------- View ---------- */

@interface AROSView : NSView
@end

@implementation AROSView

- (BOOL)wantsUpdateLayer { return YES; }

- (void)updateLayer {
    if (g_surface) {
        IOSurfaceLock(g_surface, kIOSurfaceLockReadOnly, NULL);
        self.layer.contents = (__bridge id)g_surface;
        IOSurfaceUnlock(g_surface, kIOSurfaceLockReadOnly, NULL);
    }
}

- (BOOL)isOpaque { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (NSPoint)convertToFB:(NSEvent *)event {
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    /* Flip Y (Cocoa is bottom-up, AROS is top-down) */
    p.y = g_height - p.y;
    return p;
}

- (void)mouseMoved:(NSEvent *)event {
    NSPoint p = [self convertToFB:event];
    push_event(COCOA_EVENT_MOUSE_MOVE, (int)p.x, (int)p.y, 0, 0);
}
- (void)mouseDragged:(NSEvent *)event { [self mouseMoved:event]; }
- (void)rightMouseDragged:(NSEvent *)event { [self mouseMoved:event]; }
- (void)otherMouseDragged:(NSEvent *)event { [self mouseMoved:event]; }

- (void)mouseDown:(NSEvent *)event {
    NSPoint p = [self convertToFB:event];
    push_event(COCOA_EVENT_MOUSE_PRESS, (int)p.x, (int)p.y, 1, 0);
}
- (void)mouseUp:(NSEvent *)event {
    NSPoint p = [self convertToFB:event];
    push_event(COCOA_EVENT_MOUSE_RELEASE, (int)p.x, (int)p.y, 1, 0);
}
- (void)rightMouseDown:(NSEvent *)event {
    NSPoint p = [self convertToFB:event];
    push_event(COCOA_EVENT_MOUSE_PRESS, (int)p.x, (int)p.y, 2, 0);
}
- (void)rightMouseUp:(NSEvent *)event {
    NSPoint p = [self convertToFB:event];
    push_event(COCOA_EVENT_MOUSE_RELEASE, (int)p.x, (int)p.y, 2, 0);
}

- (void)keyDown:(NSEvent *)event {
    push_event(COCOA_EVENT_KEY_PRESS, 0, 0, 0, [event keyCode]);
}
- (void)keyUp:(NSEvent *)event {
    push_event(COCOA_EVENT_KEY_RELEASE, 0, 0, 0, [event keyCode]);
}

@end

/* ---------- Window Delegate ---------- */

@interface AROSWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation AROSWindowDelegate

- (void)windowWillClose:(NSNotification *)notification {
    _exit(0);
}

@end

/* ---------- App Delegate ---------- */

@interface AROSAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AROSAppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

/* ---------- C API Implementation ---------- */

void *cocoa_display_init(int width, int height) {
    @autoreleasepool {
        /* Ensure NSApp exists */
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        AROSAppDelegate *delegate = [[AROSAppDelegate alloc] init];
        [NSApp setDelegate:delegate];

        /* Create IOSurface */
        g_width = width;
        g_height = height;

        NSDictionary *props = @{
            (id)kIOSurfaceWidth:            @(width),
            (id)kIOSurfaceHeight:           @(height),
            (id)kIOSurfaceBytesPerElement:   @(4),
            (id)kIOSurfacePixelFormat:       @((int)'BGRA'),
            (id)kIOSurfaceBytesPerRow:       @(width * 4),
        };
        g_surface = IOSurfaceCreate((__bridge CFDictionaryRef)props);
        if (!g_surface) {
            fprintf(stderr, "[Cocoa] Failed to create IOSurface\n");
            return NULL;
        }

        g_pitch = (int)IOSurfaceGetBytesPerRow(g_surface);
        g_pixels = IOSurfaceGetBaseAddress(g_surface);

        /* Clear to dark grey */
        memset(g_pixels, 0x33, g_pitch * g_height);

        /* Create window */
        NSRect frame = NSMakeRect(0, 0, width, height);
        g_window = [[NSWindow alloc]
            initWithContentRect:frame
            styleMask:(NSWindowStyleMaskTitled |
                       NSWindowStyleMaskClosable |
                       NSWindowStyleMaskMiniaturizable |
                       NSWindowStyleMaskResizable)
            backing:NSBackingStoreBuffered
            defer:NO];

        [g_window setTitle:@"AROS"];
        [g_window center];
        [g_window setDelegate:[[AROSWindowDelegate alloc] init]];

        /* Create layer-backed view */
        g_view = [[AROSView alloc] initWithFrame:frame];
        [g_view setWantsLayer:YES];
        g_view.layer.contentsGravity = kCAGravityResize;
        g_view.layer.magnificationFilter = kCAFilterNearest;

        [g_window setContentView:g_view];
        [g_window makeKeyAndOrderFront:nil];
        [g_window setAcceptsMouseMovedEvents:YES];
        [NSApp activateIgnoringOtherApps:YES];

        /* Initial display */
        [g_view.layer setNeedsDisplay];

        fprintf(stderr, "[Cocoa] Display initialized: %dx%d pitch=%d pixels=%p\n",
                width, height, g_pitch, g_pixels);

        return g_pixels;
    }
}

int cocoa_display_get_pitch(void) {
    return g_pitch;
}

void cocoa_display_refresh(void) {
    /* Must be called from main thread or dispatch to it */
    if ([NSThread isMainThread]) {
        [g_view.layer setNeedsDisplay];
    } else {
        dispatch_async(dispatch_get_main_queue(), ^{
            [g_view.layer setNeedsDisplay];
        });
    }
}

void cocoa_display_shutdown(void) {
    @autoreleasepool {
        if (g_window) {
            [g_window close];
            g_window = nil;
        }
        if (g_surface) {
            CFRelease(g_surface);
            g_surface = NULL;
        }
        g_view = nil;
        g_pixels = NULL;
    }
}

void cocoa_set_hiface(void *hiface) {
    g_hiface = (struct HostInterface *)hiface;
}

void cocoa_runloop_step(void) {
    @autoreleasepool {
        NSEvent *event;
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                          untilDate:nil
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:YES])) {
            [NSApp sendEvent:event];
        }
        /* Refresh display each step */
        [g_view.layer setNeedsDisplay];
        [CATransaction flush];
        /* Small sleep to avoid spinning and allow display to update */
        [NSThread sleepForTimeInterval:0.016]; /* ~60fps */
    }
}
