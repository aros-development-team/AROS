/*
    Copyright © 2011-2019, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <utility/tagitem.h>

#include <hidd/gallium.h>
#include <gallium/gallium.h>

#include "intelgma_hidd.h"
#include "intelgma_gallium.h"
#include "intelgma_winsys.h"
#include "intelG45_regs.h"

const struct OOP_InterfaceDescr Gallium_ifdescr[];
extern OOP_AttrBase MetaAttrBase;
OOP_AttrBase HiddGalliumAttrBase;
#if (1)
// TODO: move to instance data ..
static struct SignalSemaphore BatchBufferLock;
static struct SignalSemaphore UnusedBuffersListLock;
static ULONG allocated_mem=0;
static struct status hw_status[1024];
static ULONG flush_num;
static struct List unusedbuffers;
#endif

extern struct g45staticdata sd;
#define sd ((struct g45staticdata*)&(sd))

// ****************************************************************************
//                                                                CLASS init
// ****************************************************************************

BOOL InitGalliumClass()
{
    int i;

    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    if( sd->force_gallium
        || sd->ProductID == 0x2582 // GMA 900
        || sd->ProductID == 0x2782
        || sd->ProductID == 0x2592
        || sd->ProductID == 0x2792
        || sd->ProductID == 0x2772 // GMA 950
        || sd->ProductID == 0x2776
        || sd->ProductID == 0x27A2
        || sd->ProductID == 0x27A6
        || sd->ProductID == 0x27AE
        || sd->ProductID == 0x2972 // GMA 3000
        || sd->ProductID == 0x2973
        || sd->ProductID == 0x2992
        || sd->ProductID == 0x2993
    ){
        CloseLibrary(OpenLibrary("gallium.library", 0));

        if((HiddGalliumAttrBase = OOP_ObtainAttrBase(IID_Hidd_Gallium)))
        {
            struct TagItem Gallium_tags[] =
            {
                { aMeta_SuperID,        (IPTR)CLID_Hidd_Gallium },
                { aMeta_InterfaceDescr, (IPTR)Gallium_ifdescr },
                { aMeta_InstSize,       sizeof(struct HiddIntelGMAGalliumData)},
                { aMeta_ID,             (IPTR)CLID_Hidd_Gallium_IntelGMA},
                { TAG_DONE, 0}
            };

            D(bug("[IntelGMA:Gallium] %s: gallium.hidd attrbase obtained\n", __func__));

            sd->galliumclass = OOP_NewObject(NULL, CLID_HiddMeta, Gallium_tags);
            if (sd->galliumclass)
            {
                D(bug("[IntelGMA:Gallium] %s: %s class @ 0x%p\n", __func__, CLID_Hidd_Gallium_IntelGMA, sd->galliumclass));

                sd->galliumclass->UserData = sd;
                OOP_AddClass(sd->galliumclass);

                D(bug("[IntelGMA:Gallium] %s: class registered\n", __func__));

                sd->basegallium = OOP_FindClass(CLID_Hidd_Gallium);
                bug("[IntelGMA] base Gallium class @ 0x%p\n", sd->basegallium);

                for(i=100;i<1024;i++)
                {
                    hw_status[i].reserved = FALSE;
                }

                NEWLIST(&unusedbuffers);
                InitSemaphore(&BatchBufferLock);
                InitSemaphore(&UnusedBuffersListLock);

                return TRUE;
            }
            OOP_ReleaseAttrBase(IID_Hidd_Gallium);
        }
    }

    bug("[IntelGMA:Gallium] failed to initialise gallium subsystem\n");

    return FALSE;
}

// ****************************************************************************
//                                               winsys/support functions
// ****************************************************************************

static void IntelGMA_BatchBufferReset(struct IntelGMABatchBuffer *batch)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    memset(batch->base.map, 0, batch->actual_size);
    batch->base.ptr = batch->base.map;
    batch->base.size = batch->actual_size - BATCH_RESERVED;
    batch->base.relocs = 0;
}

static BOOL IntelGMA_StatusGet(ULONG i)
{
#ifdef GALLIUM_SIMULATION
 return 0;
#endif
    if( ! hw_status[i].reserved ) bug("[IntelGMA:Gallium] ERROR: IntelGMA_StatusGet - index not reserved %d\n",i);
    return readl( &sd->HardwareStatusPage[ i ]);
}

static VOID IntelGMA_StatusSet(ULONG i, ULONG v)
{
#ifndef GALLIUM_SIMULATION
    if( ! hw_status[i].reserved ) bug("[IntelGMA:Gallium] ERROR: IntelGMA_StatusSet - index not reserved %d\n",i);
    writel( v, &sd->HardwareStatusPage[ i ] );
    readl( &sd->HardwareStatusPage[ i ] );
#endif
}

static ULONG IntelGMA_StatusReserve()
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    for(;;){
        int i;

        for(i=100;i<1024;i++)
        {
            if( ! hw_status[i].reserved )
            {
                hw_status[i].reserved = TRUE;
                hw_status[i].flush_num = flush_num;

                D(bug("[IntelGMA:Gallium] %s: index #%d, flush_num #%d\n", __func__, i, flush_num));

                return i;
            }
        }

        D(bug("[IntelGMA:Gallium] %s: not free index,wait a moment...\n",__func__));

        delay_ms(sd,1);
    }
    return 0;
}

static VOID IntelGMA_StatusFree(ULONG i)
{
    D(bug("[IntelGMA:Gallium] %s(%d)\n", i));

    hw_status[i].reserved = FALSE;
    hw_status[i].flush_num = 0; 
}

static VOID IntelGMA_GfxMemFree(APTR ptr, ULONG size)
{
    D(bug("[IntelGMA:Gallium] %s(%p, %d)\n", __func__, ptr, size));

#ifdef GALLIUM_SIMULATION
    free(ptr);return;
#endif

    FreeGfxMem(sd, ptr, size);
    allocated_mem -= size;

    D(bug("[IntelGMA:Gallium] %s: allocated_mem %d\n", __func__, allocated_mem));
}

static VOID IntelGMA_ReleaseBuffers(struct i915_winsys *iws)
{
    D(bug("[IntelGMA:Gallium] %s: allocated_mem %d\n", __func__, allocated_mem));

    if( AttemptSemaphore(&UnusedBuffersListLock) )
    {
        struct Node *node,*next;
        struct i915_winsys_buffer *buf;
        int i=0;

        ForeachNodeSafe(&unusedbuffers,node,next)
        {
            i++;
            buf = (struct i915_winsys_buffer *)node;

            if( ! iws->buffer_is_busy(0, buf ) )
            {
                i--;
                D(bug("[IntelGMA:Gallium]     destroy %p\n",buf));
                Remove(node);
                buf->magic = 0;
                IntelGMA_GfxMemFree(buf->allocated_map, buf->allocated_size);
                FREE(buf);
            }
        }
        D(if(i) bug("[IntelGMA:Gallium] unused_buffers left:%d\n",i));
        ReleaseSemaphore(&UnusedBuffersListLock);
    }
}

static APTR IntelGMA_GfxMemAlloc(struct i915_winsys *iws, ULONG size)
{
    APTR result;

    D(bug("[IntelGMA:Gallium] %s(%d)\n", __func__, size));

#ifdef GALLIUM_SIMULATION
    return malloc(size);
#endif

    result = AllocGfxMem(sd, size);

    if (result == 0)
    {
        LOCK_HW
        {
            DO_FLUSH();
            WAIT_IDLE();
        }
        UNLOCK_HW

        IntelGMA_ReleaseBuffers(iws);

        result = AllocGfxMem(sd, size);
    }

    if (result)
    {
        allocated_mem += size;

        D(bug("[IntelGMA:Gallium] %: %p allocated_mem %d\n", __func__, result, allocated_mem));
    }

    return result;
}

static struct i915_winsys_batchbuffer *IntelGMA_WS_BatchbufferCreate(struct i915_winsys *iws)
{
    struct IntelGMABatchBuffer *batch;

    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    batch = CALLOC_STRUCT(IntelGMABatchBuffer);

    if (batch)
    {
        batch->iws = iws;
        batch->actual_size = 16 * 4096;
        batch->base.map = MALLOC(batch->actual_size);
        batch->allocated_size = batch->actual_size + 4096;

        if( !(batch->allocated_map = IntelGMA_GfxMemAlloc(iws, batch->allocated_size) ) )
            return NULL;

        batch->gfxmap = (APTR)(((IPTR)batch->allocated_map + 4095) & ~4095);

        batch->base.ptr = NULL;
        batch->base.size = 0;

        batch->base.relocs = 0;
        batch->base.iws = iws;

        IntelGMA_BatchBufferReset(batch);
    }
    return (struct i915_winsys_batchbuffer *)batch;
}

static  boolean IntelGMA_WS_ValidateBuffers(struct i915_winsys_batchbuffer *batch,
                           struct i915_winsys_buffer **buffers,
                           int num_of_buffers)
{
    int i;

    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    for(i=0;i<num_of_buffers;i++)
    {
        D(bug("[IntelGMA:Gallium] %s:    buffer %p\n", __func__, buffers[i]));
        MAGIC_WARNING(buffers[i]);
    }

     return TRUE;
}

static int IntelGMA_WS_BatchbufferReloc(struct i915_winsys_batchbuffer *batch,
                        struct i915_winsys_buffer *reloc,
                        enum i915_winsys_buffer_usage usage,
                        unsigned offset, boolean fenced)
{
    struct reloc *rl;

    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    IF_BAD_MAGIC(reloc) return -1;

    rl = &INTELGMA_BB(batch)->relocs[ batch->relocs ];
    batch->relocs++;

    if( batch->relocs >= MAX_RELOCS )
    {
        bug("[IntelGMA:Gallium] MAX_RELOCS ERROR\n");
        *(uint32_t *)(batch->ptr) = 0;
        batch->ptr += 4;
        return -1;
    }

    rl->buf = reloc;
    rl->usage = usage;
    rl->offset = offset;
    rl->ptr = (uint32_t *)batch->ptr;
    *(uint32_t *)(batch->ptr) = RELOC_MAGIC;
    batch->ptr += 4;

    D(bug("[IntelGMA:Gallium] %s: reloc %p offset %d fenced %s base=%p \n", __func__, reloc, offset, fenced ? "true" : "false", *(uint32_t *)(batch->ptr)));

    return 0;
}

static void IntelGMA_WS_BatchbufferFlush(struct i915_winsys_batchbuffer *batch,
                         struct pipe_fence_handle **fence,
                         enum i915_winsys_flush_flags flags)
{
    struct IntelGMABatchBuffer *gmabatch;

    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    gmabatch = INTELGMA_BB(batch);

    if( (batch->ptr - batch->map) & 4) {
        *(uint32_t *)(batch->ptr) = 0; /* MI_NOOP */
        batch->ptr += 4;
    }

    *(uint32_t *)(batch->ptr) = MI_BATCH_BUFFER_END;
    batch->ptr += 4;

