/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Query and control the Baseboard Management Controller through
          the IPMI hidd (C:IPMI).
*/

/******************************************************************************

    NAME

        IPMI

    SYNOPSIS

        INFO/S,SENSORS/S,SEL/S,CLEAR/S,RAW/M,INTERFACE/K/N

    LOCATION

        C:

    FUNCTION

        Talks to the system's Baseboard Management Controller (BMC) over
        the IPMI system interfaces registered by the ipmi hidd.

        With no arguments the interfaces found by the firmware tables are
        listed.

    INPUTS

        INFO       -- Send Get Device ID to the BMC and show the result.
        SENSORS    -- Walk the Sensor Data Record repository and show the
                      current reading of every sensor.
        SEL        -- List the System Event Log.
        CLEAR      -- With SEL, erase the System Event Log.
        RAW        -- Send a raw command. The values are hexadecimal:
                      network function, command, then data bytes. The
                      response is printed as hexadecimal bytes, starting
                      with the completion code.
        INTERFACE  -- Number of the interface to use (default 0).

    RESULT

        Standard DOS return codes. RAW returns WARN if the BMC answered
        with a non-zero completion code.

    EXAMPLES

        IPMI
        IPMI INFO
        IPMI SENSORS
        IPMI SEL
        IPMI SEL CLEAR
        IPMI RAW 06 01
        IPMI RAW 0a 20

******************************************************************************/

#include <aros/asmcall.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <utility/hooks.h>
#include <oop/oop.h>
#include <utility/date.h>

#include <hidd/hidd.h>
#include <hidd/system.h>
#include <hidd/ipmi.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <string.h>
#include <stdlib.h>

const char version_tag[] = "$VER: IPMI 1.0 (15.9.2026)";

#define TEMPLATE "INFO/S,SENSORS/S,SEL/S,CLEAR/S,RAW/M,INTERFACE/K/N"

enum
{
    ARG_INFO,
    ARG_SENSORS,
    ARG_SEL,
    ARG_CLEAR,
    ARG_RAW,
    ARG_INTERFACE,
    NUM_ARGS
};

#define MAX_INTERFACES 8

OOP_AttrBase HiddIPMIAB;

static const struct OOP_ABDescr ipmi_abd[] =
{
    { IID_Hidd_IPMI, &HiddIPMIAB },
    { NULL, NULL }
};

struct EnumData
{
    OOP_Object *interfaces[MAX_INTERFACES];
    ULONG count;
};

static BOOL IsIPMIObject(OOP_Object *obj)
{
    OOP_Class *cl;

    for (cl = OOP_OCLASS(obj); cl; cl = cl->superclass)
    {
        if (cl->ClassNode.ln_Name && !strcmp(cl->ClassNode.ln_Name, CLID_Hidd_IPMI))
            return TRUE;
    }
    return FALSE;
}

AROS_UFH3S(BOOL, DriverEnum,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, obj, A2),
    AROS_UFHA(struct EnumData *, data, A1))
{
    AROS_USERFUNC_INIT

    (void)hook;

    if (IsIPMIObject(obj) && data->count < MAX_INTERFACES)
        data->interfaces[data->count++] = obj;

    return FALSE;

    AROS_USERFUNC_EXIT
}

static ULONG FindInterfaces(struct EnumData *data)
{
    struct Hook enum_hook =
    {
        .h_Entry = (HOOKFUNC)DriverEnum,
        .h_Data = NULL
    };
    OOP_Object *root;

    data->count = 0;

    root = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
    if (!root)
        root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (root)
        HW_EnumDrivers(root, &enum_hook, data);

    return data->count;
}

static const char *InterfaceName(IPTR type)
{
    switch (type)
    {
    case vHidd_IPMI_Interface_KCS:  return "KCS";
    case vHidd_IPMI_Interface_SMIC: return "SMIC";
    case vHidd_IPMI_Interface_BT:   return "BT";
    case vHidd_IPMI_Interface_SSIF: return "SSIF";
    default:                        return "unknown";
    }
}

