/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
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
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"
#include "mui.h"
#include "support.h"

extern struct Library *MUIMasterBase;


static struct MinNode *Node_Next(APTR node)
{
    if(node == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ->mln_Succ == NULL)
	return NULL;
    return ((struct MinNode*)node)->mln_Succ;
}

static struct MinNode *List_First(APTR list)
{
    if( !((struct MinList*)list)->mlh_Head) return NULL;
    if(((struct MinList*)list)->mlh_Head->mln_Succ == NULL) return NULL;
    return ((struct MinList*)list)->mlh_Head;
}

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
};

static ULONG Dataspace_New (struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_DataspaceData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
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
	    return NULL;
	}
    }

    return (ULONG)obj;
}


static ULONG  Dataspace_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    if (data->pool_allocated) DeletePool(data->pool_allocated);
    else CoerceMethod(cl,obj,MUIM_Dataspace_Clear);
    return DoSuperMethodA(cl, obj, msg);
}

static ULONG Dataspace_Add(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Add *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *replace;
    struct Dataspace_Node *node = (struct Dataspace_Node*)AllocVec(sizeof(struct Dataspace_Node)+msg->len,0);
    if (!node) return 0;

    replace = (struct Dataspace_Node *)List_First(&data->list);
    while (replace)
    {
	if (replace->id == msg->id)
	{
	    Remove((struct Node*)replace);
	    FreePooled(data->pool,replace,replace->len + sizeof(struct Dataspace_Node));
	    break;
	}
	replace = (struct Dataspace_Node*)Node_Next(replace);
    }

    AddTail((struct List*)&data->list,(struct Node*)node);
    node->id = msg->id;
    node->len = msg->len;
    CopyMem(msg->data,node+1,node->len);
    return 1;
}

static ULONG Dataspace_Clear(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Clear *msg)
{
    return 1;
}

static ULONG Dataspace_Find(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Find *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *find;

    find = (struct Dataspace_Node *)List_First(&data->list);
    while (find)
    {
	if (find->id == msg->id)
	{
	    return (ULONG)(find + 1);
	    break;
	}
	find = (struct Dataspace_Node*)Node_Next(find);
    }
	
    return NULL;
}

static ULONG Dataspace_Merge(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Merge *msg)
{
    return 1;
}

static LONG Dataspace_ReadIFF(struct IClass *cl, Object *obj, struct MUIP_Dataspace_ReadIFF *msg)
{
    struct ContextNode *cn;
    UBYTE *buffer, *p;
    LONG read;

    LONG len;
    ULONG id;

    cn = CurrentChunk(msg->handle);
    if (!cn) return IFFERR_EOF;

    if (!(buffer = AllocVec(cn->cn_Size,0)))
	return IFFERR_NOMEM;

    read = ReadChunkBytes(msg->handle,buffer,cn->cn_Size);
    if (read < 0)
    {
    	FreeVec(buffer);
    	return read;
    }

    p = buffer;

    while (p < buffer + read)
    {
#ifndef _AROS /* The check should be better processor depend */

        /* Since data can be stored on uneven addresses we must read
        ** them byte by byte as MC68000 doesn't like this
        */
        id = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
        p+=4;
        len = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
        p+=4;

        /* p might be uneven but MUIM_Dataspace_Add use CopyMem() */
#else
        {
	    /* Don't know about x86, but because of endianess it's probably better to read them by long */
	    ULONG *pl = (ULONG*)p;
	    id = *pl++;
	    len = *pl++;
	    p = (UBYTE*)pl;
        }
#endif

	/* Now add the data */
        DoMethod(obj,MUIM_Dataspace_Add,p,len,id);
        p += len;
    }
    return 0;
}

static ULONG Dataspace_Remove(struct IClass *cl, Object *obj, struct MUIP_Dataspace_Remove *msg)
{
    return 1;
}

static LONG Dataspace_WriteIFF(struct IClass *cl, Object *obj, struct MUIP_Dataspace_WriteIFF *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *iter;
    LONG rc;

    if ((rc = PushChunk(msg->handle,msg->type,msg->id,IFFSIZE_UNKNOWN))) return rc;

    iter = (struct Dataspace_Node *)List_First(&data->list);
    while (iter)
    {
    	rc = WriteChunkBytes(msg->handle,&iter->id,iter->len); /* ID - LEN - DATA */
    	if (rc < 0) return rc;
	iter = (struct Dataspace_Node*)Node_Next(iter);
    }
    if ((rc = PopChunk(msg->handle))) return rc;
    return 0;
}


/*
 * The class dispatcher
 */
#ifndef __AROS
static __asm IPTR Dataspace_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, Dataspace_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW: return Dataspace_New(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return Dataspace_Dispose(cl, obj, msg);
	case MUIM_Dataspace_Add: return Dataspace_Add(cl, obj, (APTR)msg);
	case MUIM_Dataspace_Clear: return Dataspace_Clear(cl, obj, (APTR)msg);
	case MUIM_Dataspace_Find: return Dataspace_Find(cl, obj, (APTR)msg);
	case MUIM_Dataspace_Merge: return Dataspace_Merge(cl, obj, (APTR)msg);
	case MUIM_Dataspace_ReadIFF: return (IPTR)Dataspace_ReadIFF(cl, obj, (APTR)msg);
	case MUIM_Dataspace_Remove: return Dataspace_Remove(cl, obj, (APTR)msg);
	case MUIM_Dataspace_WriteIFF: return (IPTR)Dataspace_WriteIFF(cl, obj, (APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Dataspace_desc = {
    MUIC_Dataspace,                        /* Class name */
    MUIC_Semaphore,                          /* super class name */
    sizeof(struct MUI_DataspaceData),      /* size of class own datas */
    (void*)Dataspace_Dispatcher                        /* class dispatcher */
};
