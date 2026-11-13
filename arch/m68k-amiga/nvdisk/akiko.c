/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CD32 Akiko 24C08 nonvolatile storage backend.
*/

#include "nvdisk_intern.h"
#include "nvdisk_arch.h"

#include <exec/memory.h>
#include <exec/semaphores.h>
#include <proto/exec.h>

#include <string.h>

#define AKIKO_ID_ADDR       (*(volatile UWORD *)0x00b80002)
#define AKIKO_ID_MAGIC      0xcafe
#define AKIKO_I2C_LINES     (*(volatile UBYTE *)0x00b80030)
#define AKIKO_I2C_DIRECTION (*(volatile UBYTE *)0x00b80032)

#define I2C_SCL 0x80
#define I2C_SDA 0x40

#define EEPROM_SIZE         1024
#define EEPROM_PAGE_SIZE    16
#define EEPROM_RECORD_START 0x18

#define RECORD_APP          0x20
#define RECORD_ITEM         0x40
#define RECORD_LENGTH       0x60
#define RECORD_END          0xa0
#define RECORD_TYPE_MASK    0xe0
#define RECORD_VALUE_MASK   0x1f
#define RECORD_LOCKED       0x80

#define NV_NODESIZE (sizeof(struct NVEntry) + 32)

struct AkikoItem
{
    ULONG app;
    ULONG item;
    ULONG protection;
    ULONG data;
    ULONG length;
    ULONG end;
};

#define AKIKO_LOCATION ((BPTR)-1)

static struct SignalSemaphore *akiko_lock(struct NVDBase *base)
{
    return (struct SignalSemaphore *)base->nvd_DOSBase;
}

static void i2c_delay(void)
{
    volatile UBYTE value = 0;
    unsigned int i;

    /* Akiko register accesses use the slow peripheral bus.  Extra reads keep
     * the software clock below the 24C08's 100 kHz limit on a CD32. */
    for (i = 0; i < 8; i++)
        value = AKIKO_I2C_LINES;
    (void)value;
}

static void i2c_drive(UBYTE lines)
{
    AKIKO_I2C_LINES = 0;
    AKIKO_I2C_DIRECTION |= lines;
    i2c_delay();
}

static void i2c_release(UBYTE lines)
{
    AKIKO_I2C_DIRECTION &= ~lines;
    i2c_delay();
}

static void i2c_start(void)
{
    i2c_release(I2C_SDA | I2C_SCL);
    i2c_drive(I2C_SDA);
    i2c_drive(I2C_SCL);
}

static void i2c_stop(void)
{
    i2c_drive(I2C_SDA);
    i2c_release(I2C_SCL);
    i2c_release(I2C_SDA);
}

static BOOL i2c_write_byte(UBYTE value)
{
    unsigned int bit;
    BOOL ack;

    for (bit = 0; bit < 8; bit++)
    {
        if (value & 0x80)
            i2c_release(I2C_SDA);
        else
            i2c_drive(I2C_SDA);
        i2c_release(I2C_SCL);
        i2c_drive(I2C_SCL);
        value <<= 1;
    }

    i2c_release(I2C_SDA);
    i2c_release(I2C_SCL);
    ack = (AKIKO_I2C_LINES & I2C_SDA) == 0;
    i2c_drive(I2C_SCL);
    return ack;
}

static UBYTE i2c_read_byte(BOOL last)
{
    UBYTE value = 0;
    unsigned int bit;

    i2c_release(I2C_SDA);
    for (bit = 0; bit < 8; bit++)
    {
        i2c_release(I2C_SCL);
        value = (value << 1) | ((AKIKO_I2C_LINES & I2C_SDA) != 0);
        i2c_drive(I2C_SCL);
    }

    if (last)
        i2c_release(I2C_SDA);
    else
        i2c_drive(I2C_SDA);
    i2c_release(I2C_SCL);
    i2c_drive(I2C_SCL);
    i2c_release(I2C_SDA);
    return value;
}