static void PrintInterface(ULONG n, OOP_Object *obj)
{
    IPTR type = 0, major = 0, minor = 0, base = 0, space = 0, spacing = 0, irq = 0, i2c = 0;
    const char *spaceName;
    ULONG stride;

    OOP_GetAttr(obj, aHidd_IPMI_InterfaceType, &type);
    OOP_GetAttr(obj, aHidd_IPMI_SpecVersionMajor, &major);
    OOP_GetAttr(obj, aHidd_IPMI_SpecVersionMinor, &minor);
    OOP_GetAttr(obj, aHidd_IPMI_BaseAddress, &base);
    OOP_GetAttr(obj, aHidd_IPMI_AddressSpace, &space);
    OOP_GetAttr(obj, aHidd_IPMI_RegisterSpacing, &spacing);
    OOP_GetAttr(obj, aHidd_IPMI_InterruptNumber, &irq);
    OOP_GetAttr(obj, aHidd_IPMI_I2CSlaveAddress, &i2c);

    stride = spacing == vHidd_IPMI_RegSpacing_16 ? 16 : spacing == vHidd_IPMI_RegSpacing_4 ? 4 : 1;

    if (type == vHidd_IPMI_Interface_SSIF)
    {
        Printf("%lu: %s, IPMI %lu.%lu, SMBus address 0x%02lx, BMC I2C address 0x%02lx\n",
            n, InterfaceName(type), major, minor, (ULONG)base, (ULONG)i2c);
        return;
    }

    spaceName = space == vHidd_IPMI_AddressSpace_IO ? "I/O" : "memory";
    Printf("%lu: %s, IPMI %lu.%lu, %s 0x", n, InterfaceName(type), (ULONG)major, (ULONG)minor, spaceName);
#if __WORDSIZE == 64
    if (base >> 32)
        Printf("%lx%08lx", (ULONG)(base >> 32), (ULONG)base);
    else
#endif
        Printf("%lx", (ULONG)base);
    Printf(", register spacing %lu", stride);
    if (irq)
        Printf(", IRQ %lu", (ULONG)irq);
    Printf("\n");
}

static int DoInfo(OOP_Object *obj)
{
    UBYTE resp[32];
    ULONG len = sizeof(resp);

    if (!HIDD_IPMI_SendCommand(obj, IPMI_NETFN_APP, IPMI_CMD_GET_DEVICE_ID, NULL, 0, resp, &len))
    {
        PutStr("IPMI: no response from the BMC\n");
        return RETURN_ERROR;
    }
    if (resp[0] != IPMI_CC_OK || len < 12)
    {
        Printf("IPMI: Get Device ID failed, completion code 0x%02lx\n", (ULONG)resp[0]);
        return RETURN_ERROR;
    }

    Printf("Device ID          : 0x%02lx\n", (ULONG)resp[1]);
    Printf("Device revision    : %lu%s\n", (ULONG)(resp[2] & 0x0F),
        (resp[2] & 0x80) ? " (provides SDRs)" : "");
    Printf("Firmware revision  : %lu.%02lx%s\n", (ULONG)(resp[3] & 0x7F), (ULONG)resp[4],
        (resp[3] & 0x80) ? " (update in progress)" : "");
    Printf("IPMI version       : %lu.%lu\n", (ULONG)(resp[5] & 0x0F), (ULONG)(resp[5] >> 4));
    Printf("Device support     :");
    if (resp[6] & 0x80) PutStr(" Chassis");
    if (resp[6] & 0x40) PutStr(" Bridge");
    if (resp[6] & 0x20) PutStr(" EventGen");
    if (resp[6] & 0x10) PutStr(" EventRcv");
    if (resp[6] & 0x08) PutStr(" FRU");
    if (resp[6] & 0x04) PutStr(" SEL");
    if (resp[6] & 0x02) PutStr(" SDR");
    if (resp[6] & 0x01) PutStr(" Sensors");
    PutStr("\n");
    Printf("Manufacturer ID    : 0x%06lx\n", (ULONG)(resp[7] | (resp[8] << 8) | (resp[9] << 16)));
    Printf("Product ID         : 0x%04lx\n", (ULONG)(resp[10] | (resp[11] << 8)));
    if (len >= 16)
        Printf("Auxiliary firmware : %02lx %02lx %02lx %02lx\n",
            (ULONG)resp[12], (ULONG)resp[13], (ULONG)resp[14], (ULONG)resp[15]);

    return RETURN_OK;
}


