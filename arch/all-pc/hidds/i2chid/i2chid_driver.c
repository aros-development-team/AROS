/* Copyright (C) 2026, The AROS Development Team. All rights reserved.
 * HID-over-I2C transport and pointer report handling.
 */
#include <proto/exec.h>
#if defined(__AROSEXEC_SMP__)
#include <proto/execlock.h>
#include <aros/types/spinlock_s.h>
#endif
#include <exec/execbase.h>
#include <exec/semaphores.h>
#include <proto/dos.h>
#include <proto/acpica.h>
#include <proto/kernel.h>
#define __INPUT_NOLIBBASE__
#include <proto/input.h>
#include <devices/timer.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <resources/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "i2chid_core.h"
#include "i2chid_driver.h"
#include <stdarg.h>

struct driver
{
    ACPI_HANDLE device, controller, gpio;
    uint32_t base, length, gbase, glength, speed;
    uint16_t address, pin, descriptor;
    struct MsgPort *timer_port, *input_port;
    struct timerequest *timer;
    struct IOStdReq *input;
    int timer_open, input_open, saved, stopping, timer_error;
    unsigned buttons;
    uint32_t old[7];
    struct ih_bus bus;
    struct ih_layout layout;
    const struct ih_sink *sink;
    const char *report_path;
};
/* DOS output is local to the worker process; do not share CRT stdout. */
static int driver_log(const char *format, ...)
{
    char text[512];
    va_list ap;
    va_start(ap, format);
    int n = vsnprintf(text, sizeof(text), format, ap);
    va_end(ap);
    if (n > 0 && Output())
    {
        Write(Output(), text, n < (int)sizeof(text) ? n : (int)sizeof(text) - 1);
    }
    return n;
}
static int driver_puts(const char *s)
{
    return driver_log("%s\n", s);
}
#define printf driver_log
#define puts driver_puts
#define fflush(stream) ((void)0)
static const unsigned save_regs[] = {0x00, 0x04, 0x30, 0x38, 0x3c, 0x6c, 0x7c};