#ifdef GALLIUM_SIMULATION
    IntelGMA_BatchBufferReset( gmabatch );
    return;
#endif

    LOCK_BB
    {
        flush_num++;

        if(flush_num==0)flush_num=1;

        ULONG index = IntelGMA_StatusReserve();

        // relocations
        ObtainSemaphore(&UnusedBuffersListLock);
            int i;
            for(i=0;i<batch->relocs;i++)
            {
                struct reloc *rl = &gmabatch->relocs[ i ];

                D(bug("[IntelGMA:Gallium] %s: reloc %p\n", __func__, rl->buf));
                
                if( i >= MAX_RELOCS || 
                    rl->buf->map == 0 || 
                    rl->buf->magic != MAGIC ||
                    *(uint32_t *)(rl->ptr) != RELOC_MAGIC 
                  )
                {
                    IntelGMA_BatchBufferReset( gmabatch );
                    UNLOCK_BB
                    ReleaseSemaphore(&UnusedBuffersListLock);

                    bug("[IntelGMA:Gallium] ERROR: Rotten reloc, num %d buf %p map %p\n", i, rl->buf, rl->buf->map);

                    return;
                }

                if( rl->buf->flush_num != flush_num )
                {
                    while( gmabatch->base.iws->buffer_is_busy(0, rl->buf) ){}
                }

                rl->buf->flush_num = flush_num;
                rl->buf->status_index = index;           
                
                *(uint32_t *)(rl->ptr) = BASEADDRESS( rl->buf->map ) + rl->offset;
            }
        ReleaseSemaphore(&UnusedBuffersListLock);
        
        // copy to gfxmem
        memcpy(gmabatch->gfxmap, batch->map, batch->ptr - batch->map );
        
        LOCK_HW
        {
            // set status
            IntelGMA_StatusSet( index , 1 );

            //run
            START_RING(6);
            {
                // flush
                ULONG cmd = MI_FLUSH | MI_NO_WRITE_FLUSH;
                cmd &= ~MI_NO_WRITE_FLUSH;
                cmd |= MI_READ_FLUSH;
                cmd |= MI_EXE_FLUSH;
                //OUT_RING(cmd);
                //OUT_RING(0);
 
                // start batchbuffer
                OUT_RING( MI_BATCH_BUFFER_START | (2 << 6) );
                OUT_RING( BASEADDRESS( gmabatch->gfxmap ) | MI_BATCH_NON_SECURE);

             //   OUT_RING(cmd);
              //  OUT_RING(0);
                
                // clean status
                OUT_RING((0x21 << 23) | 1);
                OUT_RING( index << 2 );
                OUT_RING(0);
                OUT_RING(0);
            }  
            ADVANCE_RING();
        }    
        UNLOCK_HW

        IntelGMA_BatchBufferReset( gmabatch );
    }
    UNLOCK_BB
}