static UBYTE eeprom_device(ULONG address, BOOL read)
{
    /* The 24C08 carries address bits A9:A8 in control-byte bits 2:1. */
    return 0xa0 | ((address >> 7) & 6) | (read ? 1 : 0);
}

static BOOL eeprom_read(UBYTE *data)
{
    ULONG address;

    for (address = 0; address < EEPROM_SIZE; address += 256)
    {
        ULONG i;

        i2c_start();
        if (!i2c_write_byte(eeprom_device(address, FALSE)) ||
            !i2c_write_byte((UBYTE)address))
        {
            i2c_stop();
            return FALSE;
        }
        i2c_start();
        if (!i2c_write_byte(eeprom_device(address, TRUE)))
        {
            i2c_stop();
            return FALSE;
        }
        for (i = 0; i < 256; i++)
            data[address + i] = i2c_read_byte(i == 255);
        i2c_stop();
    }
    return TRUE;
}

static BOOL eeprom_wait_ready(ULONG address)
{
    unsigned int retry;

    /* ACK polling accommodates modern EEPROMs with a 5 ms page-write cycle,
     * as well as the unusually fast original Atmel part. */
    for (retry = 0; retry < 4096; retry++)
    {
        BOOL ready;
        i2c_start();
        ready = i2c_write_byte(eeprom_device(address, FALSE));
        i2c_stop();
        if (ready)
            return TRUE;
    }
    return FALSE;
}

static BOOL eeprom_write_page(ULONG address, const UBYTE *data)
{
    ULONG i;

    i2c_start();
    if (!i2c_write_byte(eeprom_device(address, FALSE)) ||
        !i2c_write_byte((UBYTE)address))
    {
        i2c_stop();
        return FALSE;
    }
    for (i = 0; i < EEPROM_PAGE_SIZE; i++)
    {
        if (!i2c_write_byte(data[i]))
        {
            i2c_stop();
            return FALSE;
        }
    }
    i2c_stop();
    return eeprom_wait_ready(address);
}

static BOOL eeprom_write_changed(const UBYTE *old, const UBYTE *data)
{
    ULONG address;

    for (address = 0; address < EEPROM_SIZE; address += EEPROM_PAGE_SIZE)
    {
        if (memcmp(old + address, data + address, EEPROM_PAGE_SIZE) != 0 &&
            !eeprom_write_page(address, data + address))
            return FALSE;
    }
    return TRUE;
}

static BOOL image_valid(const UBYTE *image)
{
    return image[0] == 0x00 && image[1] == 0x56 &&
        image[2] == 0xa9 && image[3] == 0x00;
}

static void image_init(UBYTE *image)
{
    memset(image, 0, EEPROM_SIZE);
    image[0] = 0x00;
    image[1] = 0x56;
    image[2] = 0xa9;
    image[3] = 0x00;
    image[EEPROM_RECORD_START] = RECORD_END;
}

static int folded_char(UBYTE value)
{
    if (value >= 'A' && value <= 'Z')
        value += 'a' - 'A';
    return value;
}

static BOOL name_equal(const UBYTE *stored, ULONG length,
    CONST_STRPTR wanted)
{
    ULONG i;

    if (strlen(wanted) != length)
        return FALSE;
    for (i = 0; i < length; i++)
    {
        if (folded_char(stored[i]) != folded_char((UBYTE)wanted[i]))
            return FALSE;
    }
    return TRUE;
}

static BOOL parse_item(const UBYTE *image, ULONG app, ULONG item,
    struct AkikoItem *parsed)
{
    ULONG appLength = image[app] & RECORD_VALUE_MASK;
    ULONG itemLength = image[item] & RECORD_VALUE_MASK;
    ULONG lengthMarker;

