/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
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

/****** Dataspace.mui/MUIC_Dataspace ************
*
*   NAME
*       MUIC_Dataspace
*
*   SUPERCLASS
*       Semaphore.mui
*
*   DESCRIPTION
*       Store copied byte records under nonzero ULONG identifiers. The class
*       supports insertion/replacement, lookup, removal, and an IFF record format.
*       Storage comes from a caller-supplied pool or an internally created pool.
*
*       Public record methods take the inherited semaphore. Find holds a shared
*       lock only during the search; mutation and IFF methods take an exclusive
*       lock. Object disposal is not synchronized by these method locks. The
*       caller must keep the object alive throughout all operations and uses of
*       borrowed record pointers.
*
*   BUGS
*       Merge is a stub. Clear frees records without resetting the list links.
*       ReadIFF can return success after an incomplete import. Consult the method
*       entries before relying on these operations.
*
*   SEE ALSO
*       MUIA_Dataspace_Pool, MUIM_Dataspace_Add, MUIM_Dataspace_Find
*
*/

/****** Dataspace.mui/MUIA_Dataspace_Pool ************
*
*   NAME
*       MUIA_Dataspace_Pool
*
*   FUNCTION
*       Select the Exec pool used for record allocation. This is an
*       initialization-only APTR attribute. Omission or NULL creates an owned
*       pool using CreatePool(0, 4096, 4096); construction fails if that fails.
*       A non-NULL value is borrowed and is not deleted by Dataspace.
*
*   NOTES
*       Keep a supplied pool alive until the Dataspace object is disposed of.
*       The object's semaphore protects its own record methods, not unrelated
*       users of a shared pool. Coordinate those users separately.
*       Disposal deletes an owned pool, or frees the object's records from a
*       borrowed pool.
*
*   SEE ALSO
*       MUIC_Dataspace, MUIM_Dataspace_Clear
*
*/

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
     ** for an entry, which is bad because this is the most frequently used
     ** operation.
     */
    struct MinList list;
    APTR pool;
    APTR pool_allocated;
    APTR current_pool;
};

/* Construct the Semaphore superclass, initialize the record list, and bind
 * a borrowed pool or create an owned one. On pool failure, dispose of the
 * partially built object and return zero; otherwise return the object.
 */
IPTR Dataspace__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_DataspaceData *data;
    struct TagItem *tags, *tag;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Dataspace_Pool:
            data->pool = (APTR) tag->ti_Data;
            break;
        }
    }

    NewList((struct List *)&data->list);

    if (!data->pool)
    {
        if (!(data->pool_allocated = CreatePool(0, 4096, 4096)))
        {
            CoerceMethod(cl, obj, OM_DISPOSE);
            return (IPTR) NULL;
        }
        data->current_pool = data->pool_allocated;
    }
    else
    {
        data->current_pool = data->pool;
    }

    return (IPTR) obj;
}


/* Dispose after all users have finished. Delete an owned pool wholesale,
 * or free this object's records from a borrowed pool, then dispose of the
 * superclass. This method does not acquire the inherited semaphore.
 */
IPTR Dataspace__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    if (data->pool_allocated)
        DeletePool(data->pool_allocated);
    else
        CoerceMethod(cl, obj, MUIM_Dataspace_Clear);
    return DoSuperMethodA(cl, obj, msg);
}

/****** Dataspace.mui/MUIM_Dataspace_Add ************
*
*   NAME
*       MUIM_Dataspace_Add
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Dataspace_Add, (IPTR)data, len, id);
*
*   FUNCTION
*       Copy a record into the dataspace. A record with the same identifier is
*       replaced after new storage has been allocated. The new record is appended
*       to the list, including when it replaces an existing record.
*
*   INPUTS
*       data - Non-NULL pointer to at least len readable bytes.
*       len - Positive LONG byte count that fits the record allocation, including
*             the internal node header.
*       id - Nonzero ULONG identifier.
*
*   RESULT
*       1 after insertion; 0 for NULL data, a nonpositive length, id == 0,
*       or allocation failure. Those failures leave an existing record intact.
*
*   NOTES
*       The caller retains its input storage. The dataspace owns the copy until
*       replacement, removal, clearing, or disposal. Do not use the old payload
*       of the same identifier as input: replacement frees that record before
*       CopyMem() reads the supplied bytes. Copy such data to separate storage
*       first. The dispatcher holds an exclusive semaphore lock.
*
*   SEE ALSO
*       MUIM_Dataspace_Find, MUIM_Dataspace_Remove
*
*/
IPTR Dataspace__MUIM_Add(struct IClass *cl, Object *obj,
    struct MUIP_Dataspace_Add *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *replace;
    struct Dataspace_Node *node;

    if (NULL == msg->data || msg->len < 1 || msg->id < 1)
        return 0;

    node = (struct Dataspace_Node *)AllocVecPooled(data->current_pool,
        sizeof(struct Dataspace_Node) + msg->len);
    if (NULL == node)
    {
        return 0;
    }

    replace = List_First(&data->list);
    while (replace)
    {
        if (replace->id == msg->id)
        {
            Remove((struct Node *)replace);
            FreeVecPooled(data->current_pool, replace);
            break;
        }
        replace = Node_Next(replace);
    }

    AddTail((struct List *)&data->list, (struct Node *)node);
    node->id = msg->id;
    node->len = msg->len;
    CopyMem(msg->data, node + 1, node->len);
    return 1;
}

