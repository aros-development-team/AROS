
#ifndef BMBMRP_H
#define BMBMRP_H

VOID
NewBltMaskBitMapRastPort(
	struct BitMap      *srcbm,
    WORD				srcx,
    WORD				srcy,
    struct RastPort    *destrp,
    WORD				destx,
    WORD				desty,
    WORD				sizex,
    WORD				sizey,
    UBYTE				minterm,
    PLANEPTR			bltmask,
    struct Library 	   *Graphics,
    struct Library	   *Layers);

#endif