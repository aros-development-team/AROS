/*
    Copyright © 2011-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>

#include "intelG45_intern.h"
#include "intelG45_regs.h"
#include "aros_winsys.h"

#include "util/u_memory.h"
#include "i915/i915_winsys.h"
#include "i915/i915_debug.h"

#include "i915/i915_reg.h"


#ifdef GALLIUM_SIMULATION
    #undef LOCK_HW
    #undef START_RING
    #undef OUT_RING
    #undef ADVANCE_RING
    #undef WAIT_IDLE
    #undef DO_FLUSH
    #undef UNLOCK_HW
    #define LOCK_HW
    #define START_RING(x)
    #define OUT_RING(x)
    #define ADVANCE_RING()
    #define WAIT_IDLE()
    #define DO_FLUSH()
    #define UNLOCK_HW
    #warning GALLIUM_SIMULATION MODE!
#endif

#define IMPLEMENT() bug("[GMA winsys]------IMPLEMENT(%s)\n", __func__)

struct status
{
    BOOL reserved;
    ULONG flush_num;
};

static struct status hw_status[1024];
//static APTR *bb_map;
static ULONG flush_num;
ULONG allocated_mem=0;
static struct SignalSemaphore BatchBufferLock;
static struct SignalSemaphore UnusedBuffersListLock;
struct List unusedbuffers;

extern struct g45staticdata sd;
#define sd ((struct g45staticdata*)&(sd))
#define LOCK_BB          { ObtainSemaphore(&BatchBufferLock); }
#define UNLOCK_BB        { ReleaseSemaphore(&BatchBufferLock); }

#define BASEADDRESS(p) ((IPTR)(p) - (IPTR)sd->Card.Framebuffer)
struct i915_winsys_batchbuffer *batchbuffer_create(struct i915_winsys *iws);

ULONG reserve_status_index()
{
    for(;;){
        int i;
        for(i=100;i<1024;i++)
        {
            if( ! hw_status[i].reserved )
            {
                hw_status[i].reserved = TRUE;
                hw_status[i].flush_num = flush_num;
                D(bug("[GMA winsys] reserve_status_index=%d flush_num %d\n",i,flush_num));
                return i;
            }
        }
        (bug("[GMA winsys] reserve_status_index: not free index,wait a moment...\n"));
        delay_ms(sd,1);
    }
    return 0;
}

VOID free_status_index(ULONG i)
{
    hw_status[i].reserved = FALSE;
    hw_status[i].flush_num = 0; 
    D(bug("[GMA winsys] free_status_index(%d)\n",i));
}

BOOL get_status(ULONG i)
{
#ifdef GALLIUM_SIMULATION
 return 0;
#endif
    if( ! hw_status[i].reserved ) bug("[GMA winsys] get_status ERROR,index not reserved %d\n",i);
    return readl( &sd->HardwareStatusPage[ i ]);
}

VOID set_status(ULONG i,ULONG v)
{
#ifndef GALLIUM_SIMULATION
    if( ! hw_status[i].reserved ) bug("[GMA winsys] set_status ERROR,index not reserved %d\n",i);
    writel( v, &sd->HardwareStatusPage[ i ] );
    readl( &sd->HardwareStatusPage[ i ] );
#endif
}

APTR alloc_gfx_mem(ULONG size)
{
    APTR result;

#ifdef GALLIUM_SIMULATION
    return malloc(size);
#endif

    result = AllocGfxMem(sd, size);

    if( result == 0 )
    {
        LOCK_HW
            DO_FLUSH();
            WAIT_IDLE();
        UNLOCK_HW
        destroy_unused_buffers();

        result = AllocGfxMem(sd, size);
    }

    if(result)
    {
        memset(result, 0, size);
        allocated_mem+=size;
        D(bug("[GMA winsys] alloc_gfx_mem(%d) = %p allocated_mem %d\n",
            size, result, allocated_mem));
    }

    return result;
}

VOID free_gfx_mem(APTR ptr, ULONG size)
{
#ifdef GALLIUM_SIMULATION
    free(ptr);return;
#endif

    FreeGfxMem(sd, ptr, size);
    allocated_mem-=size;
    D(bug("[GMA winsys] free_gfx_mem(%p, %d) allocated_mem %d\n", ptr, size, allocated_mem));
}

VOID init_aros_winsys()
{
    // clean hw_status table,reserve first 100,( 0-15 is reserved,and bitmapclass uses at least 16-20)
    int i;
    for(i=100;i<1024;i++)
    {
        hw_status[i].reserved = FALSE;
    }

    NEWLIST(&unusedbuffers);
    InitSemaphore(&BatchBufferLock);
    InitSemaphore(&UnusedBuffersListLock);
}

/*******************************************************************
* Batchbuffer functions.
******************************************************************/

