#ifndef CYBERGRAPHICS_INTERN_H
#define CYBERGRAPHICS_INTERN_H

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif

struct IntCGFXBase
{
    struct Library libnode;
    struct ExecBase *sysbase;
    BPTR seglist;
    struct Library *utilitybase;
    struct Library *oopbase;
    struct GfxBase *gfxbase;
};

#define GetCGFXBase(base) ((struct IntCGFXBase *)base)


#undef OOPBase
#define OOPBase GetCGFXBase(CyberGfxBase)->oopbase

#undef UtilityBase
#define UtilityBase GetCGFXBase(CyberGfxBase)->utilitybase

#undef GfxBase
#define GfxBase GetCGFXBase(CyberGfxBase)->gfxbase

extern LONG driver_WriteLUTPixelArray(APTR srcrect, 
	UWORD srcx, UWORD srcy,
	UWORD srcmod, struct RastPort *rp, APTR ctable,
	UWORD destx, UWORD desty,
	UWORD sizex, UWORD sizey,
	UBYTE ctabformat,
	struct Library *CyberGfxBase);

#endif /* CYBERGRAPHICS_INTERN_H */
