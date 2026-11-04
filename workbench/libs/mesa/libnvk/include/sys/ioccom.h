/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The DRM uapi's ioctl numbers as nvgfx.hidd's kernel driver decodes
    them: Linux's encoding. AROS's own sys/ioccom.h has the BSD one, whose
    read and write direction bits are the other way round, so every
    write-only or read-only request from NVK would match nothing.
*/

#ifndef NVK_SYS_IOCCOM_H
#define NVK_SYS_IOCCOM_H

#include "../../../../hidds/nvgfx/include/asm/ioctl.h"

#endif /* NVK_SYS_IOCCOM_H */
