/*
    AROS shim: sys/ioccom.h -> sys/ioctl.h
    drm.h on non-Linux includes sys/ioccom.h for _IOWR etc.
    AROS provides these in sys/ioctl.h.
*/
#ifndef _SYS_IOCCOM_H_AROS_
#define _SYS_IOCCOM_H_AROS_

#include <sys/ioctl.h>

#endif
