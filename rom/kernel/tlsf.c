#include <exec/types.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <proto/exec.h>
#include <string.h>

#include "tlsf.h"
#include "kernel_base.h"
#include "kernel_debug.h"

#define D(x)

#undef USE_MACROS

#include <stddef.h>
/*
 * Minimal alignment as required by AROS. In contrary to the default
 * TLSF implementation, we do not allow smaller blocks here.
 * Size needs to be aligned to at least 8, see THIS_FREE_MASK comment.
 */
#define SIZE_ALIGN  AROS_WORSTALIGN

/*
 * Settings for TLSF allocator:
 * MAX_LOG2_SLI - amount of bits used for the second level list
 * MAX_FLI      - maximal allowable allocation size - 2^32 should be enough
 */
#define MAX_LOG2_SLI    (5)
#define MAX_SLI         (1 << MAX_LOG2_SLI)
#define MAX_FLI         (32)
#define FLI_OFFSET      (6)
#define SMALL_BLOCK     (2 << FLI_OFFSET)

#define REAL_FLI        (MAX_FLI - FLI_OFFSET)

#define ROUNDUP(x)      (((x) + SIZE_ALIGN - 1) & ~(SIZE_ALIGN - 1))
#define ROUNDDOWN(x)    ((x) & ~(SIZE_ALIGN - 1))

/* Fields used in the block header length field to identify busy/free blocks */
#define THIS_FREE_MASK (IPTR)1
#define THIS_FREE   (IPTR)1
#define THIS_BUSY   (IPTR)0

#define PREV_FREE_MASK (IPTR)2
#define PREV_FREE   (IPTR)2
#define PREV_BUSY   (IPTR)0

#define SIZE_MASK   (~(THIS_FREE_MASK | PREV_FREE_MASK))

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* Size of additional memory needed to manage new block */
#define HEADERS_SIZE (((3 * ROUNDUP(sizeof(hdr_t))) + ROUNDUP(sizeof(tlsf_area_t))))

/* free node links together all free blocks if similar size */
typedef struct free_node_s {
    struct bhdr_s *    prev;
    struct bhdr_s *    next;
} free_node_t;

/* block header in front of each block - both free and busy */
typedef struct hdr_s {
    struct bhdr_s * prev;
    IPTR            length;
} hdr_t;

/*
 * Each block is defined by bhdr_t structure. Free blocks contain only
 * the header which allows us to go through all memory blocks in the system.
 * The free blocks contain additionally the node which chains them in one
 * of the free block lists
 */
typedef struct bhdr_s {
    union {
        hdr_t       header;
        UBYTE       __min_align[SIZE_ALIGN];
    };
    union {
        UBYTE           mem[1];
        free_node_t     free_node;
    };
} bhdr_t;

/* Memory area within the TLSF pool */
typedef struct tlsf_area_s {
    struct tlsf_area_s *    next;       // Next memory area
    bhdr_t *                end;        // Pointer to "end-of-area" block header
    LONG                    autogrown;  // Automatically allocated by TLSF pool
} tlsf_area_t;

typedef struct {
    tlsf_area_t *       memory_area;

    IPTR                total_size;
    IPTR                free_size;

    ULONG               flbitmap;
    ULONG               slbitmap[REAL_FLI];

    IPTR                autogrow_puddle_size;
    ULONG               autogrow_requirements;
    APTR                autogrow_data;
    autogrow_get        autogrow_get_fn;
    autogrow_release    autogrow_release_fn;

    UBYTE               autodestroy_self;

    bhdr_t *            matrix[REAL_FLI][MAX_SLI];
} tlsf_t;

static inline __attribute__((always_inline)) int LS(IPTR i)
{
    if (sizeof(IPTR) == 4)
        return __builtin_ffs(i) - 1;
    else
        return __builtin_ffsl(i) - 1;
}

static inline __attribute__((always_inline)) int MS(IPTR i)
{
    if (sizeof(IPTR) == 4)
        return 31 - __builtin_clz(i);
    else
        return 63 - __builtin_clzl(i);
}

static inline __attribute__((always_inline)) void SetBit(int nr, ULONG *ptr)
{
    ptr[nr >> 5] |= (1 << (nr & 31));
}

static inline __attribute__((always_inline)) void ClrBit(int nr, ULONG *ptr)
{
    ptr[nr >> 5] &= ~(1 << (nr & 31));
}

static inline __attribute__((always_inline)) void MAPPING_INSERT(IPTR r, int *fl, int *sl)
{
    if (r < SMALL_BLOCK)
    {
        *fl = 0;
        *sl = (int)(r / (SMALL_BLOCK / MAX_SLI));
    }
    else
    {
        *fl = MS(r);
        *sl = (int)(((IPTR)r >> (*fl - MAX_LOG2_SLI)) - MAX_SLI);
        *fl -= FLI_OFFSET;
    }
}

