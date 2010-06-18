struct MonitorData
{
    struct MinNode node;
    OOP_Object *driver;
    BOOL pixelformats[MONITOR_MAXPIXELFORMATS];
    
    Object *topleft;
    Object *topmiddle;
    Object *topright;
    Object *middleleft;
    Object *middleright;
    Object *bottomleft;
    Object *bottommiddle;
    Object *bottomright;
};