static int stage(const char *message)
{
    printf("I2CTouchpad: %s\n", message);
    fflush(stdout);
    return !(SetSignal(0, 0) & SIGBREAKF_CTRL_C);
}
static int evaluate(ACPI_HANDLE h, const char *name, ACPI_OBJECT_LIST *args, ACPI_BUFFER *out)
{
    ACPI_STATUS s;
    if (!stage(name))
    {
        return 0;
    }
    out->Length = ACPI_ALLOCATE_BUFFER;
    out->Pointer = NULL;
    s = AcpiEvaluateObject(h, (char *)name, args, out);
    if (ACPI_FAILURE(s))
    {
        printf("ACPI status 0x%lx\n", (unsigned long)s);
        return 0;
    }
    return out->Pointer != NULL;
}
static int is_string(const ACPI_OBJECT *o, const char *s)
{
    if (o->Type == ACPI_TYPE_STRING)
    {
        return o->String.Length == strlen(s) && !memcmp(o->String.Pointer, s, o->String.Length);
    }
    if (o->Type == ACPI_TYPE_PACKAGE)
    {
        for (unsigned i = 0; i < o->Package.Count; i++)
        {
            if (o->Package.Elements[i].Type == ACPI_TYPE_STRING &&
                is_string(&o->Package.Elements[i], s))
            {
                return 1;
            }
        }
    }
    return 0;
}
static int id_matches(ACPI_HANDLE h, const char *method, const char *id)
{
    ACPI_BUFFER b;
    int ok;
    if (!evaluate(h, method, NULL, &b))
    {
        return 0;
    }
    ok = is_string(b.Pointer, id);
    FreeVec(b.Pointer);
    return ok;
}
static int present(ACPI_HANDLE h)
{
    ACPI_BUFFER b = {ACPI_ALLOCATE_BUFFER, NULL};
    ACPI_STATUS status = AcpiEvaluateObject(h, "_STA", NULL, &b);
    int ok;

    /* ACPI specifies all status bits set when _STA is absent. */
    if (status == AE_NOT_FOUND)
    {
        return 1;
    }
    if (ACPI_FAILURE(status))
    {
        return 0;
    }
    ACPI_OBJECT *o = b.Pointer;
    ok = o && o->Type == ACPI_TYPE_INTEGER && (o->Integer.Value & 0x0b) == 0x0b;
    FreeVec(b.Pointer);
    return ok;
}
static int resources(struct driver *d, ACPI_HANDLE h, int kind)
{
    ACPI_BUFFER b = {ACPI_ALLOCATE_BUFFER, NULL};
    ACPI_STATUS status;
    unsigned char *at, *end;
    int found = 0, gpio_found = 0;
    if (!stage("_CRS"))
    {
        return 0;
    }
    status = AcpiGetCurrentResources(h, &b);
    if (ACPI_FAILURE(status) || !b.Pointer)
    {
        return 0;
    }
    at = b.Pointer;
    end = at + b.Length;
    while ((size_t)(end - at) >= 8)
    {
        ACPI_RESOURCE *r = (ACPI_RESOURCE *)at;
        if (r->Length < 8 || r->Length > (size_t)(end - at))
        {
            break;
        }
        if (r->Type == ACPI_RESOURCE_TYPE_END_TAG)
        {
            break;
        }
        if (kind == 0 && r->Type == ACPI_RESOURCE_TYPE_SERIAL_BUS &&
            r->Data.CommonSerialBus.Type == ACPI_RESOURCE_SERIAL_TYPE_I2C)
        {
            ACPI_RESOURCE_I2C_SERIALBUS *s = &r->Data.I2cSerialBus;
            if (found || s->AccessMode != ACPI_I2C_7BIT_MODE || s->SlaveMode != 0 ||
                s->SlaveAddress < 8 || s->SlaveAddress > 0x77 || !s->ResourceSource.StringPtr ||
                s->ResourceSource.StringLength < 2)
            {
                goto bad;
            }
            if (ACPI_FAILURE(AcpiGetHandle(h, s->ResourceSource.StringPtr, &d->controller)))
            {
                goto bad;
            }
            d->address = s->SlaveAddress;
            d->speed = s->ConnectionSpeed;
            found = 1;
        }
        if (kind == 0 && r->Type == ACPI_RESOURCE_TYPE_GPIO)
        {
            ACPI_RESOURCE_GPIO *s = &r->Data.Gpio;
            if (gpio_found || s->ConnectionType != ACPI_RESOURCE_GPIO_TYPE_INT ||
                s->PinTableLength != 1 || s->Polarity != ACPI_ACTIVE_LOW ||
                s->Triggering != ACPI_LEVEL_SENSITIVE || !s->ResourceSource.StringPtr)
            {
                goto bad;
            }
            if (ACPI_FAILURE(AcpiGetHandle(h, s->ResourceSource.StringPtr, &d->gpio)))
            {
                goto bad;
            }
            d->pin = s->PinTable[0];
            gpio_found = 1;
        }
        if (kind && r->Type == ACPI_RESOURCE_TYPE_FIXED_MEMORY32)
        {
            if (found)
            {
                goto bad;
            }
            if (kind == 1)
            {
                d->base = r->Data.FixedMemory32.Address;
                d->length = r->Data.FixedMemory32.AddressLength;
            }
            else
            {
                d->gbase = r->Data.FixedMemory32.Address;
                d->glength = r->Data.FixedMemory32.AddressLength;
            }
            found = 1;
        }
        at += r->Length;
    }
    FreeVec(b.Pointer);
    return found && (kind || gpio_found);
bad:
    FreeVec(b.Pointer);
    return 0;
}
static int discover_device(struct driver *d, ACPI_HANDLE device)
{
    ACPI_BUFFER b;
    ACPI_OBJECT *o;
    d->device = device;
    if (!present(device))
    {
        return 0;
    }
    if (!resources(d, d->device, 0) || !id_matches(d->controller, "_HID", "AMDI0010") ||
        !id_matches(d->gpio, "_HID", "AMDI0030") || !present(d->controller) || !present(d->gpio) ||
        !resources(d, d->controller, 1) || !resources(d, d->gpio, 2))
    {
        return 0;
    }
    if (d->length < 0x100 || !d->base || !d->gbase || (d->base & 4095) ||
        (uint64_t)d->base + d->length > UINT32_MAX ||
        (uint64_t)d->gbase + d->glength > UINT32_MAX || d->glength < 4 ||
        (unsigned)d->pin * 4 > d->glength - 4 || d->speed < 100000 || d->speed > 400000)
    {
        return 0;
    }
    /* HID-I2C descriptor address, revision 1/function 1. */
    uint8_t uuid[] = {0xf7, 0xf6, 0xdf, 0x3c, 0x67, 0x42, 0x55, 0x45,
                      0xad, 0x05, 0xb3, 0x0a, 0x3d, 0x89, 0x38, 0xde};
    ACPI_OBJECT a[4];
    ACPI_OBJECT_LIST args = {4, a};
    memset(a, 0, sizeof(a));
    a[0].Type = ACPI_TYPE_BUFFER;
    a[0].Buffer.Length = 16;
    a[0].Buffer.Pointer = uuid;
    a[1].Type = ACPI_TYPE_INTEGER;
    a[1].Integer.Value = 1;
    a[2].Type = ACPI_TYPE_INTEGER;
    a[2].Integer.Value = 1;
    a[3].Type = ACPI_TYPE_PACKAGE;
    if (!evaluate(d->device, "_DSM", &args, &b))
    {
        return 0;
    }
    o = b.Pointer;
    int ok = o->Type == ACPI_TYPE_INTEGER && o->Integer.Value <= 65535;
    if (ok)
    {
        d->descriptor = o->Integer.Value;
    }
    FreeVec(o);
    printf("I2C: MMIO=%08lx length=%lx address=%02x speed=%lu descriptor=%04x\n",
           (unsigned long)d->base, (unsigned long)d->length, d->address, (unsigned long)d->speed,
           d->descriptor);
    printf("GPIO: MMIO=%08lx pin=%u (active-low, polled)\n", (unsigned long)d->gbase, d->pin);
    return ok;
}
/* ACPICA normalizes integer EISA IDs and searches both _HID and _CID. */
static int hid_i2c_device(ACPI_HANDLE device)
{
    ACPI_DEVICE_INFO *info;
    int match = 0;

    if (ACPI_FAILURE(AcpiGetObjectInfo(device, &info)))
    {
        return 0;
    }
    if (info->Valid & ACPI_VALID_HID)
    {
        match = !strcmp(info->HardwareId.String, "PNP0C50") ||
                !strcmp(info->HardwareId.String, "ACPI0C50");
    }
    if (info->Valid & ACPI_VALID_CID)
    {
        for (UINT32 i = 0; i < info->CompatibleIdList.Count; i++)
        {
            const char *id = info->CompatibleIdList.Ids[i].String;
            match |= !strcmp(id, "PNP0C50") || !strcmp(id, "ACPI0C50");
        }
    }
    FreeVec(info);
    return match;
}