static inline __attribute__((always_inline)) void MAPPING_SEARCH(IPTR *r, int *fl, int *sl)
{
    if (*r < SMALL_BLOCK)
    {
        *fl = 0;
        *sl = (int)(*r / (SMALL_BLOCK / MAX_SLI));
    }
    else
    {
        IPTR tmp = ((IPTR)1 << (MS(*r) - MAX_LOG2_SLI)) - 1;
        IPTR tr = *r + tmp;

        *fl = MS(tr);
        *sl = (int)(((IPTR)tr >> (*fl - MAX_LOG2_SLI)) - MAX_SLI);
        *fl -= FLI_OFFSET;
        *r = tr & ~tmp;
    }
}

static inline __attribute__((always_inline)) bhdr_t * FIND_SUITABLE_BLOCK(tlsf_t *tlsf, int *fl, int *sl)
{
    IPTR bitmap_tmp = tlsf->slbitmap[*fl] & (~0 << *sl);
    bhdr_t *b = NULL;

    if (bitmap_tmp)
    {
        *sl = LS(bitmap_tmp);
        b = tlsf->matrix[*fl][*sl];
    }
    else
    {
        bitmap_tmp = tlsf->flbitmap & (~0 << (*fl + 1));
        if (likely(bitmap_tmp != 0))
        {
            *fl = LS(bitmap_tmp);
            *sl = LS(tlsf->slbitmap[*fl]);
            b = tlsf->matrix[*fl][*sl];
        }
    }

    return b;
}


#ifdef USE_MACROS

#define GET_SIZE(b) ({ IPTR size = b->header.length & SIZE_MASK; size; })
#define GET_FLAGS(b) ({ IPTR flags = b->header.length & (THIS_FREE_MASK | PREV_FREE_MASK); flags; })
#define SET_SIZE(b, size) do{ b->header.length = GET_FLAGS(b) | (size); }while(0)
#define SET_FLAGS(b, flags) do{ b->header.length = GET_SIZE(b) | (flags); }while(0)
#define SET_SIZE_AND_FLAGS(b, size, flags) do{b->header.length = (size) | (flags);}while(0)
#define FREE_BLOCK(b) ((b->header.length & THIS_FREE_MASK) == THIS_FREE)
#define SET_FREE_BLOCK(b) do{b->header.length = (b->header.length & ~THIS_FREE_MASK) | THIS_FREE;}while(0)
#define SET_BUSY_BLOCK(b) do{b->header.length = (b->header.length & ~THIS_FREE_MASK) | THIS_BUSY;}while(0)
#define SET_FREE_PREV_BLOCK(b) do{b->header.length = (b->header.length & ~PREV_FREE_MASK) | PREV_FREE;}while(0)
#define SET_BUSY_PREV_BLOCK(b) do{b->header.length = (b->header.length & ~PREV_FREE_MASK) | PREV_BUSY;}while(0)
#define FREE_PREV_BLOCK(b) ((b->header.length & PREV_FREE_MASK) == PREV_FREE)
#define GET_NEXT_BHDR(hdr, size) ({ bhdr_t * __b = (bhdr_t *)((UBYTE *)&hdr->mem[0] + (size)); __b; })
#define MEM_TO_BHDR(ptr) ({ bhdr_t * b = (bhdr_t*)((void*)(ptr) - offsetof(bhdr_t, mem)); b; })

#define REMOVE_HEADER(tlsf, b, fl, sl) do{ \
        if (b->free_node.next)                                          \
            b->free_node.next->free_node.prev = b->free_node.prev;      \
        if (b->free_node.prev)                                          \
            b->free_node.prev->free_node.next = b->free_node.next;      \
        if (tlsf->matrix[fl][sl] == b) {                                \
            tlsf->matrix[fl][sl] = b->free_node.next;                   \
            if (!tlsf->matrix[fl][sl])                                  \
                ClrBit(sl, &tlsf->slbitmap[fl]);                        \
            if (!tlsf->slbitmap[fl])                                    \
                ClrBit(fl, &tlsf->flbitmap);                            \
        } } while(0)

#define INSERT_FREE_BLOCK(tlsf, b) do {                     \
    int fl, sl; MAPPING_INSERT(GET_SIZE(b), &fl, &sl);      \
    b->free_node.prev = NULL;                               \
    b->free_node.next = tlsf->matrix[fl][sl];               \
    if (tlsf->matrix[fl][sl])                               \
        tlsf->matrix[fl][sl]->free_node.prev = b;           \
    tlsf->matrix[fl][sl] = b;                               \
    SetBit(fl, &tlsf->flbitmap);                            \
    SetBit(sl, &tlsf->slbitmap[fl]); }while(0)

#else

static inline __attribute__((always_inline)) IPTR GET_SIZE(bhdr_t *b)
{
    return b->header.length & SIZE_MASK;
}

static inline __attribute__((always_inline)) IPTR GET_FLAGS(bhdr_t *b)
{
    return b->header.length & (THIS_FREE_MASK | PREV_FREE_MASK);
}

static inline __attribute__((always_inline)) void SET_SIZE(bhdr_t *b, IPTR size)
{
    b->header.length = GET_FLAGS(b) | size;
}

static inline __attribute__((always_inline)) void SET_SIZE_AND_FLAGS(bhdr_t *b, IPTR size, IPTR flags)
{
    b->header.length = size | flags;
}

