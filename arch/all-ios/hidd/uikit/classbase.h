#include <exec/libraries.h>
#include <oop/oop.h>

#include "native_api.h"

struct UIKitInterface
{
    void (*GetMetrics)(struct DisplayMetrics *data);
    void *(*OpenDisplay)(unsigned int scrNo);
    void (*CloseDisplay)(void *display);
    void *(*NewBitMap)(void *display, unsigned int w, unsigned int h);
    void (*DisposeBitMap)(void *bitmap);
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
};

#define HostLibBase base->hostlibBase