static void IntelGMA_WS_BatchbufferDestroy(struct i915_winsys_batchbuffer *batch)
{
    struct IntelGMABatchBuffer *gmabatch;

    bug("[IntelGMA:Gallium] %s()\n", __func__);

    gmabatch = INTELGMA_BB(batch);

    FREE(batch->map);
    IntelGMA_GfxMemFree(gmabatch->allocated_map, gmabatch->allocated_size);
    FREE(gmabatch);

    LOCK_HW
    {
        DO_FLUSH();
        WAIT_IDLE();
        ObtainSemaphore(&UnusedBuffersListLock);

        IntelGMA_ReleaseBuffers(gmabatch->iws);

        ReleaseSemaphore(&UnusedBuffersListLock);
    }
    UNLOCK_HW
}

static struct i915_winsys_buffer * IntelGMA_WS_BufferCreate(struct i915_winsys *iws,
                   unsigned size,
                   enum i915_winsys_buffer_type type)
{
    struct i915_winsys_buffer *buf;

    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    buf = CALLOC_STRUCT(i915_winsys_buffer);
    if (!buf)
    return NULL;

    // allocate page aligned gfx memory
    buf->allocated_size = size + 4096;
    if( !(buf->allocated_map = IntelGMA_GfxMemAlloc(iws, buf->allocated_size) ) )
    {
        FREE(buf);
        return NULL;
    }
    buf->map = (APTR)(((IPTR)buf->allocated_map + 4095)& ~4095);
    buf->size = size;
    buf->magic = MAGIC;

