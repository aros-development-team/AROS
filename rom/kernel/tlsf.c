#include <exec/types.h>
#include <exec/memory.h>
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
 */
#define SIZE_ALIGN  16

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
        //*r = tr & ~tmp;
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

void * tlsf_malloc(void * tlsf_, IPTR size)
{
    tlsf_t *tlsf = (tlsf_t *)tlsf_;
    int fl, sl;
    bhdr_t *b = NULL;

    size = ROUNDUP(size);

    D(nbug("tlsf_malloc(%p, %ld)\n", tlsf, size));

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
            /* increase the size of requested block so that we can fit the headers too */
            IPTR sz = size + 3 * ROUNDUP(sizeof(hdr_t));

            /* Requested size less than puddle size? Get puddle size then */
            if (sz < tlsf->autogrow_puddle_size)
                sz = tlsf->autogrow_puddle_size;

            D(nbug("querying for %d bytes\n"));

            /* Try to get some memory */
            void * ptr = tlsf->autogrow_get_fn(tlsf->autogrow_data, &sz);

            /* Got it? Add to tlsf then */
            if (ptr)
            {
                tlsf_add_memory(tlsf, ptr, sz);

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

void tlsf_free(void * tlsf_, APTR ptr)
{
    tlsf_t *tlsf = (tlsf_t *)tlsf_;
    bhdr_t *fb = MEM_TO_BHDR(ptr);
    bhdr_t *next;

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

    /* Insert free block into the proper list */
    INSERT_FREE_BLOCK(tlsf, fb);
}


void * tlsf_realloc(void * tlsf_, void * ptr, IPTR new_size)
{
    tlsf_t *tlsf = (tlsf_t *)tlsf_;
    bhdr_t *b;
    bhdr_t *bnext;
    int fl;
    int sl;

    /* NULL pointer? just allocate the memory */
    if (unlikely(!ptr))
        return tlsf_malloc(tlsf, new_size);

    /* size = 0? free memory */
    if (unlikely(!new_size))
    {
        tlsf_free(tlsf, ptr);
        return NULL;
    }

    new_size = ROUNDUP(new_size);

    b = MEM_TO_BHDR(ptr);

    if (unlikely(new_size == GET_SIZE(b)))
        return ptr;

    bnext = GET_NEXT_BHDR(b, GET_SIZE(b));

    /* Is new size smaller than the previous one? Try to split the block if this is the case */
    if (new_size <= (GET_SIZE(b)))
    {
        return ptr;

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
            void * p = tlsf_malloc(tlsf, new_size);
            if (p)
            {
                CopyMemQuick(ptr, p, GET_SIZE(b));
                tlsf_free(tlsf, ptr);
                b = MEM_TO_BHDR(p);
            }
        }
    }

    return b->mem;
}


void * tlsf_allocabs(void * tlsf_, void * ptr, IPTR size)
{
    tlsf_t *tlsf = (tlsf_t *)tlsf_;
    UBYTE *region_start;
    UBYTE *region_end;

    int fl, sl;
    IPTR sz = ROUNDUP(size);

    D(nbug("[TLSF] allocabs(%p, %ld)\n", ptr, size));

    region_start = ptr;
    region_end = (UBYTE *)ptr + sz;

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

                    return breg->mem;
                }

                b0 = b0->free_node.next;
            }
        }
    }

    return NULL;
}

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
    b->header.length = (size - 3*ROUNDUP(sizeof(hdr_t)) - ROUNDUP(sizeof(tlsf_area_t))) | PREV_BUSY | THIS_BUSY;

    bend = GET_NEXT_BHDR(b, GET_SIZE(b));
    bend->header.length = 0 | THIS_BUSY | PREV_BUSY;
    bend->header.prev = b;

    area = (tlsf_area_t *)hdr->mem;
    area->end = bend;

    return area;
}

void tlsf_add_memory(void *tlsf_, void *memory, IPTR size)
{
    tlsf_t *tlsf = (tlsf_t *)tlsf_;

    D(nbug("tlsf_add_memory(%p, %p, %d)\n", tlsf, memory, size));

    if (memory && size > 3*ROUNDUP(sizeof(bhdr_t)))
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

        tlsf_free(tlsf, b->mem);
    }
}

void tlsf_add_memory_and_merge(void *tlsf_, void *memory, IPTR size)
{
    tlsf_add_memory(tlsf_, memory, size);
#warning TODO: add memory and merge...
}

#if 0
void bzero(void *ptr, IPTR len)
{
    UBYTE *p = (UBYTE *)ptr;

    while(len--)
        *p++ = 0;
}
#endif

void * tlsf_init(void * ptr, IPTR size)
{
    tlsf_t *tlsf = NULL;

    if (ptr && size >= ROUNDUP(sizeof(tlsf_t)))
    {
        tlsf = (tlsf_t *)ptr;

        bzero(tlsf, sizeof(tlsf_t));
        tlsf->autodestroy_self = 0;
    }
    else
    {
        tlsf = AllocMem(sizeof(tlsf_t), MEMF_ANY);

        if (tlsf)
        {
            bzero(tlsf, sizeof(tlsf_t));
            tlsf->autodestroy_self = 1;
        }
    }

    if (tlsf && size >= ROUNDUP(sizeof(tlsf_t)) + ROUNDUP(sizeof(hdr_t)))
    {
        tlsf_add_memory(tlsf, ptr + ROUNDUP(sizeof(tlsf_t)), size - ROUNDUP(sizeof(tlsf_t)));
    }

    return tlsf;
}

void * tlsf_init_autogrow(void * ptr, IPTR size, IPTR puddle_size, autogrow_get grow_function, autogrow_release release_function, APTR autogrow_data)
{
    tlsf_t *tlsf = tlsf_init(ptr, size);

    if (tlsf)
    {
        if (puddle_size < 4096)
            puddle_size = 4096;

        tlsf->autogrow_puddle_size = puddle_size;
        tlsf->autogrow_data = autogrow_data;
        tlsf->autogrow_get_fn = grow_function;
        tlsf->autogrow_release_fn = release_function;
    }

    return tlsf;
}

void tlsf_destroy(APTR tlsf_)
{
    tlsf_t *tlsf = (tlsf_t *)tlsf_;

    D(nbug("tlsf_destroy(%p)\n", tlsf));

    if (tlsf)
    {
        tlsf_area_t *area = tlsf->memory_area;

        if (tlsf->autogrow_release_fn)
        {
            while(area)
            {
                tlsf_area_t *next = area->next;
                bhdr_t *b;
                void *begin;
                void *end;
                IPTR size;

                /*
                 Autogrown area? Release it here.
                 Otherwise it's the responsibility of add_memory_area caller
                 */
                if (area->autogrown)
                {
                    /* get the begin of this area */
                    begin = MEM_TO_BHDR(area);

                    /* get sentinel block */
                    b = area->end;

                    /* end of this area is end of sentinel block */
                    end = GET_NEXT_BHDR(b, 0);

                    /* calculate the size of area */
                    size = (IPTR)end - (IPTR)begin;

                    if (tlsf->autogrow_release_fn)
                        tlsf->autogrow_release_fn(tlsf->autogrow_data, begin, size);
                }

                area = next;
            }
        }

        if (tlsf->autodestroy_self)
            FreeMem(tlsf, sizeof(tlsf_t));
    }
}

IPTR tlsf_avail(void * tlsf_, ULONG requirements)
{
    tlsf_t *tlsf = (tlsf_t *)tlsf_;
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

BOOL tlsf_in_bounds(void * tlsf_, void * begin, void * end)
{
    tlsf_t *tlsf = (tlsf_t *)tlsf_;
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
