/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include LC_LIBDEFS_FILE
#include "btaudio.h"
#include <exec/memory.h>
#include <proto/ahi.h>
#include <stdio.h>

#undef UtilityBase
#define UtilityBase (cls->utility)


/* Keep saved AHI choices stable across reconnects and restarts. */
static ULONG bModeID(CONST_STRPTR address)
{
    UWORD crc = 0xffff;
    while(*address) {
        UBYTE c = *address++;
        int bit;
        if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
        crc ^= (UWORD)c << 8;
        for(bit = 0; bit < 8; bit++)
            crc = (crc & 0x8000) ? (UWORD)((crc << 1) ^ 0x1021) : (UWORD)(crc << 1);
    }
    if(crc < 0x100) crc += 0x100;
    return 0x00460000 | crc;
}

/* Database calls must never run under cls->lock: AHI holds its database
   lock while calling AllocAudio, which takes cls->lock in the other direction.
   Open AHI here, not at class load; it may not be available during scanning. */
static BOOL bMode(struct BtaLink *unit, CONST_STRPTR label, BOOL add)
{
    struct Library *AHIBase = NULL;
    struct Library *BluetoothBase = OpenLibrary("bluetooth.library", 1);
    struct MsgPort *port = &unit->ahiport;
    struct AHIRequest *request = &unit->ahireq;
    BOOL ok = FALSE;
    ULONG frequency;
    ObtainSemaphore(&unit->modelock);
    ObtainSemaphore(&unit->lock);
    frequency = unit->mixfreq;
    ReleaseSemaphore(&unit->lock);
    if(!unit->ahiopen) {
        memset(port, 0, sizeof(*port));
        memset(request, 0, sizeof(*request));
        port->mp_Node.ln_Type = NT_MSGPORT;
        port->mp_Flags = PA_IGNORE;
        NEWLIST(&port->mp_MsgList);
        request->ahir_Std.io_Message.mn_Node.ln_Type = NT_MESSAGE;
        request->ahir_Std.io_Message.mn_Length = sizeof(*request);
        request->ahir_Std.io_Message.mn_ReplyPort = port;
        request->ahir_Version = 4;
        unit->ahiopen = !OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *)request, 0);
    }
    if(unit->ahiopen) {
        AHIBase = (APTR)request->ahir_Std.io_Device;
        if(add) {
            struct BTAudioModeData data = {sizeof(data), unit->modeid, frequency};
            struct TagItem tags[] = {
                {AHIDB_AudioID, unit->modeid}, {AHIDB_Driver, (IPTR)"btaudio"},
                {AHIDB_Name, (IPTR)label}, {AHIDB_Volume, TRUE},
                {AHIDB_Panning, TRUE}, {AHIDB_Stereo, TRUE},
                {AHIDB_HiFi, FALSE}, {AHIDB_MultTable, FALSE},
                {AHIDB_Data, (IPTR)&data}, {TAG_DONE, 0}
            };
            ok = AHI_AddAudioMode(tags);
            if(ok) unit->modefreq = frequency;
        } else {
            AHI_RemoveAudioMode(unit->modeid);
            ok = TRUE;
        }
        /* Keep AHI resident for the lifetime of this runtime mode. */
        if(!add || !ok) {
            CloseDevice((struct IORequest *)request);
            unit->ahiopen = FALSE;
        }
    }
    ReleaseSemaphore(&unit->modelock);
    if(BluetoothBase) {
        if(!ok) btAddErrorMsg(RETURN_ERROR, "btaudio.class",
            "Cannot %s AHI mode for %s (ahi.device unavailable or mode rejected).",
            add ? "add" : "remove", unit->want_address);
        CloseLibrary(BluetoothBase);
    }
    return ok;
}

/* AHI_NextAudioID only walks the mode database; it opens no drivers. The
   worker calls this without its mixer lock, and release waits for the worker
   before closing the per-binding AHI request. */