    D(bug("[IntelGMA:Gallium] %s: size %d @ 0x%p (map 0x%p)\n", __func__, size, buf, buf->map));
//    D(bug("[IntelGMA:Gallium] %s: type %s\n", __func__, i915_type_to_name(type)));

    return buf;
}

static struct i915_winsys_buffer * IntelGMA_WS_BufferCreateTiled(struct i915_winsys *iws,
                         unsigned *stride, unsigned height,
                         enum i915_winsys_buffer_tile *tiling,
                         enum i915_winsys_buffer_type type)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    *tiling = I915_TILE_NONE;

    return iws->buffer_create( iws, *stride * height, type );
}

static struct i915_winsys_buffer * IntelGMA_WS_BufferFromHandle(struct i915_winsys *iws,
                        struct winsys_handle *whandle,
                        unsigned height,
                        enum i915_winsys_buffer_tile *tiling,
                        unsigned *stride)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    return NULL;
}

static boolean IntelGMA_WS_BufferGetHandle(struct i915_winsys *iws,
                            struct i915_winsys_buffer *buffer,
                            struct winsys_handle *whandle,
                            unsigned stride)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    return FALSE;
}

static void * IntelGMA_WS_BufferMap(struct i915_winsys *iws,
                   struct i915_winsys_buffer *buffer,
                   boolean write)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    IF_BAD_MAGIC(buffer)
    {
        //for(;;);
        return 0;
    }

    D(bug("[IntelGMA:Gallium] %s: buffer @ 0x%p\n", __func__, buffer));

    while( iws->buffer_is_busy(iws, buffer )){};

    return buffer->map;
}


static void IntelGMA_WS_BufferUnmap(struct i915_winsys *iws,
                    struct i915_winsys_buffer *buffer)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));
}

static int IntelGMA_WS_BufferWrite(struct i915_winsys *iws,
                   struct i915_winsys_buffer *dst,
                   size_t offset,
                   size_t size,
                   const void *wdata)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    return 0;
}

static void IntelGMA_WS_BufferDestroy(struct i915_winsys *iws,
                      struct i915_winsys_buffer *buffer)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));
    
    IF_BAD_MAGIC(buffer) return;

    D(bug("[IntelGMA:Gallium] %s: buffer @ 0x%p\n", __func__, buffer));

    if (0)
    {
        while( iws->buffer_is_busy(0, buffer ) ){};
        buffer->magic = 0;
        IntelGMA_GfxMemFree(buffer->allocated_map, buffer->allocated_size);
        FREE(buffer);
    }
    else
    {
        ObtainSemaphore(&UnusedBuffersListLock);
        AddTail( &unusedbuffers, (struct Node *)buffer );
        ReleaseSemaphore(&UnusedBuffersListLock);
    }
}

