/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _DRM_COMPAT_TYPES_
#define _DRM_COMPAT_TYPES_

#include <exec/types.h>
#include <exec/semaphores.h>
#include <sys/types.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>

#define __user
#define __iomem
#define __force
#define __must_check
#define __u32                       ULONG
#define __s32                       LONG
#define __u16                       UWORD
#define __u64                       UQUAD
#define u16                         UWORD
#define s16                         WORD
#define u32                         ULONG
#define s32                         LONG
#define u64                         UQUAD
#define s64                         QUAD
#define u8                          UBYTE
#define resource_size_t             IPTR
#define dma_addr_t                  IPTR
#define loff_t                      IPTR
#define pgprot_t                    ULONG
#define INT_MAX                     2147483647
#define __le16                      WORD /* WRONG! IT WILL ONLY WORK ON LE MACHINES */

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({          \
    const typeof(((type *)0)->member) *__mptr = (ptr);    \
             (type *)((char *)__mptr - offsetof(type, member)); })

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

typedef struct
{
    LONG count;
} atomic_t;

typedef struct
{
    /* atomic_t lock; Does not work - causes deadlock */
    struct SignalSemaphore semaphore;
} spinlock_t;

typedef struct
{
    /* atomic_t lock; Does not work - causes deadlock */
    struct SignalSemaphore semaphore;
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
#define PAGE_ALIGN(addr)        (APTR)(((IPTR)(addr) + PAGE_SIZE - 1) & PAGE_MASK)
#define ALIGN(val, align)       (val + align - 1) & (~(align - 1))
#define BITS_TO_LONGS(x)        ((x / (sizeof(long) * 8)) + 1)

/* PCI support */
struct pci_dev
{
    ULONG class;
    APTR oopdev;
    TEXT name[16];
};

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

/* Reference counted objects implementation */
struct kref
{
    atomic_t refcount;
};

/* Mutex emulation */
struct mutex
{
    struct SignalSemaphore semaphore;
};

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
struct wait_queue_head
{
    ULONG dummy;
};
typedef struct wait_queue_head wait_queue_head_t;

/* Firmware */
struct firmware
{
    ULONG size;
    APTR data;
};

/* Other */
struct work_struct;
struct module;
struct edid;

#endif /* _DRM_COMPAT_TYPES_ */