    parsed->app = app;
    parsed->item = item;
    parsed->protection = item + 1 + itemLength;
    lengthMarker = parsed->protection + 1;
    if (parsed->protection >= EEPROM_SIZE ||
        lengthMarker + 1 >= EEPROM_SIZE ||
        (image[lengthMarker] & RECORD_TYPE_MASK) != RECORD_LENGTH ||
        app + 1 + appLength > item)
        return FALSE;
    parsed->length = ((ULONG)(image[lengthMarker] & RECORD_VALUE_MASK) << 8) |
        image[lengthMarker + 1];
    parsed->data = lengthMarker + 2;
    parsed->end = parsed->data + parsed->length;
    return parsed->end < EEPROM_SIZE;
}

static BOOL find_item(const UBYTE *image, CONST_STRPTR wantedApp,
    CONST_STRPTR wantedItem, struct AkikoItem *found)
{
    ULONG pos = EEPROM_RECORD_START;
    ULONG app = 0;
    BOOL appMatches = FALSE;

    while (pos < EEPROM_SIZE)
    {
        UBYTE marker = image[pos];
        ULONG type = marker & RECORD_TYPE_MASK;
        ULONG length = marker & RECORD_VALUE_MASK;

        if (marker == RECORD_END)
            return FALSE;
        if (type == RECORD_APP)
        {
            if (pos + 1 + length >= EEPROM_SIZE)
                return FALSE;
            app = pos;
            appMatches = name_equal(image + pos + 1, length, wantedApp);
            pos += 1 + length;
        }
        else if (type == RECORD_ITEM && app != 0)
        {
            struct AkikoItem item;
            if (!parse_item(image, app, pos, &item))
                return FALSE;
            if (appMatches && name_equal(image + pos + 1, length, wantedItem))
            {
                if (found)
                    *found = item;
                return TRUE;
            }
            pos = item.end;
        }
        else
            return FALSE;
    }
    return FALSE;
}

static BOOL image_end(const UBYTE *image, ULONG *end)
{
    ULONG pos = EEPROM_RECORD_START;
    ULONG app = 0;

    while (pos < EEPROM_SIZE)
    {
        UBYTE marker = image[pos];
        ULONG type = marker & RECORD_TYPE_MASK;
        ULONG length = marker & RECORD_VALUE_MASK;

        if (marker == RECORD_END)
        {
            *end = pos;
            return TRUE;
        }
        if (type == RECORD_APP)
        {
            if (pos + 1 + length >= EEPROM_SIZE)
                return FALSE;
            app = pos;
            pos += 1 + length;
        }
        else if (type == RECORD_ITEM && app != 0)
        {
            struct AkikoItem item;
            if (!parse_item(image, app, pos, &item))
                return FALSE;
            pos = item.end;
        }
        else
            return FALSE;
    }
    return FALSE;
}

static UBYTE *load_images(UBYTE **oldImage)
{
    UBYTE *memory = AllocVec(EEPROM_SIZE * 2, MEMF_ANY);

    if (!memory)
        return NULL;
    *oldImage = memory;
    if (!eeprom_read(memory))
    {
        FreeVec(memory);
        return NULL;
    }
    CopyMem(memory, memory + EEPROM_SIZE, EEPROM_SIZE);
    if (!image_valid(memory + EEPROM_SIZE))
        image_init(memory + EEPROM_SIZE);
    return memory + EEPROM_SIZE;
}

BOOL NVDisk_ArchInit(struct NVDBase *base)
{
    struct SignalSemaphore *semaphore;

    if (AKIKO_ID_ADDR != AKIKO_ID_MAGIC)
        return FALSE;
    semaphore = AllocMem(sizeof(*semaphore), MEMF_PUBLIC | MEMF_CLEAR);
    if (!semaphore)
        return FALSE;
    InitSemaphore(semaphore);
    base->nvd_location = AKIKO_LOCATION;
    base->nvd_DOSBase = (struct Library *)semaphore;
    return TRUE;
}

BOOL NVDisk_ArchActive(const struct NVDBase *base)
{
    return base->nvd_location == AKIKO_LOCATION;
}