static boolean IntelGMA_WS_BufferIsBusy(struct i915_winsys *iws,
                         struct i915_winsys_buffer *buffer)
{
    int i;

    D(bug("[IntelGMA:Gallium] %s()\n", __func__));
    
    MAGIC_WARNING(buffer);
    i = buffer->status_index;

    D(bug("[IntelGMA:Gallium] %s: %p index %d flush_num %d\n", __func__, buffer, i, buffer->flush_num));

    if (i)
    {
        if( hw_status[i].reserved &&
            hw_status[i].flush_num == buffer->flush_num )
        {
            if( IntelGMA_StatusGet(i) )
            {
                return TRUE;
            }
            else
            {
                buffer->status_index = 0;
                IntelGMA_StatusFree(i);
            }
        }
    }
    return FALSE;
}

static void IntelGMA_WS_FenceReference(struct i915_winsys *iws,
                       struct pipe_fence_handle **ptr,
                       struct pipe_fence_handle *fence)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));
}

static int IntelGMA_WS_FenceSignalled(struct i915_winsys *iws,
                      struct pipe_fence_handle *fence)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    return 0;
}

static int IntelGMA_WS_FenceFinish(struct i915_winsys *iws,
                   struct pipe_fence_handle *fence)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    return 0;
}

static int IntelGMA_WS_ApertureSize(struct i915_winsys *iws)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    return allocated_mem;
}

// ****************************************************************************
//                                                                Gallium Hidd Methods
// ****************************************************************************

OOP_Object *METHOD(GalliumIntelGMA, Root, New)
{
    IPTR interfaceVers;

    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HiddIntelGMAGalliumData * data = OOP_INST_DATA(cl, o);

        data->gma_obj = o;
        data->gma_winsys.pci_id = sd->ProductID;

        data->gma_winsys.destroy                    = NULL;
        data->gma_winsys.batchbuffer_create         = IntelGMA_WS_BatchbufferCreate;
        data->gma_winsys.validate_buffers           = IntelGMA_WS_ValidateBuffers;
        data->gma_winsys.batchbuffer_reloc          = IntelGMA_WS_BatchbufferReloc;
        data->gma_winsys.batchbuffer_flush          = IntelGMA_WS_BatchbufferFlush;
        data->gma_winsys.batchbuffer_destroy        = IntelGMA_WS_BatchbufferDestroy;
        data->gma_winsys.buffer_create              = IntelGMA_WS_BufferCreate;
        data->gma_winsys.buffer_create_tiled        = IntelGMA_WS_BufferCreateTiled;
        data->gma_winsys.buffer_from_handle         = IntelGMA_WS_BufferFromHandle;
        data->gma_winsys.buffer_get_handle          = IntelGMA_WS_BufferGetHandle;
        data->gma_winsys.buffer_map                 = IntelGMA_WS_BufferMap;
        data->gma_winsys.buffer_unmap               = IntelGMA_WS_BufferUnmap;
        data->gma_winsys.buffer_write               = IntelGMA_WS_BufferWrite;
        data->gma_winsys.buffer_destroy             = IntelGMA_WS_BufferDestroy;
        data->gma_winsys.buffer_is_busy             = IntelGMA_WS_BufferIsBusy;
        data->gma_winsys.fence_reference            = IntelGMA_WS_FenceReference;
        data->gma_winsys.fence_signalled            = IntelGMA_WS_FenceSignalled;
        data->gma_winsys.fence_finish               = IntelGMA_WS_FenceFinish;
        data->gma_winsys.aperture_size              = IntelGMA_WS_ApertureSize;
    }

    return o;
}

VOID METHOD(GalliumIntelGMA, Root, Dispose)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(GalliumIntelGMA, Root, Get)
{
    ULONG idx;

    D(bug("[IntelGMA:Gallium] %s\n", __func__));

    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            /* Overload the property */
            case aoHidd_Gallium_InterfaceVersion:
                *msg->storage = GALLIUM_INTERFACE_VERSION;
                return;
        }
    }

    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(GalliumIntelGMA, Hidd_Gallium, CreatePipeScreen)
{
    struct HiddIntelGMAGalliumData * data = OOP_INST_DATA(cl, o);
    struct pipe_screen *screen = NULL;

    D(bug("[IntelGMA:Gallium] %s()\n", __func__));
#if (0)
    D(bug("[IntelGMA:Gallium] %s: currently allocated_mem %d\n", __func__, allocated_mem));
#endif

    screen = i915_screen_create(&data->gma_winsys);

    D(bug("[IntelGMA:Gallium] %s: screen @ 0x%p\n", __func__, screen));

    return screen;
}