static inline __attribute__((always_inline)) int FREE_BLOCK(bhdr_t *b)
{
    return ((b->header.length & THIS_FREE_MASK) == THIS_FREE);
}

static inline __attribute__((always_inline)) void SET_FREE_BLOCK(bhdr_t *b)
{
    b->header.length = (b->header.length & ~THIS_FREE_MASK) | THIS_FREE;
}

static inline __attribute__((always_inline)) void SET_BUSY_BLOCK(bhdr_t *b)
{
    b->header.length = (b->header.length & ~THIS_FREE_MASK) | THIS_BUSY;
}

static inline __attribute__((always_inline)) void SET_FREE_PREV_BLOCK(bhdr_t *b)
{
    b->header.length = (b->header.length & ~PREV_FREE_MASK) | PREV_FREE;
}

static inline __attribute__((always_inline)) void SET_BUSY_PREV_BLOCK(bhdr_t *b)
{
    b->header.length = (b->header.length & ~PREV_FREE_MASK) | PREV_BUSY;
}

static inline __attribute__((always_inline)) int FREE_PREV_BLOCK(bhdr_t *b)
{
    return ((b->header.length & PREV_FREE_MASK) == PREV_FREE);
}

static inline __attribute__((always_inline)) bhdr_t * GET_NEXT_BHDR(bhdr_t *hdr, IPTR size)
{
    return (bhdr_t *)((UBYTE *)&hdr->mem[0] + size);
}

static inline __attribute__((always_inline)) bhdr_t * MEM_TO_BHDR(void *ptr)
{
    return (bhdr_t *)(ptr - offsetof(bhdr_t, mem));
}

static inline __attribute__((always_inline)) void REMOVE_HEADER(tlsf_t *tlsf, bhdr_t *b, int fl, int sl)
{
    if (b->free_node.next)
        b->free_node.next->free_node.prev = b->free_node.prev;
    if (b->free_node.prev)
        b->free_node.prev->free_node.next = b->free_node.next;

    if (tlsf->matrix[fl][sl] == b)
    {
        tlsf->matrix[fl][sl] = b->free_node.next;
        if (!tlsf->matrix[fl][sl])
            ClrBit(sl, &tlsf->slbitmap[fl]);
        if (!tlsf->slbitmap[fl])
            ClrBit(fl, &tlsf->flbitmap);
    }
}

static inline __attribute__((always_inline)) void INSERT_FREE_BLOCK(tlsf_t *tlsf, bhdr_t *b)
{
    int fl, sl;

    MAPPING_INSERT(GET_SIZE(b), &fl, &sl);

    b->free_node.prev = NULL;
    b->free_node.next = tlsf->matrix[fl][sl];

    if (tlsf->matrix[fl][sl])
        tlsf->matrix[fl][sl]->free_node.prev = b;

    tlsf->matrix[fl][sl] = b;

    SetBit(fl, &tlsf->flbitmap);
    SetBit(sl, &tlsf->slbitmap[fl]);
}

#endif /* USE_MACROS */

void * tlsf_malloc(struct MemHeaderExt *mhe, IPTR size, ULONG *flags)
{
    tlsf_t *tlsf = (tlsf_t *)mhe->mhe_UserData;
    int fl, sl;
    bhdr_t *b = NULL;

    size = ROUNDUP(size);

    if (unlikely(!size)) return NULL;

    D(nbug("tlsf_malloc(%p, %ld)\n", tlsf, size));

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ObtainSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    /* Find the indices fl and sl for given size */
    MAPPING_SEARCH(&size, &fl, &sl);

    /* Find block of either the right size or larger */
    b = FIND_SUITABLE_BLOCK(tlsf, &fl, &sl);

    D(nbug("  tlsf_malloc - adjusted size %ld\n", size));

    /* No block found? Either failure or tlsf will get more memory. */
    if (unlikely(!b))
    {
        D(nbug("tlsf_malloc out of memory\n"));

        /* Do we have the autogrow feature? */
        if (tlsf->autogrow_get_fn)
        {
            /* Increase the size of requested block so that we can fit the headers too */
            IPTR sz = size + HEADERS_SIZE;

            /* Requested size less than puddle size? Get puddle size then */
            if (sz < tlsf->autogrow_puddle_size)
                sz = tlsf->autogrow_puddle_size;

            D(nbug("querying for %d bytes\n", sz));

            /* Try to get some memory */
            void * ptr = tlsf->autogrow_get_fn(tlsf->autogrow_data, &sz);

            /* Got it? Add to tlsf then */
            if (ptr)
            {
                tlsf_add_memory(mhe, ptr, sz);

                /* We know the newly added memory is first in the list. Set the autogrown feature there */
                tlsf->memory_area->autogrown = 1;

                /* Memory is there. Try to find the block again */
                MAPPING_SEARCH(&size, &fl, &sl);
                b = FIND_SUITABLE_BLOCK(tlsf, &fl, &sl);
            }
        }

        /* No block? FAILURE! */
        if (!b)
        {
            if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
                ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

            return NULL;
        }
    }

    /* Next header */
    bhdr_t *next = GET_NEXT_BHDR(b, GET_SIZE(b));

    /* Remove the found block from the free list */
    REMOVE_HEADER(tlsf, b, fl, sl);

    /* Is this block larger then requested? Try to split it then */
    if (likely(GET_SIZE(b) > (size + ROUNDUP(sizeof(hdr_t)))))
    {
        /* New split block */
        bhdr_t *sb = GET_NEXT_BHDR(b, size);
        sb->header.prev = b;

        /* Set size, this free and previous busy */
        SET_SIZE_AND_FLAGS(sb, GET_SIZE(b) - size - ROUNDUP(sizeof(hdr_t)), THIS_FREE | PREV_BUSY);

        /* The next header points to free block now */
        next->header.prev = sb;

        /* previous block (sb) is free */
        SET_FREE_PREV_BLOCK(next);

        /* Allocated block size truncated */
        SET_SIZE(b, size);

        D(nbug("  new splitted block of size %ld\n", GET_SIZE(sb)));
        /* Free block is inserted to free list */
        INSERT_FREE_BLOCK(tlsf, sb);
    }
    else
    {
        /* The block was of right size. Set it just busy in next pointer */
        SET_BUSY_PREV_BLOCK(next);
    }

    /* The allocated block is busy */
    SET_BUSY_BLOCK(b);

    /* Clear the pointers just in case */
    b->free_node.next = NULL;
    b->free_node.prev = NULL;

    /* Update counters */
    tlsf->free_size -= GET_SIZE(b);
    mhe->mhe_MemHeader.mh_Free = tlsf->free_size;

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    if (flags && (*flags & MEMF_CLEAR))
        bzero(&b->mem[0], size);

    /* And return memory */
    return &b->mem[0];
}

