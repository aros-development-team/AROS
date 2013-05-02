struct MonitorData
{
    struct MinNode node;
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
    BOOL screenGamma;
};

/* Offsets for gamma table */
#define GAMMA_R 0
#define GAMMA_G 256
#define GAMMA_B 512