#define BATCH_RESERVED 16

static void batchbuffer_reset(struct aros_batchbuffer *batch)
{

    D(bug("[GMA winsys] batchbuffer_reset\n"));
    memset(batch->base.map, 0, batch->actual_size);
    batch->base.ptr = batch->base.map;
    batch->base.size = batch->actual_size - BATCH_RESERVED;
    batch->base.relocs = 0;
}

/**
* Create a new batchbuffer.
*/
struct i915_winsys_batchbuffer *batchbuffer_create(struct i915_winsys *iws)
{
    D(bug("[GMA winsys] batchbuffer_create\n"));
    struct aros_winsys *idws = aros_winsys(iws);
    struct aros_batchbuffer *batch = CALLOC_STRUCT(aros_batchbuffer);

    batch->actual_size = idws->max_batch_size;
    batch->base.map = MALLOC(batch->actual_size);
    
    batch->allocated_size = batch->actual_size + 4096;
    if( !(batch->allocated_map = alloc_gfx_mem(batch->allocated_size) ) )
        return NULL;
    batch->gfxmap = (APTR)(((IPTR)batch->allocated_map + 4095)& ~4095);
    
    batch->base.ptr = NULL;
    batch->base.size = 0;

    batch->base.relocs = 0;
    batch->base.iws = iws;

    batchbuffer_reset(batch);

    return &batch->base;
}

/**
* Validate buffers for usage in this batchbuffer.
* Does space-checking and assorted other book-keeping.
*
* @batch
* @buffers array to buffers to validate
* @num_of_buffers size of the passed array
*/
boolean batchbuffer_validate_buffers(struct i915_winsys_batchbuffer *batch,
                   struct i915_winsys_buffer **buffers,
                   int num_of_buffers)
{
    D(bug("[GMA winsys] batchbuffer_validate_buffers\n"));
    int i;
    for(i=0;i<num_of_buffers;i++)
    {
        D(bug("    buffer %p\n",buffers[i]));
        MAGIC_WARNING(buffers[i]);
    }

   //  IMPLEMENT();
     return TRUE;
}