/****** Dataspace.mui/MUIM_Dataspace_Clear ************
*
*   NAME
*       MUIM_Dataspace_Clear
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Dataspace_Clear);
*
*   FUNCTION
*       Free each record allocation while holding the exclusive semaphore.
*
*   INPUTS
*       No arguments beyond MethodID.
*
*   RESULT
*       Always 1 after the loop completes.
*
*   BUGS
*       The implementation does not unlink the freed records or reinitialize
*       the list. After clearing a nonempty dataspace, its list contains dangling
*       pointers. The return value does not establish a reusable empty object.
*       Subsequent record operations or borrowed-pool disposal can access freed
*       storage. Do not use this method to reset a live nonempty dataspace.
*
*   SEE ALSO
*       MUIM_Dataspace_Remove, MUIA_Dataspace_Pool
*
*/
IPTR Dataspace__MUIM_Clear(struct IClass *cl, Object *obj,
    struct MUIP_Dataspace_Clear *msg)
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

/****** Dataspace.mui/MUIM_Dataspace_Find ************
*
*   NAME
*       MUIM_Dataspace_Find
*
*   SYNOPSIS
*       APTR data = (APTR)DoMethod(obj, MUIM_Dataspace_Find, id);
*
*   FUNCTION
*       Search the record list for an identifier and return its payload.
*
*   INPUTS
*       id - ULONG identifier to find.
*
*   RESULT
*       Borrowed pointer to the stored bytes, or NULL if no record matches.
*       No length is returned; the caller must know the record format.
*
*   NOTES
*       Do not free the pointer. Replacement of that identifier, removal,
*       clearing, or disposal invalidates it. The dispatcher releases its shared
*       lock before returning, so the result alone is not protected against
*       concurrent changes. Keep the object alive and prevent mutation throughout
*       the use of its payload. For concurrent access, hold the inherited
*       semaphore exclusively around both lookup and payload use, then release it.
*
*   SEE ALSO
*       MUIM_Dataspace_Add, MUIM_Semaphore_Obtain, MUIM_Semaphore_Release
*
*/
IPTR Dataspace__MUIM_Find(struct IClass *cl, Object *obj,
    struct MUIP_Dataspace_Find *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *find;

    find = List_First(&data->list);
    while (find)
    {
        if (find->id == msg->id)
        {
            return (IPTR) (find + 1);
        }
        find = Node_Next(find);
    }

    return (IPTR) NULL;
}

/****** Dataspace.mui/MUIM_Dataspace_Merge ************
*
*   NAME
*       MUIM_Dataspace_Merge
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Dataspace_Merge, (IPTR)dataspace);
*
*   FUNCTION
*       Return 1 without copying or changing records. This is a stub.
*
*   INPUTS
*       dataspace - Declared source-object argument; ignored.
*
*   RESULT
*       Always 1. This value does not indicate that a merge occurred.
*
*   NOTES
*       The dispatcher takes and releases the destination object's exclusive
*       semaphore even though the method body does no work.
*
*   SEE ALSO
*       MUIM_Dataspace_Add
*
*/
IPTR Dataspace__MUIM_Merge(struct IClass *cl, Object *obj,
    struct MUIP_Dataspace_Merge *msg)
{
    return 1;
}