/* Shared helper: send a command, print a message and return FALSE if it failed */
static BOOL Command(OOP_Object *obj, UBYTE netfn, UBYTE cmd, const UBYTE *req, ULONG reqLen,
    UBYTE *resp, ULONG *len, const char *what)
{
    if (!HIDD_IPMI_SendCommand(obj, netfn, cmd, (APTR)req, reqLen, resp, len))
    {
        Printf("IPMI: no response to %s\n", what);
        return FALSE;
    }
    if (resp[0] != IPMI_CC_OK)
    {
        Printf("IPMI: %s failed, completion code 0x%02lx\n", what, (ULONG)resp[0]);
        return FALSE;
    }
    return TRUE;
}

/* Copy an SDR/FRU "type/length" string: 0xC0 | n means n bytes of 8-bit ASCII */
static void CopyIDString(const UBYTE *tl, ULONG avail, char *out, ULONG outSize)
{
    ULONG n = tl[0] & 0x1F, i;

    if ((tl[0] & 0xC0) != 0xC0 || n + 1 > avail)
        n = 0;
    if (n >= outSize)
        n = outSize - 1;
    for (i = 0; i < n; i++)
        out[i] = (tl[1 + i] >= 32 && tl[1 + i] < 127) ? tl[1 + i] : '?';
    out[n] = 0;
}

static const char *SensorUnit(UBYTE code)
{
    static const char *const units[] =
    {
        "", "degrees C", "degrees F", "K", "V", "A", "W", "J", "C", "VA", "nits",
        "lumen", "lux", "cd", "kPa", "PSI", "N", "CFM", "RPM", "Hz", "us", "ms",
        "s", "min", "h", "d", "wk", "mil", "in", "ft", "in^3", "ft^3", "mm", "cm",
        "m", "cm^3", "m^3", "l", "fl oz", "rad", "sr", "rev", "cycles", "g", "ounce",
        "lb", "ft-lb", "oz-in", "gauss", "gilberts", "H", "mH", "F", "uF", "ohms",
        "S", "mole", "Bq", "ppm", "", "dB", "dBA", "dBC", "Gy", "Sv", "K", "bits",
        "kb", "Mb", "Gb", "bytes", "kB", "MB", "GB", "words", "dwords", "qwords",
        "lines", "hits", "misses", "retries", "resets", "overruns", "underruns",
        "collisions", "packets", "messages", "chars", "errors", "corr. errors",
        "uncorr. errors", "fatal errors", "grams"
    };

    return code < sizeof(units) / sizeof(units[0]) ? units[code] : "";
}

/* Sign-extend a 4-bit exponent */
static LONG Exp4(UBYTE v)
{
    return (v & 0x8) ? (LONG)v - 16 : v;
}

/* Sign-extend a 10-bit factor */
static LONG Factor10(UBYTE ls, UBYTE ms)
{
    LONG v = ls | ((ms & 0xC0) << 2);

    return (v & 0x200) ? v - 1024 : v;
}

