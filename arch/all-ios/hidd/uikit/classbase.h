#include <exec/interrupts.h>
#include <exec/libraries.h>
#include <exec/tasks.h>
#include <oop/oop.h>

#include "native_api.h"

struct bitmap_data;

struct UIKitInterface
{
    void  (*GetMetrics)(struct DisplayMetrics *data);
    void *(*OpenDisplay)(unsigned int scrNo);
    void  (*CloseDisplay)(void *display);
    void  (*NewContext)(struct bitmap_data *bitmap);
    void  (*DisposeContext)(void *context);
    void  (*PollEvents)(void);
};

struct UIKitBase
{
    struct Library	   lib;
    APTR		   hostlibBase;
    APTR		   hostlib;
    struct UIKitInterface *iface;
    struct DisplayMetrics  metrics;
    OOP_Class		  *gfxclass;
    OOP_Class		  *bmclass;
    OOP_Class		  *mouseclass;
    struct Task		  *eventTask;
    ULONG		   eventMask;
    struct Interrupt	   eventInt;
};

#define HostLibBase base->hostlibBase

void EventTask(struct UIKitBase *base);
