/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common data structures of androidgfx.hidd
    Lang: English.
*/

#include <exec/tasks.h>
#include <hidd/unixio.h>

struct kbd_data;
struct mouse_data;

struct agfx_staticdata
{
    OOP_AttrBase *AttrBases;

    OOP_Class  *gfxclass;
    OOP_Class  *bmclass;
    OOP_Class  *mouseclass;
    OOP_Class  *kbdclass;

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
#undef HiddKbdAB
#undef HiddMouseAB
#undef HiddAttrBase
#define HiddChunkyBMAttrBase XSD(cl)->AttrBases[0]
#define HiddBitMapAttrBase   XSD(cl)->AttrBases[1]
#define HiddSyncAttrBase     XSD(cl)->AttrBases[2]
#define HiddPixFmtAttrBase   XSD(cl)->AttrBases[3]
#define HiddGfxAttrBase	     XSD(cl)->AttrBases[4]
#define HiddKbdAB	     XSD(cl)->AttrBases[5]
#define HiddMouseAB	     XSD(cl)->AttrBases[6]
#define HiddAttrBase	     XSD(cl)->AttrBases[7]
