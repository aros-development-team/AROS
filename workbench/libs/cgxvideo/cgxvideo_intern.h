#ifndef CGXVIDEO_INTERN_H
#define CGXVIDEO_INTERN_H

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
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

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef GRAPHICS_VIEW_H
#   include <graphics/view.h>
#endif

#include <oop/oop.h>

#undef HiddOverlayAttrBase

struct IntCGXVBase
{
    struct Library libnode;
    OOP_AttrBase   HiddOverlayAttrBase;
};

#define GetCGXVBase(base) ((struct IntCGXVBase *)base)
#define HiddOverlayAttrBase (GetCGXVBase(CGXVideoBase)->HiddOverlayAttrBase)

struct VLayerHandle
{
    OOP_Object *obj;	/* Overlay object	  */
    OOP_Object *drv;	/* Graphics driver object */
};

#endif /* CGXVIDEO_INTERN_H */