static ACPI_STATUS discover_candidate(ACPI_HANDLE device, UINT32 level, void *context,
                                      void **result)
{
    struct driver *d = context;
    (void)level;
    if (!discover_device(d, device))
    {
        return AE_OK;
    }
    *result = d;
    return AE_CTRL_TERMINATE;
}

static int discover(struct driver *d, const char *path)
{
    ACPI_HANDLE device;
    void *found = NULL;

    if (path)
    {
        if (ACPI_FAILURE(AcpiGetHandle(NULL, (char *)path, &device)) || !hid_i2c_device(device))
        {
            return 0;
        }
        return discover_device(d, device);
    }
    AcpiGetDevices("PNP0C50", discover_candidate, d, &found);
    if (!found)
    {
        AcpiGetDevices("ACPI0C50", discover_candidate, d, &found);
    }
    return found != NULL;
}

/* One persistent public semaphore per controller. Never remove it: a caller
 * may retain the pointer after leaving the list lock. This also lets the CLI
 * and the resident HIDD arbitrate without shared library-private state.
 */
static struct SignalSemaphore *claim_controller(uint32_t address)
{
    struct controller_lock
    {
        struct SignalSemaphore semaphore;
        char name[40];
    } *candidate;
    struct SignalSemaphore *semaphore;
#if defined(__AROSEXEC_SMP__)
    APTR ExecLockBase = OpenResource((CONST_STRPTR) "execlock.resource");
#endif

    candidate = AllocVec(sizeof(*candidate), MEMF_PUBLIC | MEMF_CLEAR);
    if (!candidate)
    {
        return NULL;
    }
    snprintf(candidate->name, sizeof(candidate->name), "i2chid.controller.%08lx",
             (unsigned long)address);
    candidate->semaphore.ss_Link.ln_Name = candidate->name;
    InitSemaphore(&candidate->semaphore);

#if defined(__AROSEXEC_SMP__)
    /* Forbid alone excludes only the local CPU. Fail closed if no list lock. */
    if (!ExecLockBase ||
        !ObtainSystemLock(&SysBase->SemaphoreList, SPINLOCK_MODE_WRITE, LOCKF_FORBID))
    {
        FreeVec(candidate);
        return NULL;
    }
#else
    Forbid();
#endif