/**
* Emit a relocation to a buffer.
* Target position in batchbuffer is the same as ptr.
*
* @batch
* @reloc buffer address to be inserted into target.
* @usage how is the hardware going to use the buffer.
* @offset add this to the reloc buffers address
* @target buffer where to write the address, null for batchbuffer.
* @fenced relocation needs a fence.
*/
int batchbuffer_reloc(struct i915_winsys_batchbuffer *batch,
                        struct i915_winsys_buffer *reloc,
                        enum i915_winsys_buffer_usage usage,
                        unsigned offset, boolean fenced)
{
    IF_BAD_MAGIC(reloc) return -1;
    
    struct reloc *rl = &aros_batchbuffer(batch)->relocs[ batch->relocs ];
    batch->relocs++;
    
    if( batch->relocs >= MAX_RELOCS )
    {
        bug("[GMA winsys] MAX_RELOCS ERROR\n");
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
    D(bug("[GMA winsys] batchbuffer_reloc reloc %p offset %d fenced %s base=%p \n",reloc,offset,fenced ? "true" : "false",*(uint32_t *)(batch->ptr)));

    return 0;
}

/**
* Flush a bufferbatch.
*/

#define MI_BATCH_NON_SECURE     (1)

void batchbuffer_flush(struct i915_winsys_batchbuffer *batch,
                             struct pipe_fence_handle **fence)
{
    D(bug("[GMA winsys] batchbuffer_flush size=%d\n",batch->ptr - batch->map));
    struct aros_batchbuffer *abatch = aros_batchbuffer(batch);

    if( (batch->ptr - batch->map) & 4) {
        *(uint32_t *)(batch->ptr) = 0; /* MI_NOOP */
        batch->ptr += 4;
    }

    *(uint32_t *)(batch->ptr) = MI_BATCH_BUFFER_END;
    batch->ptr += 4;
//  i915_dump_batchbuffer( batch );

#ifdef GALLIUM_SIMULATION
    batchbuffer_reset( abatch );
    return;
#endif

    LOCK_BB
    
        flush_num++;if(flush_num==0)flush_num=1;
        ULONG index = reserve_status_index();

        // relocations
        ObtainSemaphore(&UnusedBuffersListLock);
            int i;
            for(i=0;i<batch->relocs;i++)
            {
                struct reloc *rl = &abatch->relocs[ i ];
                D(bug("[GMA winsys] batchbuffer_flush reloc %p\n",rl->buf));
                
                if( i >= MAX_RELOCS || 
                    rl->buf->map == 0 || 
                    rl->buf->magic != MAGIC ||
                    *(uint32_t *)(rl->ptr) != RELOC_MAGIC 
                  )
                {
                    batchbuffer_reset( abatch );
                    UNLOCK_BB
                    ReleaseSemaphore(&UnusedBuffersListLock);
                    bug("[GMA winsys] batchbuffer_flush ERROR:Rotten reloc, num %d buf %p map %p\n",i,rl->buf,rl->buf->map);
                    return;
                }
                
                if( rl->buf->flush_num != flush_num )
                {
                    while( buffer_is_busy(0, rl->buf) ){}
                }
            
                rl->buf->flush_num = flush_num;
                rl->buf->status_index = index;           
                
                *(uint32_t *)(rl->ptr) = BASEADDRESS( rl->buf->map ) + rl->offset;
            }
        ReleaseSemaphore(&UnusedBuffersListLock);
        
        // copy to gfxmem
        memcpy(abatch->gfxmap, batch->map, batch->ptr - batch->map );
        
        LOCK_HW
        
            // set status
            set_status( index , 1 );

            //run
            START_RING(6);
            
                // flush
                ULONG cmd = MI_FLUSH | MI_NO_WRITE_FLUSH;
                cmd &= ~MI_NO_WRITE_FLUSH;
                cmd |= MI_READ_FLUSH;
                cmd |= MI_EXE_FLUSH;
                //OUT_RING(cmd);
                //OUT_RING(0);
 
                // start batchbuffer
                OUT_RING( MI_BATCH_BUFFER_START | (2 << 6) );
                OUT_RING( BASEADDRESS( abatch->gfxmap ) | MI_BATCH_NON_SECURE);

             //   OUT_RING(cmd);
              //  OUT_RING(0);
                
                // clean status
                OUT_RING((0x21 << 23) | 1);
                OUT_RING( index << 2 );
                OUT_RING(0);
                OUT_RING(0);
                
            ADVANCE_RING();
            
        UNLOCK_HW

        batchbuffer_reset( abatch );

    UNLOCK_BB
}


/**
* Destroy a batchbuffer.
*/
void batchbuffer_destroy(struct i915_winsys_batchbuffer *ibatch)
{
    (bug("[GMA winsys] batchbuffer_destroy %p\n",ibatch));
    struct aros_batchbuffer *batch = aros_batchbuffer(ibatch);
    FREE(ibatch->map);
    FREE(batch);
    free_gfx_mem(batch->allocated_map, batch->allocated_size);

    LOCK_HW
        DO_FLUSH();
        WAIT_IDLE();
        ObtainSemaphore(&UnusedBuffersListLock);
            destroy_unused_buffers();
        ReleaseSemaphore(&UnusedBuffersListLock);
    UNLOCK_HW
}

/*******************************************************************
* Buffer functions.
*****************************************************************/
D(
    static char *i915_type_to_name(enum i915_winsys_buffer_type type)
    {
       char *name;
       if (type == I915_NEW_TEXTURE) {
          name = "gallium3d_texture";
       } else if (type == I915_NEW_VERTEX) {
          name = "gallium3d_vertex";
       } else if (type == I915_NEW_SCANOUT) {
          name = "gallium3d_scanout";
       } else {
          name = "gallium3d_unknown";
       }
       return name;
    }
)

void destroy_unused_buffers()
{
    
    D(bug("[GMA winsys] destroy_unused_buffers allocated_mem %d\n",allocated_mem));
    if( AttemptSemaphore(&UnusedBuffersListLock) )
    {
        struct Node *node,*next;
        struct i915_winsys_buffer *buf;
        int i=0;
        ForeachNodeSafe(&unusedbuffers,node,next)
        {
            i++;
            buf = (struct i915_winsys_buffer *)node;
            
            if( ! buffer_is_busy(0, buf ) )
            {
                i--;
                D(bug("[GMA winsys]     destroy %p\n",buf));
                Remove(node);
                buf->magic = 0;
                free_gfx_mem(buf->allocated_map, buf->allocated_size);
                FREE(buf);
            }
            
        }
        D(if(i) bug("[GMA winsys] unused_buffers left:%d\n",i));
        ReleaseSemaphore(&UnusedBuffersListLock);
    }
}


/**
* Create a buffer.
*/
struct i915_winsys_buffer *
      buffer_create(struct i915_winsys *iws,
                       unsigned size,
                       enum i915_winsys_buffer_type type)
{
    
    struct i915_winsys_buffer *buf;

    /*
    if( AttemptSemaphore(&UnusedBuffersListLock) )
    {
        if( (buf = (struct i915_winsys_buffer *)GetTail(&unusedbuffers)) )
        {
            if( buf->size == size )
            {
                if( ! buffer_is_busy(0, buf ) )
                {
                    D(bug("[GMA winsys] buffer_create: cheap seconhand buffer found:%p\n",buf);)
                    Remove((struct Node *)buf);
                    ReleaseSemaphore(&UnusedBuffersListLock);
                    return buf;
                }
            }
        }
        ReleaseSemaphore(&UnusedBuffersListLock);
    }
    */
    
    buf = CALLOC_STRUCT(i915_winsys_buffer);
    if (!buf)
    return NULL;

    // allocate page aligned gfx memory
    buf->allocated_size = size + 4096;
    if( !(buf->allocated_map = alloc_gfx_mem(buf->allocated_size) ) )
        return NULL;
    buf->map = (APTR)(((IPTR)buf->allocated_map + 4095)& ~4095);
    buf->size = size;
    buf->magic = MAGIC;
    D(bug("[GMA winsys] buffer_create size %d type %s = %p map %p\n",size,i915_type_to_name(type),buf,buf->map));

    return buf;

}


/**
* Create a tiled buffer.
*
* *stride, height are in bytes. The winsys tries to allocate the buffer with
* the tiling mode provide in *tiling. If tiling is no possible, *tiling will
* be set to I915_TILE_NONE. The calculated stride (incorporateing hw/kernel
* requirements) is always returned in *stride.
*/
struct i915_winsys_buffer *
      buffer_create_tiled(struct i915_winsys *iws,
                             unsigned *stride, unsigned height,
                             enum i915_winsys_buffer_tile *tiling,
                             enum i915_winsys_buffer_type type)
{
    D(bug("[GMA winsys] buffer_create_tiled stride=%d heigh=%d type:%s\n",*stride,height,i915_type_to_name(type)));
    // Tiling is not implemented.
    *tiling = I915_TILE_NONE;
    return buffer_create( iws, *stride * height, type );
}


/**
* Creates a buffer from a handle.
* Used to implement pipe_screen::resource_from_handle.
* Also provides the stride information needed for the
* texture via the stride argument.
*/
struct i915_winsys_buffer *
buffer_from_handle(struct i915_winsys *iws,
                            struct winsys_handle *whandle,
                            enum i915_winsys_buffer_tile *tiling,
                            unsigned *stride)
{
    D(bug("[GMA winsys] buffer_from_handle\n"));
    IMPLEMENT();
    return NULL;
}


/**
* Used to implement pipe_screen::resource_get_handle.
* The winsys might need the stride information.
*/
boolean buffer_get_handle(struct i915_winsys *iws,
                                struct i915_winsys_buffer *buffer,
                                struct winsys_handle *whandle,
                                unsigned stride)
{
    D(bug("[GMA winsys] buffer_get_handle\n"));
    IMPLEMENT();
    return FALSE;
}


/**
* Map a buffer.
*/
void *buffer_map(struct i915_winsys *iws,
                       struct i915_winsys_buffer *buffer,
                       boolean write)
{
    IF_BAD_MAGIC(buffer)
    {
        //for(;;);
        return 0;
    }
    D(bug("[GMA winsys] buffer_map %p\n",buffer));
    while( buffer_is_busy(iws, buffer )){};
    return buffer->map;
}


/**
* Unmap a buffer.
*/
void buffer_unmap(struct i915_winsys *iws,
                        struct i915_winsys_buffer *buffer)
{
    MAGIC_WARNING(buffer);
    D(bug("[GMA winsys] buffer_unmap %p\n",buffer));
   // IMPLEMENT();
}


/**
* Write to a buffer.
*
* Arguments follows pipe_buffer_write.
*/
int buffer_write(struct i915_winsys *iws,
                       struct i915_winsys_buffer *dst,
                       size_t offset,
                       size_t size,
                       const void *data)
{
    D(bug("[GMA winsys] buffer_write offset=%d size=%d\n",offset,size));
    IMPLEMENT();
    return 0;
}


void buffer_destroy(struct i915_winsys *iws,
                          struct i915_winsys_buffer *buf)
{
    IF_BAD_MAGIC(buf) return;
    D(bug("[GMA winsys] buffer_destroy %p\n", buf));

    if(0)
    {
        while( buffer_is_busy(0, buf ) ){};
        buf->magic = 0;
        free_gfx_mem(buf->allocated_map, buf->allocated_size);
        FREE(buf);
    }
    else
    {
        ObtainSemaphore(&UnusedBuffersListLock);
            AddTail( &unusedbuffers, (struct Node *)buf );
        ReleaseSemaphore(&UnusedBuffersListLock);
    }
}


/**
* Check if a buffer is busy.
*/
boolean buffer_is_busy(struct i915_winsys *iws,
                             struct i915_winsys_buffer *buf)
{
    MAGIC_WARNING(buf);

    int i=buf->status_index;
    D(bug("[GMA winsys] buffer_is_busy %p index %d flush_num %d\n",buf,i,buf->flush_num));
    if(i)
    {
        if( hw_status[i].reserved &&
            hw_status[i].flush_num == buf->flush_num )
        {
            if( get_status(i) )
            {
                return TRUE;
            }
            else
            {
                buf->status_index = 0;
                free_status_index(i);
            }
        }
    }
    return FALSE;
}




/****************************************************************
* Fence functions.
*************************************************************/

/**
* Reference fence and set ptr to fence.
*/
void fence_reference(struct i915_winsys *iws,
                           struct pipe_fence_handle **ptr,
                           struct pipe_fence_handle *fence)
{
    D(bug("[GMA winsys] fence_reference\n"));
    IMPLEMENT();
}


/**
* Check if a fence has finished.
*/
int fence_signalled(struct i915_winsys *iws,
                          struct pipe_fence_handle *fence)
{
    D(bug("[GMA winsys] fence_signalled\n"));
    IMPLEMENT();
    return 0;
}


/**
* Wait on a fence to finish.
*/
int fence_finish(struct i915_winsys *iws,
                       struct pipe_fence_handle *fence)
{
    D(bug("[GMA winsys] fence_finish\n"));
    IMPLEMENT();
    return 0;
}


void winsys_destroy(struct i915_winsys *ws)
{
    D(bug("[GMA winsys] winsys_destroy\n"));
    struct aros_winsys *aws = aros_winsys(ws);
    FREE(aws);
}

struct aros_winsys *
winsys_create()
{
    D(bug("[GMA winsys] winsys_create\n"));
    struct aros_winsys *aws;

    aws = CALLOC_STRUCT(aros_winsys);
    if (!aws) return NULL;

    aws->base.batchbuffer_create = batchbuffer_create;
    aws->base.validate_buffers = batchbuffer_validate_buffers;
    aws->base.batchbuffer_reloc = batchbuffer_reloc;
    aws->base.batchbuffer_flush = batchbuffer_flush;
    aws->base.batchbuffer_destroy = batchbuffer_destroy;

    aws->base.buffer_create = buffer_create;
    aws->base.buffer_create_tiled = buffer_create_tiled;
    aws->base.buffer_from_handle = buffer_from_handle;
    aws->base.buffer_get_handle = buffer_get_handle;
    aws->base.buffer_map = buffer_map;
    aws->base.buffer_unmap = buffer_unmap;
    aws->base.buffer_write = buffer_write;
    aws->base.buffer_destroy = buffer_destroy;
    aws->base.buffer_is_busy = buffer_is_busy;

    aws->base.fence_reference = fence_reference;
    aws->base.fence_signalled = fence_signalled;
    aws->base.fence_finish = fence_finish;

    aws->base.destroy = winsys_destroy;

    aws->base.pci_id = sd->ProductID;
#ifdef GALLIUM_SIMULATION
    aws->base.pci_id = 0x27AE;
#endif
    aws->max_batch_size = 16 * 4096;

    return aws;
}