void bRefreshMode(struct BtaLink *unit)
{
    struct Library *AHIBase;
    ULONG id = AHI_INVALID_ID;
    BOOL refresh;
    if(!unit->ahiopen) return;
    AHIBase = (APTR)unit->ahireq.ahir_Std.io_Device;
    while((id = AHI_NextAudioID(id)) != AHI_INVALID_ID)
        if(id == unit->modeid) break;
    ObtainSemaphore(&unit->lock);
    refresh = unit->attached && !unit->binding &&
        (id == AHI_INVALID_ID || unit->mixfreq != unit->modefreq);
    ReleaseSemaphore(&unit->lock);
    if(refresh) bMode(unit, unit->label, TRUE);
}

static struct BtaLink *bBindService(struct BTAudioClass *cls, APTR service)
{
    struct Library *BluetoothBase = OpenLibrary("bluetooth.library", 1);
    struct BtaLink *unit = NULL, *other;
    APTR device = NULL, sigep, mediaep;
    IPTR uuid = 0;
    STRPTR name = NULL, address = NULL;
    ULONG id;
    BOOL added = FALSE;

    if(!BluetoothBase) return NULL;
    btGetAttrs(BGA_SERVICE, service, BSVA_UUID16, (IPTR)&uuid,
        BSVA_Device, (IPTR)&device, TAG_DONE);
    if(uuid != 0x110b || !device) goto out;
    btGetAttrs(BGA_DEVICE, device, BDA_Name, (IPTR)&name,
        BDA_AddressString, (IPTR)&address, TAG_DONE);
    sigep = btFindEndpoint(service, NULL, BEA_PSM, BT_AVDTP_PSM, TAG_DONE);
    mediaep = sigep ? btFindEndpoint(service, sigep, BEA_PSM, BT_AVDTP_PSM, TAG_DONE) : NULL;
    if(!address || !sigep || !mediaep) goto out;
    id = bModeID(address);

    ObtainSemaphore(&cls->lock);
    for(other = (APTR)cls->units.lh_Head; other->node.ln_Succ; other = (APTR)other->node.ln_Succ) {
        if(other->modeid != id) continue;
        if(other->attached || other->binding || strcmp((char *)other->want_address, address)) {
            ReleaseSemaphore(&cls->lock);
            goto out;
        }
        /* Reuse the unit even when ahi.device still owns its allocation. */
        unit = other;
    }
    if(!unit) {
        unit = AllocVec(sizeof(*unit), MEMF_PUBLIC | MEMF_CLEAR);
        if(!unit) {
            ReleaseSemaphore(&cls->lock);
            goto out;
        }
        InitSemaphore(&unit->lock);
        InitSemaphore(&unit->modelock);
        AddTail(&cls->units, &unit->node);
    }
    unit->binding = TRUE;
    unit->modeid = id;
    ReleaseSemaphore(&cls->lock);

    ObtainSemaphore(&unit->lock);
    unit->ahisubbase = cls->audio;
    unit->device = device;
    unit->service = service;
    unit->sigep = sigep;
    unit->mediaep = mediaep;
    unit->stopper = NULL;
    snprintf((char *)unit->want_address, sizeof(unit->want_address), "%s", address);
    ReleaseSemaphore(&unit->lock);
    snprintf(unit->label, sizeof(unit->label), "%.60s: 16 bit stereo++", (name && *name) ? name : address);
    if(!bLinkStart(unit)) goto failed;
    ObtainSemaphore(&unit->lock);
    unit->attached = TRUE;
    ReleaseSemaphore(&unit->lock);
    added = bMode(unit, unit->label, TRUE);
    if(added) {
        btAddErrorMsg(RETURN_OK, "btaudio.class", "Audio sink '%s' bound at %lu Hz.",
            unit->label, unit->mixfreq);
        goto done;
    }
failed:
    bLinkStop(unit);
    ObtainSemaphore(&unit->lock);
    unit->device = unit->service = unit->sigep = unit->mediaep = NULL;
    ReleaseSemaphore(&unit->lock);
done:
    ObtainSemaphore(&cls->lock);
    ObtainSemaphore(&unit->lock);
    unit->binding = FALSE;
    if(unit->task) Signal(unit->task, 1UL << unit->wakesig);
    ReleaseSemaphore(&unit->lock);
    ReleaseSemaphore(&cls->lock);
out:
    CloseLibrary(BluetoothBase);
    return added ? unit : NULL;
}

