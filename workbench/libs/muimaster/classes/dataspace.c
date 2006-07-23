/*
    Copyright © 2002-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/muimaster.h>

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct Dataspace_Node
{
    struct MinNode node;
    ULONG id;
    ULONG len;
    /* len bytes data follows */
};

struct MUI_DataspaceData
{
    /* We store this as a linear list, but it has O(n) when looking 
    ** for an entry which is bad, because the is the most fequently used
    ** operation.
    */
    struct MinList list;
    APTR pool;
    APTR pool_allocated;
    APTR current_pool;
};

IPTR Dataspace__OM_NEW (struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_DataspaceData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem**)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Dataspace_Pool:
		    data->pool = (APTR)tag->ti_Data;
		    break;
	}
    }

    NewList((struct List*)&data->list);

    if (!data->pool)
    {
	if (!(data->pool_allocated = CreatePool(0,4096,4096)))
	{
	    CoerceMethod(cl,obj,OM_DISPOSE);
	    return (IPTR)NULL;
	}
	data->current_pool = data->pool_allocated;
    }
    else
    {
	data->current_pool = data->pool;
    }

    return (IPTR)obj;
}


IPTR Dataspace__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    if (data->pool_allocated) DeletePool(data->pool_allocated);
    else CoerceMethod(cl,obj,MUIM_Dataspace_Clear);
    return DoSuperMethodA(cl, obj, msg);
}

IPTR Dataspace__MUIM_Add(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Add *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *replace;
    struct Dataspace_Node *node;

    if (NULL == msg->data || msg->len < 1 || msg->id < 1)
	return 0;

    node = (struct Dataspace_Node*)AllocVecPooled(data->current_pool,
						  sizeof(struct Dataspace_Node)+msg->len);
    if (NULL == node)
    {
	return 0;
    }

    replace = List_First(&data->list);
    while (replace)
    {
	if (replace->id == msg->id)
	{
	    Remove((struct Node*)replace);
	    FreeVecPooled(data->current_pool, replace);
	    break;
	}
	replace = Node_Next(replace);
    }

    AddTail((struct List*)&data->list,(struct Node*)node);
    node->id = msg->id;
    node->len = msg->len;
    CopyMem(msg->data, node + 1, node->len);
    return 1;
}

IPTR Dataspace__MUIM_Clear(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Clear *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *node;

    node = List_First(&data->list);
    while (node)
    {
	struct Dataspace_Node *tmp = node;
	node = Node_Next(node);

	FreeVecPooled(data->current_pool, tmp);
    }

    return 1;
}

IPTR Dataspace__MUIM_Find(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Find *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *find;

    find = List_First(&data->list);
    while (find)
    {
	if (find->id == msg->id)
	{
	    return (ULONG)(find + 1);
	}
	find = Node_Next(find);
    }
	
    return (IPTR)NULL;
}

IPTR Dataspace__MUIM_Merge(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Merge *msg)
{
    return 1;
}

IPTR Dataspace__MUIM_Remove(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Remove *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *node;

    node = List_First(&data->list);
    while (node)
    {
	if (node->id == msg->id)
	{
	    Remove((struct Node*)node);
	    FreeVecPooled(data->current_pool, node);
	    return 1;
	}
	node = Node_Next(node);
    }

    return 0;
}

IPTR Dataspace__MUIM_ReadIFF(struct IClass *cl, Object *obj, struct MUIP_Dataspace_ReadIFF *msg)
{
    struct ContextNode *cn;
    UBYTE *buffer, *p;
    LONG read;

    LONG len;
    ULONG id;

    cn = CurrentChunk(msg->handle);
    if (!cn)
    {
	return IFFERR_EOF;
    }
    if (!(buffer = AllocVec(cn->cn_Size,0)))
    {
	return IFFERR_NOMEM;
    }
    read = ReadChunkBytes(msg->handle,buffer,cn->cn_Size);
    if (read < 0)
    {
    	FreeVec(buffer);
    	return read;
    }

    p = buffer;

    while (p < buffer + read)
    {
        /* Since data can be stored on uneven addresses we must read
        ** them byte by byte as MC68000 doesn't like this
        */
        id = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
        p+=4;
        len = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
        p+=4;

        /* p might be uneven but MUIM_Dataspace_Add use CopyMem() */

	/* Now add the data */
        DoMethod(obj, MUIM_Dataspace_Add, (IPTR)p, len, id);
        p += len;
    }
    FreeVec(buffer);
    return 0;
}

