/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Common data structures of androidgfx.hidd
*/

#include <exec/tasks.h>
#include <hidd/unixio.h>

struct kbd_data;
struct mouse_data;

struct agfx_staticdata
{
    OOP_AttrBase *AttrBases;

    OOP_Class  *basebm;            /* baseclass for CreateObject */

    OOP_Class  *gfxclass;
    OOP_Class  *displayclass;
    OOP_Class  *bmclass;
    OOP_Class  *mouseclass;
    OOP_Class  *kbdclass;

    OOP_Object *displayhidd;
    OOP_Object *dmenum;
    struct mouse_data *mousehidd;
    struct kbd_data   *kbdhidd;
    OOP_Object	      *unixio;

    int DisplayPipe;
    int InputPipe;

    struct uioInterrupt serverInt;
    struct MinList	waitQueue;
};

struct AGFXBase
{
    struct Library library;
    struct agfx_staticdata xsd;
};

#define XSD(cl) (&((struct AGFXBase *)cl->UserData)->xsd)

#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddSyncAttrBase
#undef HiddPixFmtAttrBase
#undef HiddGfxAttrBase
#undef HiddDisplayAttrBase
#undef HiddDMEnumAttrBase
#undef HiddKbdAB
#undef HiddMouseAB
#undef HiddAttrBase
#define HiddChunkyBMAttrBase XSD(cl)->AttrBases[0]
#define HiddBitMapAttrBase   XSD(cl)->AttrBases[1]
#define HiddSyncAttrBase     XSD(cl)->AttrBases[2]
#define HiddPixFmtAttrBase   XSD(cl)->AttrBases[3]
#define HiddGfxAttrBase	     XSD(cl)->AttrBases[4]
#define HiddDisplayAttrBase  XSD(cl)->AttrBases[5]
#define HiddDMEnumAttrBase   XSD(cl)->AttrBases[6]
#define HiddKbdAB	     XSD(cl)->AttrBases[7]
#define HiddMouseAB	     XSD(cl)->AttrBases[8]
#define HiddAttrBase	     XSD(cl)->AttrBases[9]