static inline __attribute__((always_inline)) void MERGE(bhdr_t *b1, bhdr_t *b2)
{
    /* Merging adjusts the size - it's sum of both sizes plus size of block header */
    SET_SIZE(b1, GET_SIZE(b1) + GET_SIZE(b2) + ROUNDUP(sizeof(hdr_t)));
}

static inline __attribute__((always_inline)) bhdr_t * MERGE_PREV(tlsf_t *tlsf, bhdr_t *block)
{
    /* Is previous block free? */
    if (FREE_PREV_BLOCK(block))
    {
        int fl, sl;
        bhdr_t *prev = block->header.prev;

        /* Calculate index for removal */
        MAPPING_INSERT(GET_SIZE(prev), &fl, &sl);

        /* Do remove the header from the list */
        REMOVE_HEADER(tlsf, prev, fl, sl);

        /* Merge */
        MERGE(prev, block);

        return prev;
    }
    else
        return block;
}

static inline __attribute__((always_inline)) bhdr_t * MERGE_NEXT(tlsf_t *tlsf, bhdr_t *block)
{
    bhdr_t *next = GET_NEXT_BHDR(block, GET_SIZE(block));

    /* Is next block free? */
    if (FREE_BLOCK(next))
    {
        int fl, sl;

        /* Calculate index for removal */
        MAPPING_INSERT(GET_SIZE(next), &fl, &sl);

        /* Remove the header from the list */
        REMOVE_HEADER(tlsf, next, fl, sl);

        /* merge blocks */
        MERGE(block, next);
    }

    return block;
}

static void tlsf_release_memory_area(struct MemHeaderExt * mhe, tlsf_area_t * area)
{
    tlsf_t *tlsf = (tlsf_t *)mhe->mhe_UserData;
    tlsf_area_t *p = (tlsf_area_t *)(&tlsf->memory_area - offsetof(tlsf_area_t, next));
    bhdr_t *b;
    void *begin;
    void *end;
    IPTR size;

    /* get the begin of this area */
    begin = MEM_TO_BHDR(area);

    /* get sentinel block */
    b = area->end;

    /* end of this area is end of sentinel block */
    end = GET_NEXT_BHDR(b, 0);

    /* calculate the size of area */
    size = (IPTR)end - (IPTR)begin;

    /* update counters */
    tlsf->total_size -= size;
    tlsf->free_size  -= GET_SIZE(area->end->header.prev);

    /* remove area from list */
    for (;p->next != NULL; p = p->next)
        if (p->next == area)
        {
            p->next = area->next;
            break;
        }

    /* release */
    if (tlsf->autogrow_release_fn)
        tlsf->autogrow_release_fn(tlsf->autogrow_data, begin, size);
}

void tlsf_freevec(struct MemHeaderExt * mhe, APTR ptr)
{
    tlsf_t *tlsf = (tlsf_t *)mhe->mhe_UserData;
    bhdr_t *fb;
    bhdr_t *next;
    tlsf_area_t * area;

    if (unlikely(!ptr))
        return;

    fb = MEM_TO_BHDR(ptr);

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ObtainSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    /* Mark block as free */
    SET_FREE_BLOCK(fb);

    /* adjust free size field on tlsf */
    tlsf->free_size += GET_SIZE(fb);

    /* Try to merge with previous and next blocks (if free) */
    fb = MERGE_PREV(tlsf, fb);
    fb = MERGE_NEXT(tlsf, fb);

    /* Tell next block that previous one is free. Also update the prev link in case it changed */
    next = GET_NEXT_BHDR(fb, GET_SIZE(fb));
    SET_FREE_PREV_BLOCK(next);
    next->header.prev = fb;

    /* Check if this was the last used block of an autogrown area */
    area = fb->header.prev->header.prev == NULL ? (tlsf_area_t *)fb->header.prev->mem : NULL;
    if (area != NULL && area->end == next && area->autogrown == 1)
        tlsf_release_memory_area(mhe, area);
    else
        /* Insert free block into the proper list */
        INSERT_FREE_BLOCK(tlsf, fb);

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);
}

