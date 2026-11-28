#include <aros/debug.h>
#include <devices/usbhardware.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include "uhwcmd.h"

#if __WORDSIZE == 64

/*
 * A transfer whose buffer lies above 4 GB goes through a bounce buffer that
 * the controller can address. Bounce buffers are taken and released in the
 * completion soft interrupt, where exec's allocator must not be called, so
 * they all come from one region that usbInitBuffers() sets aside in task
 * context. The region is a private MemHeader, and every Allocate() and
 * Deallocate() on it runs under Disable().
 */

#define USB_BOUNCE_REGION_SIZE  (4UL << 20)

/* Precedes every bounce buffer and keeps it on a 16 byte boundary */
struct UsbBounceHead {
    IPTR                ubh_Size;       /* What to give back to Deallocate() */
    IPTR                ubh_Pad;
};

static struct MemHeader *usbBounceHeader;

static inline BOOL usbNeedsBounce(APTR data, ULONG len)
{
    return len && (((IPTR)data + len - 1) >> 32) != 0;
}

void usbInitBuffers(void)
{
    struct MemHeader *mh;
    APTR region;

    /* Without memory outside of the 31-bit area no buffer can lie above 4 GB */
    if (AvailMem(MEMF_TOTAL) == AvailMem(MEMF_TOTAL|MEMF_31BIT))
        return;

    /* Allocate() needs the header cleared, it holds a spinlock */
    mh = AllocMem(sizeof(struct MemHeader), MEMF_PUBLIC|MEMF_CLEAR);
    region = AllocMem(USB_BOUNCE_REGION_SIZE, MEMF_31BIT|MEMF_PUBLIC);
    if (!mh || !region) {
        if (region)
            FreeMem(region, USB_BOUNCE_REGION_SIZE);
        if (mh)
            FreeMem(mh, sizeof(struct MemHeader));
        return;
    }

    mh->mh_Attributes = MEMF_31BIT|MEMF_PUBLIC;
    mh->mh_Lower = region;
    mh->mh_Upper = (UBYTE *)region + USB_BOUNCE_REGION_SIZE;
    mh->mh_First = region;
    mh->mh_First->mc_Next = NULL;
    mh->mh_First->mc_Bytes = USB_BOUNCE_REGION_SIZE;
    mh->mh_Free = USB_BOUNCE_REGION_SIZE;

    usbBounceHeader = mh;
}

void usbFreeBuffers(void)
{
    struct MemHeader *mh = usbBounceHeader;

    if (mh) {
        usbBounceHeader = NULL;
        FreeMem(mh->mh_Lower, USB_BOUNCE_REGION_SIZE);
        FreeMem(mh, sizeof(struct MemHeader));
    }
}

/* FALSE if usbGetBuffer() can never serve this buffer, not even with the whole region free */
BOOL usbBufferFits(APTR data, ULONG len)
{
    if (!usbNeedsBounce(data, len))
        return TRUE;

    return usbBounceHeader && (len <= USB_BOUNCE_REGION_SIZE - sizeof(struct UsbBounceHead));
}

/*
 * Returns data itself, a bounce buffer, or NULL if the buffer needs a bounce
 * and the region has no room for it at the moment.
 */
APTR usbGetBuffer(APTR data, ULONG len, UWORD dir)
{
    struct UsbBounceHead *head;
    IPTR size = sizeof(struct UsbBounceHead) + len;
    APTR ret;

    if (!usbNeedsBounce(data, len))
        return data;

    if (!usbBounceHeader)
        return NULL;

    Disable();
    head = Allocate(usbBounceHeader, size);
    Enable();

    if (!head)
        return NULL;

    head->ubh_Size = size;
    ret = head + 1;

    if (dir == UHDIR_OUT)
        CopyMem(data, ret, len);

    return ret;
}

void usbReleaseBuffer(APTR buffer, APTR data, ULONG len, UWORD dir)
{
    if (buffer && (buffer != data)) {
        struct UsbBounceHead *head = (struct UsbBounceHead *)buffer - 1;

        if (len && (dir == UHDIR_IN))
            CopyMem(buffer, data, len);

        Disable();
        Deallocate(usbBounceHeader, head, head->ubh_Size);
        Enable();
    }
}

#endif
