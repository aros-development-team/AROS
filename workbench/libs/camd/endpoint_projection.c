/*
 * Legacy clusters of an endpoint a program publishes.
 *
 * The endpoint takes part in <name>.out.0 and <name>.in.0 the way a driver's
 * port does, through a DriverData of its own, so every existing program
 * reaches it by linking to those clusters. No driver is behind it: complete
 * messages pass between the clusters and the endpoint without becoming a byte
 * stream.
 */

#include <proto/exec.h>
#include <exec/memory.h>
#include <midi/camddevices.h>

#include <string.h>

#include "camd_intern.h"
#include "app_endpoint.h"

struct EndpointProjection {
    struct Drivers driver;
    struct MidiDeviceData device;
    struct CAMDAppEndpoint *app;
    struct CamdBase *CamdBase;
    char name[CAMD_ENDPOINT_NAME_BYTES];
};

/* CAMD opens and closes the port as links come and go; nothing to do. */
static SAVEDS ASM void projection_activate(REG(a2) APTR userdata,
                                           REG(d0) ULONG portnum)
{
    (void)userdata;
    (void)portnum;
}

static struct MidiPortData projection_port = {
    projection_activate
};

static SAVEDS ASM struct MidiPortData *projection_open(
    REG(a3) struct MidiDeviceData *data,
    REG(d0) LONG portnum,
    REG(a0) ULONG (* ASM transmitfunc)(APTR REG(a2) userdata),
    REG(a1) void (* ASM receivefunc)(UWORD REG(d0) input,APTR REG(a2) userdata),
    REG(a2) APTR userdata)
{
    (void)data;
    (void)portnum;
    (void)transmitfunc;
    (void)receivefunc;
    (void)userdata;
    return &projection_port;
}

static ASM void projection_close(REG(a3) struct MidiDeviceData *data,
                                 REG(d0) LONG portnum)
{
    (void)data;
    (void)portnum;
}

/* What the publisher emits, for the receivers linked to <name>.in.0. Runs in
 * the publisher's task with the endpoint locked, so one message at a time. */
static enum CAMDProviderResult project_midi1(
    void *context, const struct CAMDMIDI1EventV1 *events, size_t event_count)
{
    struct DriverData *driverdata = context;
    struct MyMidiMessage2 msg2;
    size_t i;

    for (i = 0; i < event_count; ++i) {
        msg2.status = events[i].Bytes[0];
        msg2.data1 = events[i].Bytes[1];
        msg2.data2 = events[i].Bytes[2];
        msg2.len = (UBYTE)events[i].Length;
        Receiver_SuperTreat2(driverdata, &msg2);
    }
    return CAMD_PROVIDER_OK;
}

static enum CAMDProviderResult project_sysex(
    void *context, const uint8_t *bytes, size_t byte_count,
    uint32_t time_high, uint32_t time_low, uint32_t clock_domain,
    uint32_t flags)
{
    struct DriverData *driverdata = context;
    size_t i;

    (void)time_high;
    (void)time_low;
    (void)clock_domain;
    (void)flags;
    for (i = 0; i < byte_count; ++i)
        Receiver_SysExSuperTreat(driverdata, bytes[i]);
    return CAMD_PROVIDER_OK;
}

/* The bytes of a channel or system message, 0 for what is not one. */
static ULONG message_length(UBYTE status)
{
    if (status < 0x80)
        return 0;
    if (status >= 0xf8)
        return 1;
    if (status >= 0xf0) {
        if (status == 0xf1 || status == 0xf3)
            return 2;
        if (status == 0xf2)
            return 3;
        return status == 0xf6 ? 1 : 0;
    }
    return (status & 0xe0) == 0xc0 ? 2 : 3;
}

/* What a legacy sender put in <name>.out.0, from Midi2Driver_internal(). A
 * publisher that does not read loses messages; senders are never held up. */
