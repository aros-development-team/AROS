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
ULONG ght_GetAttr(OOP_Object *obj, ULONG attrID);
STRPTR ght_GetCLID(STRPTR hiddName);

OOP_Object * NewGC(OOP_Object *hiddGfx, ULONG gcType, struct TagItem *tagList);
void DisposeGC(OOP_Object *hiddGfx, OOP_Object *gc);
OOP_Object * NewBitMap(OOP_Object *hiddGfx, struct TagItem *tagList);
void DisposeBitMap(OOP_Object *hiddGfx, OOP_Object *bitMap);

#endif /* GFX_HIDD_TOOL_H */