void NVDisk_ArchExpunge(struct NVDBase *base)
{
    FreeMem(base->nvd_DOSBase, sizeof(struct SignalSemaphore));
}

APTR NVDisk_ArchRead(struct NVDBase *base, CONST_STRPTR appName,
    CONST_STRPTR itemName)
{
    UBYTE *image = AllocVec(EEPROM_SIZE, MEMF_ANY);
    APTR result = NULL;
    struct AkikoItem item;

    if (!image)
        return NULL;
    ObtainSemaphore(akiko_lock(base));
    if (eeprom_read(image) && image_valid(image) &&
        find_item(image, appName, itemName, &item))
    {
        result = AllocVec(item.length, MEMF_ANY);
        if (result)
            CopyMem(image + item.data, result, item.length);
    }
    ReleaseSemaphore(akiko_lock(base));
    FreeVec(image);
    return result;
}

LONG NVDisk_ArchWrite(struct NVDBase *base, CONST_STRPTR appName,
    CONST_STRPTR itemName, CONST_APTR data, ULONG length)
{
    UBYTE *oldImage;
    UBYTE *image;
    struct AkikoItem item;
    ULONG end, appLength = strlen(appName), itemLength = strlen(itemName);
    ULONG needed;
    BOOL reuseApp = FALSE;
    LONG result = NVERR_FAIL;

    if (appLength > 31 || itemLength > 31 || length > 0x1fff)
        return NVERR_BADNAME;

    ObtainSemaphore(akiko_lock(base));
    image = load_images(&oldImage);
    if (!image)
        goto out;

    if (find_item(image, appName, itemName, &item))
    {
        if (image[item.protection] & RECORD_LOCKED)
        {
            result = NVERR_WRITEPROT;
            goto free;
        }
        reuseApp = image[item.end] == RECORD_END;
        memmove(image + item.item, image + item.end,
            EEPROM_SIZE - item.end);
    }
    needed = (reuseApp ? 0 : 1 + appLength) +
        1 + itemLength + 1 + 2 + length + 1;
    if (!image_end(image, &end) || end + needed > EEPROM_SIZE)
        goto free;

    if (!reuseApp)
    {
        image[end++] = RECORD_APP | appLength;
        CopyMem(appName, image + end, appLength);
        end += appLength;
    }
    image[end++] = RECORD_ITEM | itemLength;
    CopyMem(itemName, image + end, itemLength);
    end += itemLength;
    image[end++] = 0;
    image[end++] = RECORD_LENGTH | ((length >> 8) & RECORD_VALUE_MASK);
    image[end++] = length;
    CopyMem(data, image + end, length);
    end += length;
    image[end] = RECORD_END;

    result = eeprom_write_changed(oldImage, image) ? 0 : NVERR_FATAL;
free:
    FreeVec(oldImage);
out:
    ReleaseSemaphore(akiko_lock(base));
    return result;
}

BOOL NVDisk_ArchDelete(struct NVDBase *base, CONST_STRPTR appName,
    CONST_STRPTR itemName)
{
    UBYTE *oldImage;
    UBYTE *image;
    struct AkikoItem item;
    BOOL result = FALSE;

    ObtainSemaphore(akiko_lock(base));
    image = load_images(&oldImage);
    if (image && find_item(image, appName, itemName, &item) &&
        !(image[item.protection] & RECORD_LOCKED))
    {
        memmove(image + item.item, image + item.end,
            EEPROM_SIZE - item.end);
        result = eeprom_write_changed(oldImage, image);
    }
    if (image)
        FreeVec(oldImage);
    ReleaseSemaphore(akiko_lock(base));
    return result;
}