void tlsf_freemem(struct MemHeaderExt * mhe, APTR ptr, IPTR size)
{
    (void)size;
    tlsf_freevec(mhe, ptr);
}

void * tlsf_realloc(struct MemHeaderExt *mhe, APTR ptr, IPTR new_size)
{
    tlsf_t *tlsf = (tlsf_t *)mhe->mhe_UserData;
    bhdr_t *b;
    bhdr_t *bnext;
    int fl;
    int sl;

    /* NULL pointer? just allocate the memory */
    if (unlikely(!ptr))
        return tlsf_malloc(mhe, new_size, NULL);

    /* size = 0? free memory */
    if (unlikely(!new_size))
    {
        tlsf_freevec(mhe, ptr);
        return NULL;
    }

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ObtainSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    new_size = ROUNDUP(new_size);

    b = MEM_TO_BHDR(ptr);

    if (unlikely(new_size == GET_SIZE(b)))
        return ptr;

    bnext = GET_NEXT_BHDR(b, GET_SIZE(b));

    /* Is new size smaller than the previous one? Try to split the block if this is the case */
    if (new_size <= (GET_SIZE(b)))
    {
        /* New header starts right after the current block b */
        bhdr_t * b1 = GET_NEXT_BHDR(b, new_size);

        /* Update pointer and size */
        b1->header.prev = b;
        SET_SIZE_AND_FLAGS(b1, GET_SIZE(b) - new_size - ROUNDUP(sizeof(hdr_t)), THIS_FREE | PREV_BUSY);

        /* Current block gets smaller */
        SET_SIZE(b, new_size);

        tlsf->free_size += GET_SIZE(b1);

        /* Try to merge with next block */
        b1 = MERGE_NEXT(tlsf, b1);

        /* Tell next block that previous one is free. Also update the prev link in case it changed */
        bnext = GET_NEXT_BHDR(b1, GET_SIZE(b1));
        SET_FREE_PREV_BLOCK(bnext);
        bnext->header.prev = b1;

        /* Insert free block into the proper list */
        INSERT_FREE_BLOCK(tlsf, b1);
    }
    else
    {
        /* Is next block free? Is there enough free space? */
        if (FREE_BLOCK(bnext) && new_size <= GET_SIZE(b) + GET_SIZE(bnext) + ROUNDUP(sizeof(hdr_t)))
        {
            bhdr_t *b1;
            IPTR rest_size = ROUNDUP(sizeof(hdr_t)) + GET_SIZE(bnext) + GET_SIZE(b) - new_size;

            MAPPING_INSERT(GET_SIZE(bnext), &fl, &sl);

            REMOVE_HEADER(tlsf, bnext, fl, sl);

            if (rest_size > ROUNDUP(sizeof(hdr_t)))
            {
                rest_size -= ROUNDUP(sizeof(hdr_t));

                SET_SIZE(b, new_size);

                b1 = GET_NEXT_BHDR(b, GET_SIZE(b));
                b1->header.prev = b;

                SET_SIZE_AND_FLAGS(b1, rest_size, THIS_FREE | PREV_BUSY);

                bnext = GET_NEXT_BHDR(b1, GET_SIZE(b1));
                bnext->header.prev = b1;
                SET_FREE_PREV_BLOCK(bnext);

                INSERT_FREE_BLOCK(tlsf, b1);
            }
            else
            {
                if (rest_size)
                    SET_SIZE(b, new_size + ROUNDUP(sizeof(hdr_t)));
                else
                    SET_SIZE(b, new_size);

                bnext = GET_NEXT_BHDR(b, GET_SIZE(b));
                bnext->header.prev = b;
                SET_BUSY_PREV_BLOCK(bnext);
            }
        }
        else
        {
            /* Next block was not free. Create new buffer and copy old contents there */
            void * p = tlsf_malloc(mhe, new_size, NULL);
            if (p)
            {
                CopyMemQuick(ptr, p, GET_SIZE(b));
                tlsf_freevec(mhe, ptr);
                b = MEM_TO_BHDR(p);
            }
        }
    }

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    return b->mem;
}


