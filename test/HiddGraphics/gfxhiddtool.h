#ifndef GFX_HIDD_TOOL_H
#define GFX_HIDD_TOOL_H

#define AROS_USE_OOP

#include <utility/tagitem.h>
#include <oop/oop.h>

#define GHT_LIB(name, version, adr) {name, version, (struct Library **) adr}

struct ght_OpenLibs
{
    STRPTR libName;
    ULONG  version;
    struct Library **base;
};

BOOL ght_OpenLibs(struct ght_OpenLibs *libsArray);
void  ght_CloseLibs(struct ght_OpenLibs *libsArray);
ULONG ght_GetAttr(Object *obj, ULONG attrID);
STRPTR ght_GetCLID(STRPTR hiddName);

Object * NewGC(Object *hiddGfx, ULONG gcType, struct TagItem *tagList);
void DisposeGC(Object *hiddGfx, Object *gc);
Object * NewBitMap(Object *hiddGfx, struct TagItem *tagList);
void DisposeBitMap(Object *hiddGfx, Object *bitMap);

#endif /* GFX_HIDD_TOOL_H */
