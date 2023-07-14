#ifndef DWC2_INTERN_H
#define DWC2_INTERN_H

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/initializers.h>
#include <dos/dos.h>

#include <devices/timer.h>
#include <utility/utility.h>

#include <devices/usbhardware.h>
#include <devices/newstyle.h>

#include <oop/oop.h>

#include <asm/cpu.h>
#include <stdint.h>

#include <hardware/bcm2708.h>

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

/* The memory mapped IO base address */
extern IPTR __arm_periiobase;
#define ARM_PERIIOBASE __arm_periiobase

/* Reply the iorequest with success */
#define RC_OK	      0

/* Magic cookie, don't set error fields & don't reply the ioreq */
#define RC_DONTREPLY  -1

struct DWC2Unit
{
    struct Unit hu_Unit;
    UBYTE       hu_RootHubAddr;  // USB device address of the virtual root hub
    struct Channel
    {
        struct IOUsbHWReq *hc_Request;           // IO request currently being processed
        ULONG              hc_BytesTransferred;  // how much data was transferred in the last operation
        UBYTE              hc_PID;               // PID of last operation
        int                hc_ErrorCount;
        BOOL               hc_AckSent;           // For Control transfers, set after the 0-byte ACK was sent to the device.
    }           hu_Channels[16];
    short int   hu_NumChannels;
    UBYTE       hu_TransferSizeWidth;
    UBYTE       hu_PacketSizeWidth;
};

/* Private device node */
struct DWC2Device
{
    struct Library       hd_Library;  // base library struct
    // additional fields
    APTR                 hd_KernelBase;  // kernel.resource base
    APTR                 hd_UtilityBase; // utility.library base
    struct MsgPort      *hd_MsgPort;
    struct timerequest  *hd_TimerReq;
    struct DWC2Unit     *hd_Unit;
    APTR                 hd_MemPool;
};

#ifndef LIBBASETYPEPTR
typedef struct DWC2Device *LIBBASETYPEPTR;
#endif

/* Every driver function is passed an instance of DWC2Device called DWC2Base.
 * These macros are shorthand to access the utility and kernel libraries. */
#undef KernelBase
#define KernelBase DWC2Base->hd_KernelBase
#undef UtilityBase
#define UtilityBase DWC2Base->hd_UtilityBase

#define FNAME_DEV(x)            DWC2__Dev__ ## x

/* Register read and write functions. */

static inline uint32_t rd32le(volatile uint32_t *reg)
{
    uint32_t val;
    dmb();
    val = AROS_LE2LONG(*reg);
    dsb();
    return val;
}

static inline void wr32le(volatile uint32_t *reg, uint32_t val)
{
    dsb();
    *reg = val;
    dmb();
}

// Use for debug messages
#define DWC2_BUG(fmt, ...) D(bug("[DWC2] %s: " fmt "\n", __PRETTY_FUNCTION__, ##__VA_ARGS__))

static inline void hang(void)
{
    Disable();
    while (1)
    {
        asm volatile ("wfe");
    }
}

// dwc2.c
void dwc2_delay(unsigned int milliseconds);
BOOL dwc2_reset_root_port(void);
void dwc2_power_on_port(void);
void dwc2_power_off_port(void);

// dwc2_roothub.c
WORD dwc2_roothub_cmd_controlxfer(struct IOUsbHWReq *ioreq, struct DWC2Unit *unit, LIBBASETYPEPTR DWC2Base);

#define VCMB_PROPCHAN   8
#define VCPOWER_USBHCD  3
#define VCPOWER_STATE_ON    1
#define VCPOWER_STATE_WAIT  2

#define ENABLE_HIGH_SPEED 1

#endif
