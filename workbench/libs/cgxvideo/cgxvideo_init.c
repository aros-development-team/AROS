#include <aros/symbolsets.h>
#include <hidd/graphics.h>
#include <proto/oop.h>

#include "cgxvideo_intern.h"

static int cgxv_init(struct IntCGXVBase *CGXVideoBase)
{
     HiddOverlayAttrBase = OOP_ObtainAttrBase(IID_Hidd_Overlay);
     
     return HiddOverlayAttrBase ? TRUE : FALSE;
}

static int cgxv_expunge(struct IntCGXVBase *CGXVideoBase)
{
    if (HiddOverlayAttrBase)
        OOP_ReleaseAttrBase(IID_Hidd_Overlay);

    return TRUE;
}

ADD2INITLIB(cgxv_init, 0);
ADD2EXPUNGELIB(cgxv_expunge, 0);