void * tlsf_allocabs(struct MemHeaderExt * mhe, IPTR size, void * ptr)
{
    tlsf_t *tlsf = (tlsf_t *)mhe->mhe_UserData;
    UBYTE *region_start;
    UBYTE *region_end;

    int fl, sl;
    IPTR sz = ROUNDUP(size);

    D(nbug("[TLSF] allocabs(%p, %ld)\n", ptr, size));

    region_start = ptr;
    region_end = (UBYTE *)ptr + sz;

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ObtainSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    /* Start searching here. It doesn't make sense to go through regions which are smaller */
    MAPPING_SEARCH(&sz, &fl, &sl);

    /* Start looking now :) */
    for (; fl < MAX_FLI; fl++)
    {
        for (; sl < MAX_SLI; sl++)
        {
            bhdr_t *b0 = tlsf->matrix[fl][sl];

            /* If block was there, check it */
            while (b0)
            {
                bhdr_t *b1 = GET_NEXT_BHDR(b0, GET_SIZE(b0));

                /* The block has to contain _whole_ requested region, max exceed it in size though */
                if (b0->mem <= region_start && (UBYTE *)b1 >= region_end)
                {
                    /* block header of requested region */
                    bhdr_t *breg = MEM_TO_BHDR(ptr);

                    /*
                     This is the block we're looking for. Unchain it from the bidirectional list of
                     free blocks now.

                     Previous entry's next will point to this block's next. If previous is NULL, matrix
                     will be set to block's next
                     */
                    if (b0->free_node.prev)
                        b0->free_node.prev->free_node.next = b0->free_node.next;
                    else
                        tlsf->matrix[fl][sl] = b0->free_node.next;

                    /*
                     Next entry's prev will point to this block's previous.
                     */
                    if (b0->free_node.next)
                        b0->free_node.next->free_node.prev = b0->free_node.prev;

                    /* Empty SL matrix for size j? Clear bit */
                    if (!tlsf->matrix[fl][sl])
                    {
                        ClrBit(sl, &tlsf->slbitmap[fl]);

                        /* Empty entire SL matrix for given FL index? clear that bit too */
                        if (tlsf->slbitmap[fl])
                            ClrBit(fl, &tlsf->flbitmap);
                    }

                    b0->free_node.prev = NULL;
                    b0->free_node.next = NULL;
                    SET_BUSY_BLOCK(b0);

                    /*
                     At this point the block is removed from free list and marked as used.
                     Now, split it if necessary...
                     */

                    /* begin of the block != begin of the block header of requested region? */
                    if (b0 != breg)
                    {
                        /*
                         Adjust region's block header. Mark in size that previous (aka b0) is free.
                         Reduce the size of b0 as well as size of breg too.
                         */
                        breg->header.prev = b0;
                        SET_SIZE_AND_FLAGS(breg, GET_SIZE(b0)-((IPTR)breg - (IPTR)b0), PREV_FREE | THIS_BUSY);

                        /* Update the next block. Mark in size that previous (breg) is used */
                        b1->header.prev = breg;
                        SET_BUSY_PREV_BLOCK(b1);

                        /* b0's prev state is keept. b0 itself is marked as free block */
                        SET_FREE_BLOCK(b0);
                        SET_SIZE(b0, (IPTR)breg - (IPTR)b0->mem);

                        /* Insert b0 to free list */
                        MAPPING_INSERT(GET_SIZE(b0), &fl, &sl);
                        INSERT_FREE_BLOCK(tlsf, b0);
                    }

                    /* Is it necessary to split the requested region at the end? */
                    if ((SIZE_ALIGN + GET_SIZE(breg)) > size)
                    {
                        IPTR tmp_size = GET_SIZE(breg) - size - SIZE_ALIGN;

                        /* New region header directly at end of the requested region */
                        bhdr_t *b2 = GET_NEXT_BHDR(breg, size);

                        /* Adjust fields */
                        b2->header.prev = breg;
                        SET_SIZE_AND_FLAGS(b2, tmp_size, PREV_BUSY | THIS_FREE);

                        /* requested region's size is now smaller */
                        SET_SIZE(breg, size);

                        /* The next block header point to newly created one */
                        b1->header.prev = b2;
                        SET_FREE_PREV_BLOCK(b1);

                        /* Insert newly created block to free list */
                        MAPPING_INSERT(GET_SIZE(b2), &fl, &sl);
                        INSERT_FREE_BLOCK(tlsf, b2);
                    }

                    tlsf->free_size -= GET_SIZE(breg);


                    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
                        ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

                    return breg->mem;
                }

                b0 = b0->free_node.next;
            }
        }
    }

    if (mhe->mhe_MemHeader.mh_Attributes & MEMF_SEM_PROTECTED)
        ReleaseSemaphore((struct SignalSemaphore *)mhe->mhe_MemHeader.mh_Node.ln_Name);

    return NULL;
}

/* Allocation of headers in memory:
 * hdr
 *  header      (ROUNDUP(sizeof(hdr_t))
 *  mem         (ROUNDUP(sizeof(tlst_area_t))
 * b
 *  header      (ROUNDUP(sizeof(hdr_t))
 *  free space  (size - HEADERS_SIZE)
 * bend
 *  header      (ROUNDUP(sizeof(hdr_t))
 */
