#include <exec/libraries.h>
#include <exec/semaphores.h>

/* ID components */
#define HPET_HW_REV_MASK		0x000000FF
#define HPET_NUM_COMPARATORS_MASK	0x00001F00
#define HPET_NUM_COMPARATORS_SHIFT	8
#define HPET_COUNTER_SIZE		0x00002000
#define HPET_LEGACY_REPLACEMENT		0x00008000
#define HPET_PCI_VENDOR_MASK		0xFFFF0000
#define HPET_PCI_VENDOR_SHIFT		16

/* page_protect components */
#define HPET_PAGE_PROTECT_MASK	0x0F
#define HPET_OEM_ATTR_MASK	0xF0
#define HPET_OEM_ATTR_SHIFT	4

#define HPET_PAGE_NONE	0
#define HPET_PAGE_4K	1
#define HPET_PAGE_64K	2


struct HPETUnit
{
    IPTR	base;
    IPTR	block;
    const char *Owner;
};

struct HPETBase
{
    struct Library          libnode;
    ULONG		    unitCnt;
    struct HPETUnit	   *units;
    struct SignalSemaphore  lock;
};