VOID METHOD(GalliumIntelGMA, Hidd_Gallium, DestroyPipeScreen)
{
    D(bug("[IntelGMA:Gallium] %s()\n", __func__));
}

VOID METHOD(GalliumIntelGMA, Hidd_Gallium, DisplayResource)
{
    struct HiddIntelGMAGalliumData * data = OOP_INST_DATA(cl, o);

    D(bug("[IntelGMA:Gallium] %s()\n", __func__));

#ifndef GALLIUM_SIMULATION
    OOP_Object *bm = HIDD_BM_OBJ(msg->bitmap);
    GMABitMap_t *bm_dst;

   // if (!((IPTR)bm == (IPTR)sd.BMClass) ) return; // Check if bitmap is really intelGFX bitmap

    bm_dst = OOP_INST_DATA(OOP_OCLASS(bm), bm);
    struct i915_texture *tex = i915_texture(msg->resource);

    IF_BAD_MAGIC(tex->buffer) return;

    LOCK_BITMAP_BM(bm_dst)
    {
        uint32_t br00, br13, br22, br23, br09, br11, br26, br12;
        int mode = 3;

        br00 = (2 << 29) | (0x53 << 22) | (6);
        if (bm_dst->bpp == 4)
            br00 |= 3 << 20;

        br13 = bm_dst->pitch | ROP_table[mode].rop;
        if (bm_dst->bpp == 4)
            br13 |= 3 << 24;
        else if (bm_dst->bpp == 2)
            br13 |= 1 << 24;

        br22 = msg->dstx | (msg->dsty << 16);
        br23 = (msg->dstx + msg->width) | (msg->dsty + msg->height) << 16;
        br09 = bm_dst->framebuffer;

        br11 = tex->stride;
        br26 = msg->srcx | (msg->srcy << 16);
        br12 = (IPTR)tex->buffer->map - (IPTR)sd->Card.Framebuffer;

        while(data->gma_winsys.buffer_is_busy(0, tex->buffer)){};

        LOCK_HW
        {
            START_RING(8);
            {
                OUT_RING(br00);
                OUT_RING(br13);
                OUT_RING(br22);
                OUT_RING(br23);
                OUT_RING(br09);
                OUT_RING(br26);
                OUT_RING(br11);
                OUT_RING(br12);
            }
            ADVANCE_RING();
            //DO_FLUSH();
        }
        UNLOCK_HW
    }
    UNLOCK_BITMAP_BM(bm_dst)
#endif

    IntelGMA_ReleaseBuffers(&data->gma_winsys);
}

static const struct OOP_MethodDescr Gallium_Root_descr[] =
{
    {(OOP_MethodFunc)GalliumIntelGMA__Root__New,                        moRoot_New                                                      },
    {(OOP_MethodFunc)GalliumIntelGMA__Root__Dispose,                    moRoot_Dispose                                                  },
    {(OOP_MethodFunc)GalliumIntelGMA__Root__Get,                        moRoot_Get                                                      },
    {NULL,                                                              0                                                               }
};
#define NUM_IntelGMAGallium_Root_METHODS 3

static const struct OOP_MethodDescr Gallium_Hidd_Gallium_descr[] =
{
    {(OOP_MethodFunc)GalliumIntelGMA__Hidd_Gallium__CreatePipeScreen,   moHidd_Gallium_CreatePipeScreen                                 },
    {(OOP_MethodFunc)GalliumIntelGMA__Hidd_Gallium__DisplayResource,    moHidd_Gallium_DisplayResource                                  },
    {NULL,                                                              0                                                               }
};
#define NUM_IntelGMAGallium_Gallium_METHODS 2

const struct OOP_InterfaceDescr Gallium_ifdescr[] =
{
    {(struct OOP_MethodDescr*)Gallium_Root_descr,                       IID_Root,               NUM_IntelGMAGallium_Root_METHODS        },
    {(struct OOP_MethodDescr*)Gallium_Hidd_Gallium_descr,               IID_Hidd_Gallium,       NUM_IntelGMAGallium_Gallium_METHODS     },
    {NULL,                                                              NULL,                   0                                       }
};