BOOL ProjectMidiToEndpoint(struct DriverData *driverdata, ULONG msg)
{
    struct CAMDMIDI1EventV1 event;

    memset(&event, 0, sizeof(event));
    event.Size = sizeof(event);
    event.Version = 1;
    event.Bytes[0] = (UBYTE)(msg >> 24);
    event.Bytes[1] = (UBYTE)(msg >> 16);
    event.Bytes[2] = (UBYTE)(msg >> 8);
    event.Length = message_length(event.Bytes[0]);
    if (event.Length == 0)
        return TRUE;
    if (event.Length < 3)
        event.Bytes[2] = 0;
    if (event.Length < 2)
        event.Bytes[1] = 0;
    if (camd_app_endpoint_inject_midi1(driverdata->Projection, &event) !=
        CAMD_PROVIDER_OK)
        ++driverdata->EndpointInputDropped;
    return TRUE;
}

/* A complete SysEx message from SysEx2Driver(). */
BOOL ProjectSysExToEndpoint(struct DriverData *driverdata, UBYTE *buffer)
{
    if (camd_app_endpoint_inject_sysex(driverdata->Projection, buffer,
                                       (size_t)GetSysXLen(buffer)) !=
        CAMD_PROVIDER_OK)
        ++driverdata->EndpointInputDropped;
    driverdata->issending_sx = 0;
    return TRUE;
}

/* Must not hold CLSemaphore. */
struct EndpointProjection *CreateEndpointProjection(
    struct CAMDAppEndpoint *app, const char *name, ULONG directions,
    struct CamdBase *CamdBase)
{
    struct EndpointProjection *projection;
    struct CAMDProviderReceiveSinkV1 sink;
    BOOL attached;

    projection = AllocMem(sizeof(*projection),
                          MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC);
    if (projection == NULL)
        return NULL;
    strcpy(projection->name, name);
    projection->app = app;
    projection->CamdBase = CamdBase;
    projection->device.Magic = MDD_Magic;
    projection->device.Name = projection->name;
    projection->device.IDString = projection->name;
    projection->device.OpenPort = projection_open;
    projection->device.ClosePort = projection_close;
    projection->device.NPorts = 1;
    projection->device.Flags = 1;
    projection->driver.mididevicedata = &projection->device;
    projection->driver.numports = 1;
    if (AllocDriverData(&projection->driver, &projection->device,
                        CamdBase) == FALSE) {
        FreeDriverData(&projection->driver, CamdBase);
        FreeMem(projection, sizeof(*projection));
        return NULL;
    }
    projection->driver.driverdatas[0]->Projection = app;

    ObtainSemaphore(CB(CamdBase)->CLSemaphore);
    attached = AttachDriverData(&projection->driver, CamdBase);
    ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
    if (attached == FALSE) {
        FreeDriverData(&projection->driver, CamdBase);
        FreeMem(projection, sizeof(*projection));
        return NULL;
    }

    if ((directions & CAMD_DIRECTION_INPUT) != 0) {
        memset(&sink, 0, sizeof(sink));
        sink.Size = sizeof(sink);
        sink.Version = 1;
        sink.Context = projection->driver.driverdatas[0];
        sink.SubmitMIDI1 = project_midi1;
        sink.SubmitMIDI1SysEx = project_sysex;
        camd_app_endpoint_set_projection(app, &sink);
    }
    return projection;
}

/* Takes the endpoint out of its clusters. Links of other programs stay there
 * and wait, as they do for an unplugged port. Must not hold CLSemaphore. */
void DeleteEndpointProjection(struct EndpointProjection *projection)
{
    struct CamdBase *CamdBase;

    if (projection == NULL)
        return;
    CamdBase = projection->CamdBase;
    /* No emit is in the sink once this returns. */
    camd_app_endpoint_set_projection(projection->app, NULL);
    /* A sender inside ProjectMidiToEndpoint() holds its cluster's semaphore,
     * which detaching waits for. */
    ObtainSemaphore(CB(CamdBase)->CLSemaphore);
    DetachDriverData(&projection->driver, CamdBase);
    ReleaseSemaphore(CB(CamdBase)->CLSemaphore);
    FreeDriverData(&projection->driver, CamdBase);
    FreeMem(projection, sizeof(*projection));
}