/* Raw reading -> value * 1000 using the record's linear conversion (IPMI 2.0 36.3) */
static BOOL ConvertReading(const UBYTE *rec, UBYTE raw, LONG *milli)
{
    LONG x, m, b, bexp, rexp, i;
    LONG scaled;    /* (M * x + B * 10^bexp) * 1000 */

    switch ((rec[20] >> 6) & 0x03)
    {
    case 0:     x = raw; break;                                     /* unsigned */
    case 1:     x = (raw & 0x80) ? -(LONG)(~raw & 0xFF) : raw; break; /* 1's complement */
    case 2:     x = (BYTE)raw; break;                               /* 2's complement */
    default:    return FALSE;                                       /* no analog reading */
    }
    if ((rec[23] & 0x7F) != 0)      /* only linear sensors are converted */
        return FALSE;

    m = Factor10(rec[24], rec[25]);
    b = Factor10(rec[26], rec[27]);
    rexp = Exp4(rec[29] >> 4);
    bexp = Exp4(rec[29] & 0x0F);

    scaled = b * 1000;
    for (i = 0; i < bexp; i++) scaled *= 10;
    for (i = 0; i > bexp; i--) scaled /= 10;
    scaled += m * x * 1000;
    for (i = 0; i < rexp; i++) scaled *= 10;
    for (i = 0; i > rexp; i--) scaled /= 10;

    *milli = scaled;
    return TRUE;
}

static void PrintMilli(LONG milli)
{
    LONG whole = milli / 1000, frac = milli % 1000;

    if (frac < 0)
        frac = -frac;
    if (milli < 0 && whole == 0)
        PutStr("-");
    Printf("%ld.%03ld", whole, frac);
}

