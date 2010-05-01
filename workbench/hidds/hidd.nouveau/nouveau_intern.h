#ifndef _NOUVEAU_INTERN_H
#define _NOUVEAU_INTERN_H
/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/graphics.h>

#include LC_LIBDEFS_FILE

#define CLID_Hidd_Gfx_Nouveau           "hidd.gfx.nouveau"

struct HIDDNouveauData
{
};

#define CLID_Hidd_BitMap_Nouveau        "hidd.bitmap.nouveau"
#define CLID_Hidd_BitMap_NouveauOff     "hidd.bitmap.nouveauoff"
#define CLID_Hidd_BitMap_NouveauPlanar  "hidd.bitmap.nouveauplanar"

struct HIDDNouveauBitMapData
{
    ULONG height;
    ULONG width;

    APTR buffer; /* TEMP FIELD */
};

struct planarbm_data
{
    UBYTE   **planes;
    ULONG   planebuf_size;
    ULONG   bytesperrow;
    ULONG   rows;
    UBYTE   depth;
    BOOL    planes_alloced;
};

struct staticdata
{
    OOP_Class       *gfxclass;
    OOP_Class       *bmclass;
    OOP_Class       *offbmclass;
    OOP_Class       *planarbmclass;
    
    OOP_AttrBase    pixFmtAttrBase;
    OOP_AttrBase    gfxAttrBase;
    OOP_AttrBase    syncAttrBase;
    OOP_AttrBase    bitMapAttrBase;
    OOP_AttrBase    planarAttrBase;
};

LIBBASETYPE 
{
    struct Library      base;
    struct staticdata   sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#endif /* _NOUVEAU_INTERN_H */
