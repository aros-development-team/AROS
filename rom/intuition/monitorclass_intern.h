struct IMonitorNode
{
   /*
    * Actually this is Node. However, some MorphOS source code
    * directly checks MonitorName field, so for compatibility
    * we keep it this way.
    * We still want struct Node because in some other cases we use
    * FindName() on our list.
    */
    struct MinNode node;
    UBYTE type;
    BYTE pri;
    const char *MonitorName;

    struct MonitorHandle *handle;
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

    struct SharedPointer *pointer;
    ULONG mouseX;
    ULONG mouseY;

    UBYTE *gamma;
    UBYTE *active_r;
    UBYTE *active_b;
    UBYTE *active_g;
    UBYTE  screenGamma;
};

/* Offsets for gamma table */
#define GAMMA_R 0
#define GAMMA_G 256
#define GAMMA_B 512
