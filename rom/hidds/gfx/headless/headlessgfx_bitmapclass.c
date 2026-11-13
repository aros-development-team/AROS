/*
    Copyright (C) 2021, The AROS Development Team. All rights reserved.

    Desc: Bitmap class for Headless Gfx hidd.
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <aros/symbolsets.h>

#include <string.h>

#include "headlessgfx_hidd.h"

#include LC_LIBDEFS_FILE

#define MNAME_ROOT(x) HeadlessGfxBM__Root__ ## x
#define MNAME_BM(x) HeadlessGfxBM__Hidd_BitMap__ ## x

/*********** BitMap::New() *************************************/
OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("HeadlessGfx.BitMap::New()\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct HeadlessGfxBitMapData *data;
        data = OOP_INST_DATA(cl, o);
    } /* if created object */

    ReturnPtr("HeadlessGfx.BitMap::New()", OOP_Object *, o);
}

/*** BitMap::Get() *******************************************/

VOID MNAME_ROOT(Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HeadlessGfxBitMapData *data = OOP_INST_DATA(cl, o);
    ULONG              idx;

    if (IS_BM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_BitMap_Visible:
            *msg->storage = data->disp;
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*** BitMap::Set() *******************************************/

VOID MNAME_ROOT(Set)(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct HeadlessGfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG           idx;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
            case aoHidd_BitMap_Visible:
                D(bug("[HeadlessGfx:BitMap] Setting Visible to %d\n", tag->ti_Data));
                data->disp = tag->ti_Data;
                break;
            }
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