    semaphore =
        (struct SignalSemaphore *)FindName(&SysBase->SemaphoreList, (CONST_STRPTR)candidate->name);
    if (!semaphore)
    {
        semaphore = &candidate->semaphore;
        /* Already initialized; AddSemaphore would take this same list lock. */
        semaphore->ss_Link.ln_Type = NT_SIGNALSEM;
        Enqueue(&SysBase->SemaphoreList, &semaphore->ss_Link);
        candidate = NULL;
    }
#if defined(__AROSEXEC_SMP__)
    ReleaseSystemLock(&SysBase->SemaphoreList, LOCKF_FORBID);
#else
    Permit();
#endif
    if (candidate)
    {
        FreeVec(candidate);
    }
    return AttemptSemaphore(semaphore) ? semaphore : NULL;
}

static uint32_t rd(void *ctx, unsigned reg)
{
    struct driver *d = ctx;
    return *(volatile uint32_t *)(uintptr_t)(d->base + reg);
}
static void wr(void *ctx, unsigned reg, uint32_t value)
{
    struct driver *d = ctx;
    *(volatile uint32_t *)(uintptr_t)(d->base + reg) = value;
}
static int pause_us(struct driver *d, unsigned us)
{
    d->timer->tr_node.io_Command = TR_ADDREQUEST;
    d->timer->tr_time.tv_secs = 0;
    d->timer->tr_time.tv_micro = us;
    if (DoIO((struct IORequest *)d->timer))
    {
        d->timer_error = 1;
        return 1;
    }
    return !d->stopping && !!(SetSignal(0, 0) & SIGBREAKF_CTRL_C);
}
static int poll_pause(void *ctx)
{
    return pause_us(ctx, 25);
}
static int disable(struct driver *d)
{
    wr(d, 0x6c, 0);
    for (unsigned i = 0; i < 1000; i++)
    {
        if (!(rd(d, 0x9c) & 1))
        {
            return 1;
        }
        if (pause_us(d, 25))
        {
            return 0;
        }
    }
    return 0;
}
static int pending(struct driver *d)
{
    return !(*(volatile uint32_t *)(uintptr_t)(d->gbase + 4 * d->pin) & (1u << 16));
}
static int map_mmio(uint32_t address, uint32_t length)
{
    /* Match PCIPC MapPCI: the PC identity map uses 2 MiB pages.
     * KrnMapGlobal cannot insert a 4 KiB mapping into an existing large page.
     */
    APTR KernelBase = OpenResource((CONST_STRPTR) "kernel.resource");
    if (!KernelBase)
    {
        return 0;
    }
    const uintptr_t mask = 0x1fffff;
    uintptr_t start = address & ~mask, end = ((uintptr_t)address + length + mask) & ~mask;
    int result = KrnMapGlobal((void *)start, (void *)start, end - start,
                              MAP_Readable | MAP_Writable | MAP_CacheInhibit);
    printf("MMIO mapping %08lx-%08lx result=%d\n", (unsigned long)start, (unsigned long)(end - 1),
           result);
    fflush(stdout);
    return result == 0;
}
static int acquire_controller(struct driver *d, int check_only)
{
    if (!map_mmio(d->base, d->length))
    {
        puts("I2C MMIO mapping failed.");
        return 0;
    }
    if (!map_mmio(d->gbase, d->glength))
    {
        puts("GPIO MMIO mapping failed.");
        return 0;
    }
    /* These status/configuration reads do not acknowledge interrupts. */
    uint32_t type = rd(d, 0xfc);
    printf("Controller COMP_TYPE=%08lx\n", (unsigned long)type);
    fflush(stdout);
    if (type != 0x44570140)
    {
        puts("Unsupported or inaccessible DesignWare controller.");
        return 0;
    }
    uint32_t status = rd(d, 0x70), mask = rd(d, 0x30);
    uint32_t gpio = *(volatile uint32_t *)(uintptr_t)(d->gbase + 4 * d->pin);
    unsigned timing = d->speed <= 100000 ? 0x14 : 0x1c;
    uint32_t high = rd(d, timing), low = rd(d, timing + 4);
    printf("Controller STATUS=%08lx ENABLE=%08lx ENABLE_STATUS=%08lx INTR_MASK=%08lx\n",
           (unsigned long)status, (unsigned long)rd(d, 0x6c), (unsigned long)rd(d, 0x9c),
           (unsigned long)mask);
    printf("GPIO pin=%u value=%08lx; SCL HCNT=%lu LCNT=%lu\n", d->pin, (unsigned long)gpio,
           (unsigned long)high, (unsigned long)low);
    fflush(stdout);
    if (status & 0x21)
    {
        puts("Controller reports bus activity.");
        return 0;
    }
    /* GPIO direction must already be input. Never alter GPIO/IRQ configuration. */
    if (gpio & (1u << 23))
    {
        puts("GPIO pin is configured as an output.");
        return 0;
    }
    if (mask)
    {
        puts("Controller interrupts enabled; refusing to take over an active driver.");
        return 0;
    }
    if (high < 6 || low < 8)
    {
        puts("Firmware SCL timing absent; refusing to guess clock rate.");
        return 0;
    }
    if (check_only)
    {
        puts("Controller checks passed; no controller registers changed.");
        return 1;
    }
    for (unsigned i = 0; i < 7; i++)
    {
        d->old[i] = rd(d, save_regs[i]);
    }
    d->saved = 1;
    if (!disable(d))
    {
        puts("Controller disable failed or was interrupted.");
        return 0;
    }
    wr(d, 0x00, 0x61 | (d->speed <= 100000 ? 2 : 4));
    wr(d, 0x30, 0);
    uint32_t param = rd(d, 0xf4);
    d->bus.context = d;
    d->bus.read = rd;
    d->bus.write = wr;
    d->bus.pause = poll_pause;
    d->bus.address = d->address;
    d->bus.rx_depth = ((param >> 8) & 255) + 1;
    d->bus.tx_depth = ((param >> 16) & 255) + 1;
    return 1;
}
static int read_register(struct driver *d, uint16_t reg, uint8_t *buf, unsigned length)
{
    uint8_t w[] = {reg & 255, reg >> 8};
    return ih_transfer(&d->bus, w, 2, buf, length);
}
static int command(struct driver *d, uint16_t reg, uint8_t op, uint8_t arg)
{
    uint8_t w[] = {reg & 255, reg >> 8, arg, op};
    return ih_transfer(&d->bus, w, 4, NULL, 0);
}
static int event(struct driver *d, int x, int y, unsigned code)
{
    if (d->sink && d->sink->event)
    {
        unsigned type = IH_MOTION, button = 0;
        if (code != IECODE_NOBUTTON)
        {
            button = (code & ~IECODE_UP_PREFIX) - IECODE_LBUTTON + 1;
            type = (code & IECODE_UP_PREFIX) ? IH_RELEASE : IH_PRESS;
        }
        if (!d->sink->event(d->sink->context, x, y, type, button))
        {
            return 0;
        }
        if (button && button <= 3)
        {
            if (type == IH_PRESS)
            {
                d->buttons |= 1u << (button - 1);
            }
            else
            {
                d->buttons &= ~(1u << (button - 1));
            }
        }
        return 1;
    }
    struct InputEvent e;
    memset(&e, 0, sizeof(e));
    e.ie_Class = IECLASS_RAWMOUSE;
    struct Device *InputBase = d->input->io_Device;
    unsigned buttons = d->buttons;
    if (code != IECODE_NOBUTTON)
    {
        unsigned button = (code & ~IECODE_UP_PREFIX) - IECODE_LBUTTON;
        if (button < 3)
        {
            if (code & IECODE_UP_PREFIX)
            {
                buttons &= ~(1u << button);
            }
            else
            {
                buttons |= 1u << button;
            }
        }
    }
    e.ie_Code = code;
    e.ie_Qualifier = (PeekQualifier() &
                      ~(IEQUALIFIER_LEFTBUTTON | IEQUALIFIER_RBUTTON | IEQUALIFIER_MIDBUTTON)) |
                     IEQUALIFIER_RELATIVEMOUSE;
    if (buttons & 1)
    {
        e.ie_Qualifier |= IEQUALIFIER_LEFTBUTTON;
    }
    if (buttons & 2)
    {
        e.ie_Qualifier |= IEQUALIFIER_RBUTTON;
    }
    if (buttons & 4)
    {
        e.ie_Qualifier |= IEQUALIFIER_MIDBUTTON;
    }
    e.ie_X = x;
    e.ie_Y = y;
    d->input->io_Command = IND_WRITEEVENT;
    d->input->io_Data = &e;
    d->input->io_Length = sizeof(e);
    if (DoIO((struct IORequest *)d->input))
    {
        return 0;
    }
    d->buttons = buttons;
    return 1;
}
static int input_loop(struct driver *d)
{
    struct ih_descriptor hid;
    uint8_t raw[IH_MAX_REPORT], *desc = NULL;
    int result = 0, powered = 0;
    struct ih_sample previous = {0};
    int tracking = 0;
    int64_t remainder_x = 0, remainder_y = 0;
    if (!read_register(d, d->descriptor, raw, 30) || !ih_descriptor(raw, 30, &hid))
    {
        goto out;
    }
    printf("HID vendor=%04x product=%04x report=%u input=%u\n", hid.vendor, hid.product,
           hid.report_length, hid.input_length);
    if (!command(d, hid.command_register, 8, 0))
    {
        goto out;
    }
    powered = 1;
    if (pause_us(d, 1000) || !command(d, hid.command_register, 1, 0))
    {
        goto out;
    }
    /* Reset completion is a zero length input, normally signalled by GPIO. */
    int reset = 0;
    for (unsigned i = 0; i < 100; i++)
    {
        if (pending(d))
        {
            if (!ih_transfer(&d->bus, NULL, 0, raw, hid.input_length))
            {
                goto out;
            }
            if (!raw[0] && !raw[1])
            {
                reset = 1;
                break;
            }
        }
        if (pause_us(d, 10000))
        {
            goto out;
        }
    }
    if (!reset)
    {
        puts("HID reset completion timed out.");
        goto out;
    }
    desc = AllocVec(hid.report_length, MEMF_ANY);
    if (!desc)
    {
        goto out;
    }
    if (!read_register(d, hid.report_register, desc, hid.report_length))
    {
        goto out;
    }
    /* Descriptor capture is an explicit CLI request, never a boot side effect. */
    if (d->report_path)
    {
        BPTR capture = Open((CONST_STRPTR)d->report_path, MODE_NEWFILE);
        if (capture)
        {
            if (Write(capture, desc, hid.report_length) != hid.report_length)
            {
                puts("Descriptor capture write failed.");
            }
            Close(capture);
        }
        else
        {
            puts("Could not open descriptor capture file.");
        }
    }
    if (!ih_parse(desc, hid.report_length, &d->layout))
    {
        puts("Unsupported or malformed pointer report descriptor; use REPORT to capture it.");
        goto out;
    }
    if (d->sink && d->sink->ready && !d->sink->ready(d->sink->context))
    {
        goto out;
    }
    puts("Pointer input running. Ctrl-C stops and restores controller configuration.");
    fflush(stdout);
    for (;;)
    {
        struct ih_sample sample;
        int decoded;
        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
        {
            result = 1;
            break;
        }
        if (!pending(d))
        {
            if (pause_us(d, 10000))
            {
                result = 1;
                break;
            }
            continue;
        }
        if (!ih_transfer(&d->bus, NULL, 0, raw, hid.input_length))
        {
            goto out;
        }
        decoded = ih_decode(&d->layout, raw, hid.input_length, &sample);
        if (decoded < 0)
        {
            puts("Malformed input report; stopping.");
            goto out;
        }
        if (decoded > 0)
        {
            int64_t dx = 0, dy = 0;
            if (sample.relative)
            {
                dx = sample.x;
                dy = sample.y;
                remainder_x = remainder_y = 0;
            }
            else if (sample.contact && tracking && sample.id == previous.id)
            {
                int64_t sx = ((int64_t)sample.x - previous.x) * 1024 + remainder_x;
                int64_t sy = ((int64_t)sample.y - previous.y) * 768 + remainder_y;
                dx = sx / sample.xrange;
                remainder_x = sx % sample.xrange;
                dy = sy / sample.yrange;
                remainder_y = sy % sample.yrange;
            }
            else
            {
                remainder_x = remainder_y = 0;
            }
            tracking = sample.contact;
            previous = sample;
            if (dx > 32767)
            {
                dx = 32767;
            }
            if (dx < -32768)
            {
                dx = -32768;
            }
            if (dy > 32767)
            {
                dy = 32767;
            }
            if (dy < -32768)
            {
                dy = -32768;
            }
            if ((dx || dy) && !event(d, dx, dy, IECODE_NOBUTTON))
            {
                goto out;
            }
            for (unsigned i = 0; i < 3; i++)
            {
                if ((sample.buttons ^ d->buttons) & (1u << i))
                {
                    if (!event(d, 0, 0,
                               (IECODE_LBUTTON + i) |
                                   ((sample.buttons & (1u << i)) ? 0 : IECODE_UP_PREFIX)))
                    {
                        goto out;
                    }
                }
            }
        }
        if (pause_us(d, 1000))
        {
            result = 1;
            break;
        }
    }
out:
    d->stopping = 1;
    for (unsigned i = 0; i < 3; i++)
    {
        if (d->buttons & (1u << i))
        {
            (void)event(d, 0, 0, (IECODE_LBUTTON + i) | IECODE_UP_PREFIX);
        }
    }
    uint32_t failure = d->bus.failure;
    if (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
    {
        result = 1;
    }
    if (powered)
    {
        (void)command(d, hid.command_register, 8, 1);
    }
    if (desc)
    {
        FreeVec(desc);
    }
    if (d->timer_error)
    {
        puts("Timer request failed.");
        result = 0;
    }
    if (!result)
    {
        printf("Stopped, controller failure=0x%lx\n", (unsigned long)failure);
    }
    return result;
}
int ih_driver_run(const char *path, int run, int check, const struct ih_sink *sink,
                  const char *report_path)
{
    int result = RETURN_FAIL;
    struct SignalSemaphore *lease = NULL;
    struct driver *d = AllocVec(sizeof(*d), MEMF_PUBLIC | MEMF_CLEAR);
    if (!d)
    {
        return RETURN_FAIL;
    }
    d->sink = sink;
    d->report_path = report_path;
    if (!discover(d, path))
    {
        puts("Discovery incomplete; no controller registers changed.");
        goto out;
    }
    if (!run && !check)
    {
        puts("Discovery only. RUN starts pointer input.");
        result = RETURN_OK;
        goto out;
    }
    lease = claim_controller(d->base);
    if (!lease)
    {
        puts("Controller lease unavailable; another instance may be running.");
        goto out;
    }
    d->timer_port = CreateMsgPort();
    if (!d->timer_port)
    {
        goto out;
    }
    d->timer = (struct timerequest *)CreateIORequest(d->timer_port, sizeof(*d->timer));
    if (!d->timer)
    {
        goto out;
    }
    if (OpenDevice((CONST_STRPTR) "timer.device", UNIT_MICROHZ, (struct IORequest *)d->timer, 0))
    {
        goto out;
    }
    d->timer_open = 1;
    if (!sink || !sink->event)
    {
        d->input_port = CreateMsgPort();
        if (!d->input_port)
        {
            goto out;
        }
        d->input = (struct IOStdReq *)CreateIORequest(d->input_port, sizeof(*d->input));
        if (!d->input)
        {
            goto out;
        }
        if (OpenDevice((CONST_STRPTR) "input.device", 0, (struct IORequest *)d->input, 0))
        {
            goto out;
        }
        d->input_open = 1;
    }
    if (!acquire_controller(d, check))
    {
        puts("Controller preconditions failed.");
        goto out;
    }
    if (check)
    {
        result = RETURN_OK;
        goto out;
    }
    result = input_loop(d) ? RETURN_OK : RETURN_FAIL;
out:
    d->stopping = 1;
    if (d->saved)
    {
        if (disable(d))
        {
            for (unsigned i = 0; i < 7; i++)
            {
                if (save_regs[i] != 0x6c)
                {
                    wr(d, save_regs[i], d->old[i]);
                }
            }
            wr(d, 0x6c, d->old[5] & 1);
        }
        else
        {
            puts("Controller did not disable; restoration incomplete.");
            result = RETURN_FAIL;
        }
    }
    if (d->input_open)
    {
        CloseDevice((struct IORequest *)d->input);
    }
    if (d->timer_open)
    {
        CloseDevice((struct IORequest *)d->timer);
    }
    if (d->input)
    {
        DeleteIORequest((struct IORequest *)d->input);
    }
    if (d->timer)
    {
        DeleteIORequest((struct IORequest *)d->timer);
    }
    if (d->input_port)
    {
        DeleteMsgPort(d->input_port);
    }
    if (d->timer_port)
    {
        DeleteMsgPort(d->timer_port);
    }
    if (lease)
    {
        ReleaseSemaphore(lease);
    }
    FreeVec(d);
    return result;
}
