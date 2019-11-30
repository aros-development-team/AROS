#ifndef AMIGAVIDEO_INTERN_H
#define AMIGAVIDEO_INTERN_H

/* Private definitions for AmigaVideo class */

#define AMIGAVIDEO_LIBNAME "amigavideo.hidd"

#define CLID_Hidd_Gfx_AmigaVideo "hidd.gfx.amigavideo"

struct AmigaVideoBase
{
    struct Library              library;	/* Common library header */
};

struct copper2data
{
    UWORD                       *copper2;
    UWORD                       *copper2_palette;
    UWORD                       *copper2_palette_aga_lo;
    UWORD                       *copper2_scroll;
    UWORD                       *copper2_bplcon0;
    UWORD                       *copper2_bpl;
    UWORD                       *copper2_fmode;
    UWORD                       *copper2_tail;

    UBYTE                       extralines;
};

#endif /* ! AMIGAVIDEO_INTERN_H */
