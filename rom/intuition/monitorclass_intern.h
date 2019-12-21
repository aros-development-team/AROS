#define USE_FAST_DISPLAYTOBMCOORDS 1
#define USE_FAST_BMTODISPLAYCOORDS 1

struct IMonitorNode
{
   /*
    * Actually this is Node. However, some MorphOS source code
    * directly checks MonitorName field, so for compatibility
    * we keep it this way.
    * We still want struct Node because in some other cases we use
    * FindName() on our list.
    */
    struct MinNode              node;
    UBYTE                       type;
    BYTE                        pri;
    const char                  *MonitorName;

    /* Monitor Display data */
    struct MonitorHandle        *handle;
    ULONG                       pixelformats[MONITOR_MAXPIXELFORMATS];
    OOP_Object                  *pfobjects[MONITOR_MAXPIXELFORMATS];
    IPTR                        FrameBufferType;
    struct Rectangle            FBBounds;

#if USE_FAST_DISPLAYTOBMCOORDS
    OOP_MethodFunc              displaytobmcoords;
    OOP_Class 	                *displaytobmcoords_Class;
#endif
#if USE_FAST_BMTODISPLAYCOORDS
    OOP_MethodFunc              bmtodisplaycoords;
    OOP_Class 	                *bmtodisplaycoords_Class;
#endif

    /* xxx */
    Object                      *topleft;
    Object                      *topmiddle;
    Object                      *topright;
    Object                      *middleleft;
    Object                      *middleright;
    Object                      *bottomleft;
    Object                      *bottommiddle;
    Object                      *bottomright;

    /* Monitor Cursor data */
    IPTR                        SpriteType;
    struct SharedPointer        *pointer;
    ULONG                       mouseX;
    ULONG                       mouseY;

    /* Monitor Gamma data */
    UBYTE                       *gamma;
    UBYTE                       *active_r;
    UBYTE                       *active_b;
    UBYTE                       *active_g;
    UBYTE                       screenGamma;

    BOOL                        dcsupported;
};

/* Offsets for gamma table */
#define GAMMA_R 0
#define GAMMA_G 256
#define GAMMA_B 512