IPTR Dataspace__MUIM_WriteIFF(struct IClass *cl, Object *obj, struct MUIP_Dataspace_WriteIFF *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *iter;
    LONG rc;

    if ((rc = PushChunk(msg->handle,msg->type,msg->id,IFFSIZE_UNKNOWN))) return rc;

    iter = List_First(&data->list);
    while (iter)
    {
	ULONG len = iter->len+8;
	/* Data is stored in big-endian whatever your machine.
	 * Be sure sure to convert data to big endian.
	 */
	iter->id = AROS_LONG2BE(iter->id);
	iter->len = AROS_LONG2BE(iter->len);
    	rc = WriteChunkBytes(msg->handle, &iter->id, len); /* ID - LEN - DATA */
	iter->id = AROS_LONG2BE(iter->id);
	iter->len = AROS_LONG2BE(iter->len);
    	if (rc < 0)
	{
	    return rc;
	}
	iter = Node_Next(iter);
    }
    if ((rc = PopChunk(msg->handle))) return rc;
    return 0;
}


BOOPSI_DISPATCHER(IPTR, Dataspace_Dispatcher, cl, obj, msg)
{
    IPTR res;

    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW: return Dataspace__OM_NEW(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return Dataspace__OM_DISPOSE(cl, obj, msg);
	case MUIM_Dataspace_Add:
	    DoMethod(obj, MUIM_Semaphore_Obtain);
	    res = Dataspace__MUIM_Add(cl, obj, (APTR)msg);
	    DoMethod(obj, MUIM_Semaphore_Release);
	    return res;
	case MUIM_Dataspace_Clear:
	    DoMethod(obj, MUIM_Semaphore_Obtain);
	    res = Dataspace__MUIM_Clear(cl, obj, (APTR)msg);
	    DoMethod(obj, MUIM_Semaphore_Release);
	    return res;
	case MUIM_Dataspace_Find:
	    DoMethod(obj, MUIM_Semaphore_ObtainShared);
	    res = Dataspace__MUIM_Find(cl, obj, (APTR)msg);
	    DoMethod(obj, MUIM_Semaphore_Release);
	    /* now that sem is released, imagine that the object gets freed ...
	     * it really needs that the caller locks the object,
	     * and release it when done with the result
	     */
	    return res;
	case MUIM_Dataspace_Merge:
	    DoMethod(obj, MUIM_Semaphore_Obtain);
	    res = Dataspace__MUIM_Merge(cl, obj, (APTR)msg);
	    DoMethod(obj, MUIM_Semaphore_Release);
	    return res;
	case MUIM_Dataspace_Remove:
	    DoMethod(obj, MUIM_Semaphore_Obtain);
	    res = Dataspace__MUIM_Remove(cl, obj, (APTR)msg);
	    DoMethod(obj, MUIM_Semaphore_Release);
	    return res;
	case MUIM_Dataspace_ReadIFF:
	    DoMethod(obj, MUIM_Semaphore_Obtain);
	    res = (IPTR)Dataspace__MUIM_ReadIFF(cl, obj, (APTR)msg);
	    DoMethod(obj, MUIM_Semaphore_Release);
	    return res;
	case MUIM_Dataspace_WriteIFF:
	    DoMethod(obj, MUIM_Semaphore_Obtain);
	    res = (IPTR)Dataspace__MUIM_WriteIFF(cl, obj, (APTR)msg);
	    DoMethod(obj, MUIM_Semaphore_Release);
	    return res;
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Dataspace_desc = {
    MUIC_Dataspace,                        /* Class name */
    MUIC_Semaphore,                        /* super class name */
    sizeof(struct MUI_DataspaceData),      /* size of class own datas */
    (void*)Dataspace_Dispatcher            /* class dispatcher */
};