tlsf_area_t * init_memory_area(void * memory, IPTR size)
{
    bhdr_t * hdr = (bhdr_t *)memory;
    bhdr_t * b;
    bhdr_t * bend;

    tlsf_area_t * area;

    size = ROUNDDOWN(size);

    /* Prepare first header, which protects the tlst_area_t header */
    hdr->header.length = ROUNDUP(sizeof(tlsf_area_t)) | THIS_BUSY | PREV_BUSY;
    hdr->header.prev = NULL;

    b = GET_NEXT_BHDR(hdr, ROUNDUP(sizeof(tlsf_area_t)));
    b->header.prev = hdr;
    b->header.length = (size - HEADERS_SIZE) | PREV_BUSY | THIS_BUSY;

    bend = GET_NEXT_BHDR(b, GET_SIZE(b));
    bend->header.length = 0 | THIS_BUSY | PREV_BUSY;
    bend->header.prev = b;

    area = (tlsf_area_t *)hdr->mem;
    area->end = bend;

    return area;
}

void tlsf_add_memory(struct MemHeaderExt *mhe, void *memory, IPTR size)
{
    tlsf_t *tlsf = (tlsf_t *)mhe->mhe_UserData;

    D(nbug("tlsf_add_memory(%p, %p, %d)\n", tlsf, memory, size));

    if (memory && size > HEADERS_SIZE)
    {
        tlsf_area_t *area = init_memory_area(memory, size);
        bhdr_t *b;

        D(nbug("  adding memory\n"));

        area->next = tlsf->memory_area;
        tlsf->memory_area = area;

        /* User added memory. Not autogrown */
        area->autogrown = 0;

        b = MEM_TO_BHDR(area);
        b = GET_NEXT_BHDR(b, GET_SIZE(b));

        tlsf->total_size += size;

        D(nbug("  total_size=%08x\n", tlsf->total_size));

        /* Add the initialized memory */
        tlsf_freevec(mhe, b->mem);
    }
}

void tlsf_add_memory_and_merge(struct MemHeaderExt *mhe, void *memory, IPTR size)
{
    tlsf_add_memory(mhe, memory, size);
    // TODO: add memory and merge...
}

#if 0
void bzero(void *ptr, IPTR len)
{
    UBYTE *p = (UBYTE *)ptr;

    while(len--)
        *p++ = 0;
}
#endif

void * tlsf_init(struct MemHeaderExt * mhe)
{
    tlsf_t *tlsf = NULL;
    void * ptr = mhe->mhe_MemHeader.mh_Lower;

    /* if MemHeaderExt starts at the beginning of handled memory, advance the ptr */
    if (mhe == ptr)
        ptr += ROUNDUP(sizeof(struct MemHeaderExt));

    /* Is there enough room for tlsf in the mem header itself? */
    if (mhe->mhe_MemHeader.mh_Free >= (ROUNDUP(sizeof(tlsf_t)) + 3 * ROUNDUP(sizeof(bhdr_t))))
    {
        /* tlsf will be stored inside handled memory */
        tlsf = (tlsf_t *)ptr;

        ptr += ROUNDUP(sizeof(tlsf_t));

        bzero(tlsf, sizeof(tlsf_t));
        tlsf->autodestroy_self = 0;
    }
    else
    {
        /* No place for tlsf header in MemHeaderExt? Allocate it separately */
        tlsf = AllocMem(sizeof(tlsf_t), MEMF_ANY);

        if (tlsf)
        {
            bzero(tlsf, sizeof(tlsf_t));
            tlsf->autodestroy_self = 1;
        }
    }

    /* Store the tlsf pointer in UserData field */
    mhe->mhe_UserData = tlsf;

    if (tlsf && ptr < mhe->mhe_MemHeader.mh_Upper)
    {
        tlsf_add_memory(mhe, ptr, (IPTR)mhe->mhe_MemHeader.mh_Upper - (IPTR)ptr);
    }

    return tlsf;
}

static void * tlsf_init_autogrow(struct MemHeaderExt * mhe, IPTR puddle_size, ULONG requirements, autogrow_get grow_function, autogrow_release release_function, APTR autogrow_data)
{
    tlsf_t *tlsf = tlsf_init(mhe);

    if (tlsf)
    {
        if (puddle_size < 4096)
            puddle_size = 4096;

        tlsf->autogrow_puddle_size = puddle_size;
        tlsf->autogrow_requirements = requirements;
        tlsf->autogrow_data = autogrow_data;
        tlsf->autogrow_get_fn = grow_function;
        tlsf->autogrow_release_fn = release_function;
    }

    return tlsf;
}

void tlsf_destroy(struct MemHeaderExt * mhe)
{
    tlsf_t *tlsf = (tlsf_t *)mhe->mhe_UserData;

    D(nbug("tlsf_destroy(%p)\n", tlsf));

    if (tlsf)
    {
        tlsf_area_t *area = tlsf->memory_area;

        if (tlsf->autogrow_release_fn)
        {
            while(area)
            {
                tlsf_area_t *next = area->next;

                /*
                 Autogrown area? Release it here.
                 Otherwise it's the responsibility of add_memory_area caller
                 */
                if (area->autogrown)
                    tlsf_release_memory_area(mhe, area);

                area = next;
            }
        }

        if (tlsf->autodestroy_self)
            FreeMem(tlsf, sizeof(tlsf_t));
    }
}

