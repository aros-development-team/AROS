/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_FIRMWARE_H_
#define _LINUX_FIRMWARE_H_

#include <linux/types.h>
#include <linux/device.h>
#include <linux/gfp.h>
#include <linux/compiler.h>

struct firmware {
    size_t size;
    const u8 *data;
    void *priv;
};

/*
 * Firmware files are read from DEVS:Firmware/<name>, the same names as
 * linux-firmware uses (nvidia/<chip>/...).
 */
int  request_firmware(const struct firmware **fw, const char *name, struct device *device);
int  firmware_request_nowarn(const struct firmware **fw, const char *name, struct device *device);
int  request_firmware_direct(const struct firmware **fw, const char *name, struct device *device);
int  request_firmware_into_buf(const struct firmware **fw, const char *name, struct device *device, void *buf, size_t size);
void release_firmware(const struct firmware *fw);
#define request_firmware_nowait(m, u, n, d, g, c, cb) (-ENOSYS)
#define firmware_request_platform(fw, n, d)         request_firmware(fw, n, d)
#define firmware_request_cache(d, n)                (0)

#endif /* _LINUX_FIRMWARE_H_ */
