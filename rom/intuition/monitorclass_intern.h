#include <oop/oop.h>

struct MonitorData
{
    struct MinNode node;
    OOP_Object *driver;
    ULONG pixelformats[MONITOR_MAXPIXELFORMATS];
    OOP_Object *pfobjects[MONITOR_MAXPIXELFORMATS];
    
    Object *topleft;
    Object *topmiddle;
    Object *topright;
    Object *middleleft;
    Object *middleright;
    Object *bottomleft;
    Object *bottommiddle;
    Object *bottomright;
};
