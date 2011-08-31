#define X11_LIBNAME "x11gfx.hidd"

#define CLID_Hidd_X11Gfx "hidd.gfx.x11"

/* External part of library base */
struct X11Base
{
    struct Library library;

    UBYTE	   keycode2rawkey[256];
    BOOL	   havetable;
    OOP_Class	  *gfxclass;
};