IPTR tlsf_avail(struct MemHeaderExt * mhe, ULONG requirements)
{
    tlsf_t *tlsf = (tlsf_t *)mhe->mhe_UserData;
    IPTR ret = 0;

    if (requirements & MEMF_TOTAL)
        ret = tlsf->total_size;
    else if (requirements & MEMF_LARGEST)
    {
        bhdr_t *b = NULL;

        if (tlsf->flbitmap)
        {
            int fl = MS(tlsf->flbitmap);

            if (tlsf->slbitmap[fl])
            {
                int sl = MS(tlsf->slbitmap[fl]);

                b = tlsf->matrix[fl][sl];
            }
        }

        while (b)
        {
            if (GET_SIZE(b) > ret)
                ret = GET_SIZE(b);

            b = b->free_node.next;
        }
    }
    else
        ret = tlsf->free_size;

    return ret;
}

BOOL tlsf_in_bounds(struct MemHeaderExt * mhe, void * begin, void * end)
{
    tlsf_t *tlsf = (tlsf_t *)mhe->mhe_UserData;
    tlsf_area_t *area;

    area = tlsf->memory_area;

    D(nbug("tlsf_in_bounds(%p, %p, %p)\n", tlsf, begin, end));

    while (area)
    {
        D(nbug("  area %p\n"));
        /*
         * Do checks only if questioned memory ends before the end (sentinel bhdr)
         * of area
         */
        if ((IPTR)end <= (IPTR)area->end)
        {
            D(nbug("  end <= area->end (%p <= %p)\n", end, area->end));

            /* Get the bhdr of this area */
            bhdr_t *b = MEM_TO_BHDR(area);

            /* Forward to the begin of the memory */
            b = GET_NEXT_BHDR(b, GET_SIZE(b));

            /* requested memory starts at begin or after begin of the area */
            if ((IPTR)begin >= (IPTR)b->mem)
            {
                D(nbug("  begin >= b->mem (%p >= %p)\n", begin, b->mem));
                return TRUE;
            }
        }

        area = area->next;
    }

    return FALSE;
}


static void destroy_Pool(struct MemHeaderExt *mhe)
{
    tlsf_destroy(mhe);
}

static APTR fetch_more_ram(void * data, IPTR *size)
{
    struct MemHeaderExt *mhe = (struct MemHeaderExt *)data;
    tlsf_t *tlsf = (tlsf_t *)mhe->mhe_UserData;

    D(nbug("[TLSF] fetch_more_ram(%p, %d)\n", mhe, *size));

    APTR ptr = AllocMem(*size, tlsf->autogrow_requirements);
    return ptr;
}

static VOID release_ram(void * data, APTR ptr, IPTR size)
{
    D(nbug("[TLSF] release_ram(%p, %d)\n", ptr, size));

    FreeMem(ptr, size);
}

static void * init_Pool(struct MemHeaderExt *mhe, IPTR puddleSize, IPTR initialSize)
{
    return tlsf_init_autogrow(mhe, puddleSize, (ULONG)mhe->mhe_MemHeader.mh_First, fetch_more_ram, release_ram, mhe);
}

void krnCreateTLSFMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size, ULONG flags)
{
    /* If the end is less than (1 << 31), MEMF_31BIT is implied */
    if (((IPTR)start+size) < (1UL << 31))
        flags |= MEMF_31BIT;
    else
        flags &= ~MEMF_31BIT;

    flags |= MEMF_MANAGED;

    struct MemHeaderExt *mhe = start;

    mhe->mhe_Magic         = MEMHEADER_EXT_MAGIC;

    mhe->mhe_DestroyPool   = destroy_Pool;
    mhe->mhe_InitPool      = init_Pool;

    mhe->mhe_Alloc         = tlsf_malloc;
    mhe->mhe_AllocVec      = tlsf_malloc;
    mhe->mhe_Free          = tlsf_freemem;
    mhe->mhe_FreeVec       = tlsf_freevec;
    mhe->mhe_AllocAbs      = tlsf_allocabs;
    mhe->mhe_ReAlloc       = tlsf_realloc;
    mhe->mhe_Avail         = tlsf_avail;
    mhe->mhe_InBounds      = tlsf_in_bounds;

    mhe->mhe_MemHeader.mh_Node.ln_Succ    = NULL;
    mhe->mhe_MemHeader.mh_Node.ln_Pred    = NULL;
    mhe->mhe_MemHeader.mh_Node.ln_Type    = NT_MEMORY;
    mhe->mhe_MemHeader.mh_Node.ln_Name    = (STRPTR)name;
    mhe->mhe_MemHeader.mh_Node.ln_Pri     = pri;
    mhe->mhe_MemHeader.mh_Attributes      = flags;
    /* The first MemChunk needs to be aligned. We do it by adding MEMHEADER_TOTAL. */
    mhe->mhe_MemHeader.mh_First           = NULL;

    mhe->mhe_UserData      = NULL;

    /*
     * mh_Lower and mh_Upper are informational only. Since our MemHeader resides
     * inside the region it describes, the region includes MemHeader.
     */
    mhe->mhe_MemHeader.mh_Lower           = start;
    mhe->mhe_MemHeader.mh_Upper           = start + size;
    mhe->mhe_MemHeader.mh_Free            = size;

    tlsf_init(mhe);
}