/****** Dataspace.mui/MUIM_Dataspace_Remove ************
*
*   NAME
*       MUIM_Dataspace_Remove
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Dataspace_Remove, id);
*
*   FUNCTION
*       Unlink and free the first record with the specified identifier.
*
*   INPUTS
*       id - ULONG identifier to remove.
*
*   RESULT
*       1 if a record was removed; 0 if no record matched.
*
*   NOTES
*       The dispatcher holds an exclusive semaphore lock. A successful removal
*       invalidates all borrowed pointers to that record.
*
*   SEE ALSO
*       MUIM_Dataspace_Find, MUIM_Dataspace_Add
*
*/
IPTR Dataspace__MUIM_Remove(struct IClass *cl, Object *obj,
    struct MUIP_Dataspace_Remove *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *node;

    node = List_First(&data->list);
    while (node)
    {
        if (node->id == msg->id)
        {
            Remove((struct Node *)node);
            FreeVecPooled(data->current_pool, node);
            return 1;
        }
        node = Node_Next(node);
    }

    return 0;
}

/****** Dataspace.mui/MUIM_Dataspace_ReadIFF ************
*
*   NAME
*       MUIM_Dataspace_ReadIFF
*
*   SYNOPSIS
*       LONG result = (LONG)DoMethod(obj, MUIM_Dataspace_ReadIFF, (IPTR)handle);
*
*   FUNCTION
*       Read bytes from the current IFF chunk and interpret successive records
*       as a big-endian 32-bit identifier, a big-endian 32-bit payload length,
*       and that many payload bytes. Pass each complete record to MUIM_Dataspace_Add.
*       Existing records are retained except where an imported identifier replaces
*       them. Payload bytes are copied without byte-order conversion.
*
*   INPUTS
*       handle - Valid IFF handle opened for reading and positioned in the
*                intended chunk. This method does not scan for a chunk type or ID.
*
*   RESULT
*       IFFERR_EOF if there is no current chunk; IFFERR_NOMEM if temporary
*       buffer allocation fails; a negative ReadChunkBytes() error on read failure;
*       otherwise 0, including incomplete or partially failed imports.
*
*   NOTES
*       The handle remains caller-owned. The dispatcher holds an exclusive
*       semaphore throughout reading and insertion. I/O can block.
*
*   BUGS
*       The method ignores Add failures. It stops without an error when a record
*       length exceeds the remaining bytes, and ignores fewer than eight trailing
*       bytes. It does not roll back records already imported or distinguish these
*       conditions from a complete import.
*
*   SEE ALSO
*       MUIM_Dataspace_WriteIFF, MUIM_Dataspace_Add
*
*/
IPTR Dataspace__MUIM_ReadIFF(struct IClass *cl, Object *obj,
    struct MUIP_Dataspace_ReadIFF *msg)
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
    if (!(buffer = AllocVec(cn->cn_Size, 0)))
    {
        return IFFERR_NOMEM;
    }
    read = ReadChunkBytes(msg->handle, buffer, cn->cn_Size);
    if (read < 0)
    {
        FreeVec(buffer);
        return read;
    }

    p = buffer;

    while (p + 8 <= buffer + read)
    {
        /* Since data can be stored on uneven addresses we must read
         ** them byte by byte as MC68000 doesn't like this
         */
        id = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
        p += 4;
        len = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
        p += 4;

        /* p might be uneven but MUIM_Dataspace_Add use CopyMem() */

        /* Guard against a corrupted/truncated file - do not read past
         ** the chunk if the length field is bogus */
        if (len > (ULONG)(buffer + read - p))
            break;

        /* Now add the data */
        DoMethod(obj, MUIM_Dataspace_Add, (IPTR) p, len, id);
        p += len;
    }
    FreeVec(buffer);
    return 0;
}

