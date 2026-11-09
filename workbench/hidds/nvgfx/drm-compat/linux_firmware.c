/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <aros/debug.h>

#include <linux/kernel.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/mm.h>

/*
 * Firmware lives under DEVS:Firmware/ with the linux-firmware layout
 * (nvidia/<chip>/<name>.bin). Images can run to tens of megabytes on the
 * GSP parts, so they are read straight into page memory.
 */
#define FIRMWARE_PATH "DEVS:Firmware/"

int request_firmware(const struct firmware **fw, const char *name, struct device *device)
{
    struct firmware *firmware;
    char path[256];
    BPTR file;
    LONG size, got;
    void *data;

    if (!fw || !name)
        return -EINVAL;
    *fw = NULL;

    snprintf(path, sizeof(path), FIRMWARE_PATH "%s", name);

    file = Open(path, MODE_OLDFILE);
    if (!file) {
        bug("[nouveau] firmware: cannot open %s\n", path);
        return -ENOENT;
    }

    Seek(file, 0, OFFSET_END);
    size = Seek(file, 0, OFFSET_BEGINNING);
    if (size <= 0) {
        Close(file);
        return -ENOENT;
    }

    data = vmalloc(size);
    if (!data) {
        Close(file);
        return -ENOMEM;
    }

    got = Read(file, data, size);
    Close(file);
    if (got != size) {
        vfree(data);
        return -EIO;
    }

    firmware = kzalloc(sizeof(*firmware), GFP_KERNEL);
    if (!firmware) {
        vfree(data);
        return -ENOMEM;
    }
    firmware->size = size;
    firmware->data = data;
    firmware->priv = data;
    *fw = firmware;

    D(bug("[nouveau] firmware: loaded %s (%ld bytes)\n", path, size));
    return 0;
}

int firmware_request_nowarn(const struct firmware **fw, const char *name, struct device *device)
{
    return request_firmware(fw, name, device);
}

int request_firmware_direct(const struct firmware **fw, const char *name, struct device *device)
{
    return request_firmware(fw, name, device);
}

int request_firmware_into_buf(const struct firmware **fw, const char *name, struct device *device, void *buf, size_t size)
{
    return -ENOSYS;
}

void release_firmware(const struct firmware *fw)
{
    if (fw) {
        vfree(fw->priv);
        kfree(fw);
    }
}
