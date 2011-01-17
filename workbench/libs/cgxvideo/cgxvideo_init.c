#include <aros/symbolsets.h>
#include <hidd/graphics.h>
#include <proto/oop.h>

#include "cgxvideo_intern.h"

static int cgxv_init(struct IntCGXVBase *CGXVideoBase)
{
    CGXVideoBase->attrbases[0].interfaceID = IID_Hidd_BitMap;
    CGXVideoBase->attrbases[0].attrBase = &HiddBitMapAttrBase;
    CGXVideoBase->attrbases[1].interfaceID = IID_Hidd_Overlay;
    CGXVideoBase->attrbases[1].attrBase = &HiddOverlayAttrBase;

    return OOP_ObtainAttrBases(CGXVideoBase->attrbases);
}

static int cgxv_expunge(struct IntCGXVBase *CGXVideoBase)
{
    OOP_ReleaseAttrBases(CGXVideoBase->attrbases);

    return TRUE;
}

ADD2INITLIB(cgxv_init, 0);
ADD2EXPUNGELIB(cgxv_expunge, 0);