/****** Dataspace.mui/MUIM_Dataspace_WriteIFF ************
*
*   NAME
*       MUIM_Dataspace_WriteIFF
*
*   SYNOPSIS
*       LONG result = (LONG)DoMethod(obj, MUIM_Dataspace_WriteIFF,
*                                   (IPTR)handle, type, id);
*
*   FUNCTION
*       Push an IFF chunk, write records in current list order, then pop the
*       chunk. Each record contains a big-endian 32-bit identifier and length
*       followed by the unchanged payload bytes. No per-record padding is added
*       by Dataspace.
*
*   INPUTS
*       handle - Caller-owned IFF handle opened for writing.
*       type - Type passed unchanged to PushChunk().
*       id - Chunk identifier passed unchanged to PushChunk().
*
*   RESULT
*       0 after a successful PopChunk(); otherwise the PushChunk() error,
*       a negative WriteChunkBytes() result, or the PopChunk() error.
*
*   NOTES
*       The dispatcher holds an exclusive semaphore throughout the operation;
*       I/O can block. Internal identifier and length fields are temporarily
*       converted in place and restored after each write. Do not access the
*       dataspace concurrently outside its locking discipline.
*
*   BUGS
*       A negative record-write result returns without popping the chunk.
*       Nonnegative short writes are not checked against the requested size.
*       Failure can leave partial output; no rollback is performed.
*
*   SEE ALSO
*       MUIM_Dataspace_ReadIFF
*
*/
IPTR Dataspace__MUIM_WriteIFF(struct IClass *cl, Object *obj,
    struct MUIP_Dataspace_WriteIFF *msg)
{
    struct MUI_DataspaceData *data = INST_DATA(cl, obj);
    struct Dataspace_Node *iter;
    LONG rc;

    if ((rc = PushChunk(msg->handle, msg->type, msg->id, IFFSIZE_UNKNOWN)))
        return rc;

    iter = List_First(&data->list);
    while (iter)
    {
        ULONG len = iter->len + 8;
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
    if ((rc = PopChunk(msg->handle)))
        return rc;
    return 0;
}


/* Dispatch record methods under the inherited semaphore: shared for Find,
 * exclusive for mutations and IFF operations. Release before returning the
 * method result. Construction and disposal are dispatched without this
 * lock; unknown methods are forwarded to Semaphore.
 */
BOOPSI_DISPATCHER(IPTR, Dataspace_Dispatcher, cl, obj, msg)
{
    IPTR res;

    switch (msg->MethodID)
    {
        /* Whenever an object shall be created using NewObject(), it will be
         ** sent a OM_NEW method.
         */
    case OM_NEW:
        return Dataspace__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Dataspace__OM_DISPOSE(cl, obj, msg);
    case MUIM_Dataspace_Add:
        DoMethod(obj, MUIM_Semaphore_Obtain);
        res = Dataspace__MUIM_Add(cl, obj, (APTR) msg);
        DoMethod(obj, MUIM_Semaphore_Release);
        return res;
    case MUIM_Dataspace_Clear:
        DoMethod(obj, MUIM_Semaphore_Obtain);
        res = Dataspace__MUIM_Clear(cl, obj, (APTR) msg);
        DoMethod(obj, MUIM_Semaphore_Release);
        return res;
    case MUIM_Dataspace_Find:
        DoMethod(obj, MUIM_Semaphore_ObtainShared);
        res = Dataspace__MUIM_Find(cl, obj, (APTR) msg);
        DoMethod(obj, MUIM_Semaphore_Release);
        /* now that sem is released, imagine that the object gets freed ...
         * it really needs that the caller locks the object,
         * and release it when done with the result
         */
        return res;
    case MUIM_Dataspace_Merge:
        DoMethod(obj, MUIM_Semaphore_Obtain);
        res = Dataspace__MUIM_Merge(cl, obj, (APTR) msg);
        DoMethod(obj, MUIM_Semaphore_Release);
        return res;
    case MUIM_Dataspace_Remove:
        DoMethod(obj, MUIM_Semaphore_Obtain);
        res = Dataspace__MUIM_Remove(cl, obj, (APTR) msg);
        DoMethod(obj, MUIM_Semaphore_Release);
        return res;
    case MUIM_Dataspace_ReadIFF:
        DoMethod(obj, MUIM_Semaphore_Obtain);
        res = (IPTR) Dataspace__MUIM_ReadIFF(cl, obj, (APTR) msg);
        DoMethod(obj, MUIM_Semaphore_Release);
        return res;
    case MUIM_Dataspace_WriteIFF:
        DoMethod(obj, MUIM_Semaphore_Obtain);
        res = (IPTR) Dataspace__MUIM_WriteIFF(cl, obj, (APTR) msg);
        DoMethod(obj, MUIM_Semaphore_Release);
        return res;
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Dataspace_desc =
{
    MUIC_Dataspace,             /* Class name */
    MUIC_Semaphore,             /* super class name */
    sizeof(struct MUI_DataspaceData),   /* size of class own datas */
    (void *) Dataspace_Dispatcher        /* class dispatcher */
};
