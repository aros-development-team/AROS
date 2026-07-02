/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <aros/debug.h>

#include <drm-compat/drm_compat_funcs.h>
#include <drm-compat/drm_compat_mem.h>

int request_firmware(const struct firmware **fw, const char *name, struct device *device)
{
    struct firmware *firmware;
    BPTR file;
    LONG size;
    APTR data;
    TEXT fullpath[256];

    if (!fw || !name)
        return -EINVAL;

    *fw = NULL;

    sprintf(fullpath, "DEVS:Firmware/%s", name);

    file = Open(fullpath, MODE_OLDFILE);
    if (!file)
    {
        bug("FIRMWARE: failed to open firmware file: %s\n", fullpath);
        return -ENOENT;
    }

    Seek(file, 0, OFFSET_END);
    size = Seek(file, 0, OFFSET_BEGINNING);
    if (size <= 0)
    {
        Close(file);
        return -ENOENT;
    }

    data = kzalloc(size, GFP_KERNEL);
    if (!data)
    {
        Close(file);
        return -ENOMEM;
    }

    if (Read(file, data, size) != size)
    {
        kfree(data);
        Close(file);
        return -EIO;
    }

    Close(file);

    firmware = kzalloc(sizeof(struct firmware), GFP_KERNEL);
    if (!firmware)
    {
        kfree(data);
        return -ENOMEM;
    }

    firmware->size = size;
    firmware->data = data;
    *fw = firmware;

    return 0;
}

int firmware_request_nowarn(const struct firmware **fw, const char *name, struct device *device)
{
    return request_firmware(fw, name, device);
}

void release_firmware(const struct firmware *fw)
{
    if (fw)
    {
        if (fw->data)
            kfree(fw->data);
        kfree(fw);
    }
}