BOOL NVDisk_ArchInfo(struct NVDBase *base, struct NVInfo *info)
{
    UBYTE *image = AllocVec(EEPROM_SIZE, MEMF_ANY);
    ULONG end;
    BOOL result = FALSE;

    if (!image)
        return FALSE;
    ObtainSemaphore(akiko_lock(base));
    if (eeprom_read(image))
    {
        if (!image_valid(image))
            image_init(image);
        if (image_end(image, &end))
        {
            info->nvi_MaxStorage = EEPROM_SIZE - EEPROM_RECORD_START;
            info->nvi_FreeStorage = EEPROM_SIZE - end - 1;
            result = TRUE;
        }
    }
    ReleaseSemaphore(akiko_lock(base));
    FreeVec(image);
    return result;
}

BOOL NVDisk_ArchProtect(struct NVDBase *base, CONST_STRPTR appName,
    CONST_STRPTR itemName, ULONG mask)
{
    UBYTE *oldImage;
    UBYTE *image;
    struct AkikoItem item;
    BOOL result = FALSE;

    ObtainSemaphore(akiko_lock(base));
    image = load_images(&oldImage);
    if (image && find_item(image, appName, itemName, &item))
    {
        if (mask & NVEF_DELETE)
            image[item.protection] |= RECORD_LOCKED;
        else
            image[item.protection] &= ~RECORD_LOCKED;
        result = eeprom_write_changed(oldImage, image);
    }
    if (image)
        FreeVec(oldImage);
    ReleaseSemaphore(akiko_lock(base));
    return result;
}

struct MinList *NVDisk_ArchList(struct NVDBase *base, CONST_STRPTR appName)
{
    UBYTE *image = AllocVec(EEPROM_SIZE, MEMF_ANY);
    struct MinList *result = NULL;
    ULONG pos = EEPROM_RECORD_START, app = 0, count = 0, offset;
    BOOL appMatches = FALSE;

    if (!image)
        return NULL;
    ObtainSemaphore(akiko_lock(base));
    if (!eeprom_read(image) || !image_valid(image))
        goto out;

    while (pos < EEPROM_SIZE && image[pos] != RECORD_END)
    {
        ULONG type = image[pos] & RECORD_TYPE_MASK;
        ULONG length = image[pos] & RECORD_VALUE_MASK;
        if (type == RECORD_APP)
        {
            app = pos;
            appMatches = name_equal(image + pos + 1, length, appName);
            pos += 1 + length;
        }
        else if (type == RECORD_ITEM && app)
        {
            struct AkikoItem item;
            if (!parse_item(image, app, pos, &item))
                goto out;
            if (appMatches)
                count++;
            pos = item.end;
        }
        else
            goto out;
    }

    result = AllocVec(sizeof(*result) + count * NV_NODESIZE, MEMF_CLEAR);
    if (!result)
        goto out;
    NEWLIST((struct List *)result);
    pos = EEPROM_RECORD_START;
    app = 0;
    appMatches = FALSE;
    offset = sizeof(*result);
    while (pos < EEPROM_SIZE && image[pos] != RECORD_END)
    {
        ULONG type = image[pos] & RECORD_TYPE_MASK;
        ULONG length = image[pos] & RECORD_VALUE_MASK;
        if (type == RECORD_APP)
        {
            app = pos;
            appMatches = name_equal(image + pos + 1, length, appName);
            pos += 1 + length;
        }
        else
        {
            struct AkikoItem item;
            if (!parse_item(image, app, pos, &item))
                break;
            if (appMatches)
            {
                struct NVEntry *entry =
                    (struct NVEntry *)((UBYTE *)result + offset);
                ULONG nameLength = image[pos] & RECORD_VALUE_MASK;
                entry->nve_Name = (STRPTR)entry + sizeof(*entry);
                CopyMem(image + pos + 1, entry->nve_Name, nameLength);
                entry->nve_Name[nameLength] = 0;
                entry->nve_Size = item.length;
                entry->nve_Protection =
                    (image[item.protection] & RECORD_LOCKED) ?
                    NVEF_DELETE : 0;
                AddTail((struct List *)result, (struct Node *)entry);
                offset += NV_NODESIZE;
            }
            pos = item.end;
        }
    }
out:
    ReleaseSemaphore(akiko_lock(base));
    FreeVec(image);
    return result;
}
