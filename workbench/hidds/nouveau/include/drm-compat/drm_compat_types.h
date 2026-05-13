/*
    Copyright 2009-2017, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_COMPAT_TYPES_
#define _DRM_COMPAT_TYPES_

#include <aros/config.h>

#include <exec/lists.h>
#include <exec/types.h>
#include <exec/semaphores.h>
#include <sys/types.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>

#define __user
#define __iomem
#define __force
#define __u64                       UQUAD
#define __s64                       QUAD
#define __u32                       ULONG
#define __s32                       LONG
#define __u16                       UWORD
#define __s16                       WORD
#define __u8                        UBYTE
#define __s8                        BYTE
#define u16                         UWORD
#define s16                         WORD
#define u32                         ULONG
#define s32                         LONG
#define u64                         UQUAD
#define s64                         QUAD
#define u8                          UBYTE
#define s8                          BYTE
#define resource_size_t             IPTR
#define __kernel_size_t             IPTR
#define dma_addr_t                  IPTR
#define phys_addr_t                 IPTR
#define loff_t                      IPTR
#define pgoff_t                     IPTR
#define pgprot_t                    ULONG
#define irqreturn_t                 ULONG
#define INT_MAX                     2147483647
#define U64_MAX                     ULLONG_MAX
#define __le16                      WORD /* WRONG! IT WILL ONLY WORK ON LE MACHINES */

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({          \
    const typeof(((type *)0)->member) *__mptr = (ptr);    \
             (type *)((char *)__mptr - offsetof(type, member)); })

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define	DECLARE_BITMAP(n, bits) \
    unsigned long n[howmany(bits, sizeof(long) * 8)]


#define IRQ_NONE    0
#define IRQ_HANDLED 1

typedef struct
{
    LONG count;
} atomic_t;

#ifndef _AROS_TYPES_SPINLOCK_S_H_
typedef struct
{
    ULONG dummy;
} spinlock_t;
#endif

typedef struct
{
    LONG dummy;
} rwlock_t;

/* Page handling */
struct page
{
    APTR address;
    APTR allocated_buffer;
};

#undef PAGE_SIZE
#define PAGE_SHIFT              12
#define PAGE_SIZE               ((1UL) << PAGE_SHIFT)
#define PAGE_MASK               (~(PAGE_SIZE-1))
#define PAGE_ALIGN(addr)        (typeof(addr))(((IPTR)(addr) + PAGE_SIZE - 1) & PAGE_MASK)
#define ALIGN(val, align)       (((val) + (align) - 1) & (~((align) - 1)))
#define ALIGN_DOWN(val, align)  (((val)) & (~((align) - 1)))
#define IS_ALIGNED(x, n)        ((x & ((typeof(x))(n) - 1)) == 0)
#define __aligned(x)            __attribute__((__alligned__(x)))


/* io_mapping support */
struct io_mapping
{
    IPTR address;
};

/* AGP support */
struct agp_bridge_data
{
    IPTR agpbridgedevice;
    ULONG mode;
    IPTR aperturebase;
    ULONG aperturesize;
};

#define NOT_SUPPORTED           0
#define SUPPORTED               1
struct agp_kern_info
{
    int chipset;
    int cant_use_aperture;
    unsigned long aper_base;
    unsigned long aper_size;
    unsigned long mode;
    unsigned long page_mask;
};
struct agp_memory
{
    struct page **pages;
    size_t page_count;
    BOOL is_flushed;
    BOOL is_bound;
    ULONG type;
    ULONG pg_start;
};

#define AGP_USER_MEMORY         1
#define AGP_USER_CACHED_MEMORY  2


#define ERESTARTSYS             782434897 /* Just some random value */
#define ENOTSUPP                154335234 /* Just some random value */

/* Reference counted objects implementation */
struct kref
{
    atomic_t refcount;
};

/* Mutex emulation */
struct mutex
{
    struct SignalSemaphore semaphore;
    const char *name;
};

#define DEFINE_MUTEX(name) struct mutex name;   \
static int init_##name()                        \
{                                               \
    mutex_init(&name);                          \
    return TRUE;                                \
}                                               \
ADD2INIT(init_##name, 0)

/* IDR handling */
struct idr
{
    ULONG size;
    ULONG occupied;
    ULONG last_starting_id;
    IPTR * pointers;
};

/* I2C handling */
struct i2c_adapter
{
    IPTR i2cdriver;     /* OOP_Object * */
    APTR algo_data;
};

struct i2c_client;
struct i2c_board_info
{
    BYTE type[20]; /* Name? */
    UWORD addr;
};

struct i2c_driver
{
    ULONG dummy;
};

struct i2c_algo_bit_data
{
    void (*setsda)(void *data, int state);
    void (*setscl)(void *data, int state);
    int  (*getsda)(void *data);
    int  (*getscl)(void *data);
};

#define I2C_M_RD    0x0001

struct i2c_msg
{
    UWORD addr;
    UWORD flags;
    UWORD len;
    UBYTE *buf;
};

/* Wait queue handling */
typedef struct MinList wait_queue_head_t;

/* Firmware */
struct firmware
{
    ULONG size;
    APTR data;
};

/* Other */
struct work_struct
{
    ULONG dummy;
};
struct module;
struct edid;
struct clk;
struct drm_printer;
struct scatterlist;
struct sg_table
{
    struct scatterlist *sgl;
};
struct lock_class_key
{
    ULONG dummy;
};

struct device
{
    ULONG dummy;
};

#endif /* _DRM_COMPAT_TYPES_ */