static void bReleaseService(struct BTAudioClass *cls, struct BtaLink *unit)
{
    bLinkStop(unit);
    bMode(unit, NULL, FALSE);
    ObtainSemaphore(&unit->lock);
    unit->device = unit->service = unit->sigep = unit->mediaep = NULL;
    ReleaseSemaphore(&unit->lock);
    {
        struct Library *BluetoothBase = OpenLibrary("bluetooth.library", 1);
        if(BluetoothBase) {
            btAddErrorMsg(RETURN_OK, "btaudio.class", "Audio sink %s released.", unit->want_address);
            CloseLibrary(BluetoothBase);
        }
    }
}

static void bCleanup(struct BTAudioClass *cls)
{
    struct BtaLink *unit;
    if(cls->audio) bAudioDelete(cls->audio);
    while((unit = (APTR)RemHead(&cls->units))) FreeVec(unit);
    if(cls->dos) CloseLibrary((struct Library *)cls->dos);
    if(cls->utility) CloseLibrary(cls->utility);
}

static int GM_UNIQUENAME(libInit)(struct BTAudioClass *cls)
{
    NEWLIST(&cls->units);
    InitSemaphore(&cls->lock);
    cls->utility = OpenLibrary("utility.library", 39);
    cls->dos = (APTR)OpenLibrary("dos.library", 39);
    if(cls->utility && cls->dos) cls->audio = bAudioCreate(cls);
    if(cls->audio) return TRUE;
    bCleanup(cls);
    return FALSE;
}

static int GM_UNIQUENAME(libExpunge)(struct BTAudioClass *cls)
{
    struct BtaLink *unit;
    if(cls->audio->library.lib_OpenCnt != 1) return FALSE;
    for(unit = (APTR)cls->units.lh_Head; unit->node.ln_Succ; unit = (APTR)unit->node.ln_Succ)
        if(unit->attached || unit->task || unit->owner) return FALSE;
    bCleanup(cls);
    return TRUE;
}
ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0)

AROS_LH3(LONG, btcGetAttrsA,
    AROS_LHA(ULONG, type, D0), AROS_LHA(APTR, object, A0),
    AROS_LHA(struct TagItem *, tags, A1), struct BTAudioClass *, cls, 5, btaudio)
{
    AROS_LIBFUNC_INIT
    struct TagItem *tag, *state = tags;
    struct BtaLink *unit = object;
    LONG count = 0;
    while((tag = NextTagItem(&state))) {
        IPTR value;
        if(type == BCGA_CLASS) {
            switch(tag->ti_Tag) {
            case BCCA_Priority: value = 0; break;
            case BCCA_Description: value = (IPTR)"Audio sinks (A2DP) via AHI"; break;
            case BCCA_HasClassCfgGUI:
            case BCCA_HasBindingCfgGUI:
            case BCCA_AfterDOSRestart: value = FALSE; break;
            case BCCA_UsingDefaultCfg: value = TRUE; break;
            default: continue;
            }
        } else if(type == BCGA_BINDING && unit) {
            switch(tag->ti_Tag) {
            case BCBA_UsingDefaultCfg: value = TRUE; break;
            case BCBA_Device: value = (IPTR)unit->device; break;
            case BCBA_Service: value = (IPTR)unit->service; break;
            case BCBA_Task: value = (IPTR)unit->task; break;
            default: continue;
            }
        } else continue;
        *(IPTR *)tag->ti_Data = value;
        count++;
    }
    return count;
    AROS_LIBFUNC_EXIT
}
AROS_LH3(LONG, btcSetAttrsA,
    AROS_LHA(ULONG, type, D0), AROS_LHA(APTR, object, A0),
    AROS_LHA(struct TagItem *, tags, A1), struct BTAudioClass *, cls, 6, btaudio)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}
AROS_LH2(SIPTR, btcDoMethodA,
    AROS_LHA(ULONG, method, D0), AROS_LHA(IPTR *, data, A1),
    struct BTAudioClass *, cls, 7, btaudio)
{
    AROS_LIBFUNC_INIT
    switch(method) {
    case BCM_AttemptServiceBinding:
    case BCM_ForceServiceBinding: return (SIPTR)bBindService(cls, (APTR)data[0]);
    case BCM_ReleaseServiceBinding: bReleaseService(cls, (APTR)data[0]); return TRUE;
    default: return 0;
    }
    AROS_LIBFUNC_EXIT
}
