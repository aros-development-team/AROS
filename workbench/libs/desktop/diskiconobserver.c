/*
   Copyright © 1995-2003, The AROS Development Team. All rights reserved.
   $Id$ 
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <libraries/desktop.h>
#include <libraries/mui.h>

#include "support.h"
#include "worker.h"
#include "desktop_intern.h"

#include <proto/desktop.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "presentation.h"
#include "iconobserver.h"
#include "diskiconobserver.h"

#include "desktop_intern_protos.h"

#include <string.h>


/*
   enum Unit { U_GIGABYTE, U_MEGABYTE, U_KILOBYTE, U_BYTE; };

   UBYTE* formatDiskUsageString(Object *obj) {

   This function will return a string in the form of: "1Mb in use, 2Mb free,
   33% full" "1kb in use, 2kb free, 33% full" "1b in use, 2b free, 33% full
   (validating)"

   struct InfoData *info BOOL success; BPTR lock; UBYTE retval=NULL; struct
   InfoData info; UQUAD usedBytes=0, totalBytes=0, usagePercent=0; Unit used, 
   total; LONG length=0;

   lock=Lock(_name(obj)); if(lock) { success=Info(lock, &info); if(success) {
   totalBytes=id.id_NumBlocks*id.id_BlocksPerByte;
   usedBytes=id.id_NumBlocksUsed*id.id_BlocksPerByte;
   usagePercent=(usedBytes/freeBytes);

   if(usedBytes > 1024*1024*1024) { usedBytes/=(1024*1024*1024);
   used=U_GIGABYTE; length+=(2+(int)log10(number)+1)); } else if(usedBytes >
   1024*1024) { usedBytes/=(1024*1024); used=U_MEGABYTE;
   length+=(2+(int)log10(number)+1); } else if(usedBytes > 1024) {
   usedBytes/=(1024); used=U_KILOBYTE; length+=(2+(int)log10(number)+1); }
   else { used=U_BYTE; length+=(1+(int)log10(number)+1); }

   length+=strlen(" in use, ");

   if(totalBytes > 1024*1024*1024) { totalBytes/=(1024*1024*1024);
   total=U_GIGABYTE; length+=(2+(int)log10(number)+1); } else if(totalBytes >
   1024*1024) { totalBytes/=(1024*1024); total=U_MEGABYTE;
   length+=(2+(int)log10(number)+1); } else if(totalBytes > 1024) {
   totalBytes/=(1024); total=U_KILOBYTE; length+=(2+(int)log10(number)+1); }
   else { total=U_BYTE; length+=(1+(int)log10(number)+1); }

   length+=strlen(" free"); }

   length+=((int)log10(usagePercent)+1+strlen("%");

   if(id.id_DiskState==ID_VALIDATING) length+=strlen(" (Validating)");

   length+=1;

   retval=AllocMem(length, MEMF_ANY);

   sprintf("1763Mb in use, 2Mb free, 33% full"); strcpy(retval, );


   Unlock(lock); }

   return retval; } 
 */
IPTR diskIconObserverNew(Class * cl, Object * obj, struct opSet * msg)
{
    IPTR            retval = 0;
    struct DiskIconObserverClassData *data;
    //struct TagItem *tag;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (retval)
    {
        obj = (Object *) retval;
        data = INST_DATA(cl, obj);
    }

    return retval;
}

IPTR diskIconObserverSet(Class * cl, Object * obj, struct opSet * msg)
{
    struct DiskIconObserverClassData *data;
    IPTR            retval = 1;
    struct TagItem *tag,
                   *tstate = msg->ops_AttrList;

    data = (struct DiskIconObserverClassData *) INST_DATA(cl, obj);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            default:
                break;
        }
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    return retval;
}

IPTR diskIconObserverGet(Class * cl, Object * obj, struct opGet * msg)
{
    IPTR            retval = 1;
    struct DiskIconObserverClassData *data;

    data = (struct DiskIconObserverClassData *) INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        default:
            retval = DoSuperMethodA(cl, obj, (Msg) msg);
            break;
    }

    return retval;
}

IPTR diskIconObserverDispose(Class * cl, Object * obj, Msg msg)
{
    IPTR            retval;

    retval = DoSuperMethodA(cl, obj, msg);

    return retval;
}

BOOPSI_DISPATCHER(IPTR, diskIconObserverDispatcher, cl, obj, msg)
{
    ULONG           retval = 0;

    switch (msg->MethodID)
    {
        case OM_NEW:
            retval = diskIconObserverNew(cl, obj, (struct opSet *) msg);
            break;
        case OM_SET:
            retval = diskIconObserverSet(cl, obj, (struct opSet *) msg);
            break;
        case OM_GET:
            retval = diskIconObserverGet(cl, obj, (struct opGet *) msg);
            break;
        case OM_DISPOSE:
            retval = diskIconObserverDispose(cl, obj, msg);
            break;
        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}
BOOPSI_DISPATCHER_END