static int DoSensors(OOP_Object *obj)
{
    UBYTE req[6], resp[80];
    ULONG len;
    UWORD reservation, recordId = 0;
    ULONG shown = 0;

    len = sizeof(resp);
    if (!Command(obj, IPMI_NETFN_STORAGE, IPMI_CMD_RESERVE_SDR_REPOSITORY, NULL, 0, resp, &len,
        "Reserve SDR Repository"))
        return RETURN_ERROR;
    reservation = resp[1] | (resp[2] << 8);

    while (recordId != 0xFFFF)
    {
        UBYTE rec[64];
        ULONG recLen = 0, offset = 0;
        UWORD nextId;
        char name[32];

        /* Fetch the record in chunks; small BMCs limit the bytes per Get SDR */
        do
        {
            ULONG chunk = 16, n;

            if (recLen && recLen - offset < chunk)
                chunk = recLen - offset;

            req[0] = reservation & 0xFF;
            req[1] = reservation >> 8;
            req[2] = recordId & 0xFF;
            req[3] = recordId >> 8;
            req[4] = offset;
            req[5] = chunk;
            len = sizeof(resp);
            if (!HIDD_IPMI_SendCommand(obj, IPMI_NETFN_STORAGE, IPMI_CMD_GET_SDR, req, 6, resp, &len))
            {
                PutStr("IPMI: no response to Get SDR\n");
                return RETURN_ERROR;
            }
            if (resp[0] == IPMI_CC_INVALID_RESERVATION)
            {
                len = sizeof(resp);
                if (!Command(obj, IPMI_NETFN_STORAGE, IPMI_CMD_RESERVE_SDR_REPOSITORY, NULL, 0,
                    resp, &len, "Reserve SDR Repository"))
                    return RETURN_ERROR;
                reservation = resp[1] | (resp[2] << 8);
                continue;
            }
            if (resp[0] != IPMI_CC_OK || len < 3)
            {
                Printf("IPMI: Get SDR (record 0x%04lx) failed, completion code 0x%02lx\n",
                    (ULONG)recordId, (ULONG)resp[0]);
                return RETURN_ERROR;
            }
            nextId = resp[1] | (resp[2] << 8);
            n = len - 3;
            if (n > sizeof(resp) - 3)
                n = sizeof(resp) - 3;
            if (offset + n > sizeof(rec))
                n = sizeof(rec) - offset;
            CopyMem(&resp[3], &rec[offset], n);
            offset += n;
            if (n == 0)
                break;
            /* Header byte 4 is the length of the body */
            recLen = offset >= 5 ? rec[4] + 5 : 5;
        } while (offset < recLen && offset < sizeof(rec));

        if (offset >= 8 && (rec[3] == 0x01 || rec[3] == 0x02))
        {
            UBYTE sensorNum = rec[7], sensorType = rec[12];
            UBYTE tlOffset = rec[3] == 0x01 ? 47 : 31;
            UBYTE unitCode = rec[21];       /* sensor units 2: base unit */

            if (offset > tlOffset)
                CopyIDString(&rec[tlOffset], offset - tlOffset, name, sizeof(name));
            else
                name[0] = 0;

            Printf("%-16s (0x%02lx, type 0x%02lx): ", name, (ULONG)sensorNum, (ULONG)sensorType);

            req[0] = sensorNum;
            len = sizeof(resp);
            if (!HIDD_IPMI_SendCommand(obj, IPMI_NETFN_SENSOR_EVENT, IPMI_CMD_GET_SENSOR_READING,
                req, 1, resp, &len))
                PutStr("no response\n");
            else if (resp[0] != IPMI_CC_OK || len < 3)
                Printf("completion code 0x%02lx\n", (ULONG)resp[0]);
            else if (resp[2] & 0x20)
                PutStr("reading unavailable\n");
            else if (!(resp[2] & 0x40))
                PutStr("scanning disabled\n");
            else
            {
                LONG milli;

                if (rec[3] == 0x01 && offset >= 30 && ConvertReading(rec, resp[1], &milli))
                {
                    PrintMilli(milli);
                    Printf(" %s", SensorUnit(unitCode));
                }
                else
                    Printf("raw 0x%02lx", (ULONG)resp[1]);
                if (len >= 4 && rec[13] == 0x01)   /* threshold based */
                {
                    if (resp[3] & 0x3F)
                        Printf(" [%s%s%s%s%s%s]",
                            (resp[3] & 0x01) ? " lower non-critical" : "",
                            (resp[3] & 0x02) ? " lower critical" : "",
                            (resp[3] & 0x04) ? " lower non-recoverable" : "",
                            (resp[3] & 0x08) ? " upper non-critical" : "",
                            (resp[3] & 0x10) ? " upper critical" : "",
                            (resp[3] & 0x20) ? " upper non-recoverable" : "");
                    else
                        PutStr(" ok");
                }
                else if (len >= 4)
                    Printf(" state 0x%04lx", (ULONG)(resp[3] | (len >= 5 ? resp[4] << 8 : 0)));
                PutStr("\n");
            }
            shown++;
        }

        if (nextId == recordId)
            break;
        recordId = nextId;
    }

    if (!shown)
        PutStr("IPMI: no sensor records\n");

    return RETURN_OK;
}

static void PrintTimestamp(ULONG ts)
{
    struct ClockData cd;

    if (ts < 0x20000000)
    {
        Printf("boot+%08lus", ts);
        return;
    }
    /* IPMI counts from 1970, Amiga2Date from 1978 (2922 days) */
    Amiga2Date(ts - 2922UL * 24 * 60 * 60, &cd);
    Printf("%04lu-%02lu-%02lu %02lu:%02lu:%02lu", (ULONG)cd.year, (ULONG)cd.month, (ULONG)cd.mday,
        (ULONG)cd.hour, (ULONG)cd.min, (ULONG)cd.sec);
}

