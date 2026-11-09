/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Private, task-context palette invalidation for the display compositor.
*/
#ifndef GFX_PALETTE_H
#define GFX_PALETTE_H

#include <exec/ports.h>
#include <exec/semaphores.h>

struct HIDDCompositorData;
struct class_static_data;

struct GfxPaletteWorker
{
    struct SignalSemaphore control;
    struct SignalSemaphore notify;
    struct Task *task;
    ULONG signal;
    BOOL dirty;
    BOOL ready;
    BOOL stopping;
    struct Message reply;
};

void GfxPaletteInit(struct HIDDCompositorData *data);
BOOL GfxPaletteEnsure(struct HIDDCompositorData *data);
void GfxPaletteQueue(struct HIDDCompositorData *data);
void GfxPaletteStop(struct HIDDCompositorData *data);
void Compositor__Internal__FlushPalette(struct HIDDCompositorData *data);
BOOL Compositor__Internal__EnsurePaletteTask(OOP_Class *cl, OOP_Object *o);
BOOL Compositor__Internal__CanHandleSoftware(OOP_Class *cl, OOP_Object *o);
void Compositor__Internal__PaletteChanged(struct class_static_data *data_csd, OOP_Object *display);
BOOL BM__Internal__SnapshotPalette(struct class_static_data *data_csd, OOP_Object *bitmap,
                                 HIDDT_Color *colors, ULONG capacity);
void CM__Internal__Snapshot(OOP_Class *cl, OOP_Object *o, HIDDT_Color *colors, ULONG capacity);

#endif