static int DoSEL(OOP_Object *obj, BOOL clear)
{
    UBYTE req[6], resp[40];
    ULONG len, entries, i;
    UWORD recordId = 0;

    len = sizeof(resp);
    if (!Command(obj, IPMI_NETFN_STORAGE, IPMI_CMD_GET_SEL_INFO, NULL, 0, resp, &len, "Get SEL Info"))
        return RETURN_ERROR;
    if (len < 15)
    {
        PutStr("IPMI: short Get SEL Info response\n");
        return RETURN_ERROR;
    }
    entries = resp[2] | (resp[3] << 8);
    Printf("SEL version %lu.%lu, %lu entries, %lu bytes free\n", (ULONG)(resp[1] & 0x0F),
        (ULONG)(resp[1] >> 4), entries, (ULONG)(resp[4] | (resp[5] << 8)));

    if (clear)
    {
        UWORD reservation;
        int tries;

        len = sizeof(resp);
        if (!Command(obj, IPMI_NETFN_STORAGE, IPMI_CMD_RESERVE_SEL, NULL, 0, resp, &len, "Reserve SEL"))
            return RETURN_ERROR;
        reservation = resp[1] | (resp[2] << 8);

        req[0] = reservation & 0xFF;
        req[1] = reservation >> 8;
        req[2] = 'C';
        req[3] = 'L';
        req[4] = 'R';
        req[5] = 0xAA;      /* initiate erase */
        len = sizeof(resp);
        if (!Command(obj, IPMI_NETFN_STORAGE, IPMI_CMD_CLEAR_SEL, req, 6, resp, &len, "Clear SEL"))
            return RETURN_ERROR;

        for (tries = 0; tries < 50 && (resp[1] & 0x0F) != 1; tries++)
        {
            Delay(5);
            req[5] = 0x00;  /* get erasure status */
            len = sizeof(resp);
            if (!Command(obj, IPMI_NETFN_STORAGE, IPMI_CMD_CLEAR_SEL, req, 6, resp, &len, "Clear SEL"))
                return RETURN_ERROR;
        }
        PutStr((resp[1] & 0x0F) == 1 ? "SEL cleared\n" : "SEL erase still in progress\n");
        return RETURN_OK;
    }

    for (i = 0; i < entries && recordId != 0xFFFF; i++)
    {
        const UBYTE *e;
        UWORD nextId;

        req[0] = 0;         /* no reservation: whole record */
        req[1] = 0;
        req[2] = recordId & 0xFF;
        req[3] = recordId >> 8;
        req[4] = 0;
        req[5] = 0xFF;
        len = sizeof(resp);
        if (!Command(obj, IPMI_NETFN_STORAGE, IPMI_CMD_GET_SEL_ENTRY, req, 6, resp, &len, "Get SEL Entry"))
            return RETURN_ERROR;
        if (len < 19)
        {
            PutStr("IPMI: short SEL entry\n");
            return RETURN_ERROR;
        }
        nextId = resp[1] | (resp[2] << 8);
        e = &resp[3];

        Printf("%04lx: ", (ULONG)(e[0] | (e[1] << 8)));
        if (e[2] == 0x02)
        {
            PrintTimestamp(e[3] | (e[4] << 8) | (e[5] << 16) | ((ULONG)e[6] << 24));
            Printf(" gen 0x%04lx sensor type 0x%02lx #0x%02lx %s type 0x%02lx data %02lx %02lx %02lx\n",
                (ULONG)(e[7] | (e[8] << 8)), (ULONG)e[10], (ULONG)e[11],
                (e[12] & 0x80) ? "deasserted" : "asserted", (ULONG)(e[12] & 0x7F),
                (ULONG)e[13], (ULONG)e[14], (ULONG)e[15]);
        }
        else if (e[2] >= 0xC0 && e[2] <= 0xDF)
        {
            PrintTimestamp(e[3] | (e[4] << 8) | (e[5] << 16) | ((ULONG)e[6] << 24));
            Printf(" OEM record type 0x%02lx manufacturer 0x%06lx data %02lx %02lx %02lx %02lx %02lx %02lx\n",
                (ULONG)e[2], (ULONG)(e[7] | (e[8] << 8) | (e[9] << 16)),
                (ULONG)e[10], (ULONG)e[11], (ULONG)e[12], (ULONG)e[13], (ULONG)e[14], (ULONG)e[15]);
        }
        else
        {
            ULONG k;

            Printf("OEM record type 0x%02lx data", (ULONG)e[2]);
            for (k = 3; k < 16; k++)
                Printf(" %02lx", (ULONG)e[k]);
            PutStr("\n");
        }

        if (nextId == recordId)
            break;
        recordId = nextId;
    }

    return RETURN_OK;
}

static BOOL ParseHexByte(const char *s, UBYTE *out)
{
    char *end;
    long v;

    if (!*s)
        return FALSE;
    v = strtol(s, &end, 16);
    if (*end || v < 0 || v > 0xFF)
        return FALSE;
    *out = v;
    return TRUE;
}

static int DoRaw(OOP_Object *obj, CONST_STRPTR *words)
{
    UBYTE req[IPMI_MAX_REQUEST_DATA];
    UBYTE resp[256];
    UBYTE netfn, cmd;
    ULONG reqLen = 0, len = sizeof(resp), i;

    if (!words[0] || !words[1] || !ParseHexByte(words[0], &netfn) || !ParseHexByte(words[1], &cmd)
        || netfn > 0x3F)
    {
        PutStr("IPMI: RAW needs a network function and a command in hexadecimal\n");
        return RETURN_FAIL;
    }
    for (i = 2; words[i]; i++)
    {
        if (reqLen >= sizeof(req) || !ParseHexByte(words[i], &req[reqLen]))
        {
            Printf("IPMI: bad data byte '%s'\n", words[i]);
            return RETURN_FAIL;
        }
        reqLen++;
    }

    if (!HIDD_IPMI_SendCommand(obj, netfn, cmd, req, reqLen, resp, &len))
    {
        PutStr("IPMI: no response from the BMC\n");
        return RETURN_ERROR;
    }

    if (len > sizeof(resp))
        len = sizeof(resp);
    for (i = 0; i < len; i++)
        Printf("%s%02lx", i ? " " : "", (ULONG)resp[i]);
    PutStr("\n");

    return resp[0] == IPMI_CC_OK ? RETURN_OK : RETURN_WARN;
}

int main(void)
{
    IPTR args[NUM_ARGS] = { 0 };
    struct RDArgs *rdargs;
    struct EnumData data;
    ULONG which = 0, i;
    int rc = RETURN_OK;

    rdargs = ReadArgs(TEMPLATE, args, NULL);
    if (!rdargs)
    {
        PrintFault(IoErr(), "IPMI");
        return RETURN_FAIL;
    }

    if (!OOP_ObtainAttrBases(ipmi_abd))
    {
        PutStr("IPMI: the ipmi hidd is not available\n");
        FreeArgs(rdargs);
        return RETURN_FAIL;
    }

    if (FindInterfaces(&data) == 0)
    {
        PutStr("IPMI: no BMC interface is described by the firmware\n");
        rc = RETURN_WARN;
    }
    else if (!args[ARG_INFO] && !args[ARG_SENSORS] && !args[ARG_SEL] && !args[ARG_RAW])
    {
        for (i = 0; i < data.count; i++)
            PrintInterface(i, data.interfaces[i]);
    }
    else
    {
        if (args[ARG_INTERFACE])
            which = *(LONG *)args[ARG_INTERFACE];
        if (which >= data.count)
        {
            Printf("IPMI: no interface %lu\n", which);
            rc = RETURN_FAIL;
        }
        else if (args[ARG_INFO])
            rc = DoInfo(data.interfaces[which]);
        else if (args[ARG_SENSORS])
            rc = DoSensors(data.interfaces[which]);
        else if (args[ARG_SEL])
            rc = DoSEL(data.interfaces[which], args[ARG_CLEAR] != 0);
        else
            rc = DoRaw(data.interfaces[which], (CONST_STRPTR *)args[ARG_RAW]);
    }

    OOP_ReleaseAttrBases(ipmi_abd);
    FreeArgs(rdargs);

    return rc;
}
