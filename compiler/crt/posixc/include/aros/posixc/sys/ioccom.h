/* Minimal AROS ioctl command encoding definitions used by Mesa DRM UAPI. */
#ifndef _SYS_IOCCOM_H
#define _SYS_IOCCOM_H

#define IOCPARM_MASK    0x1fff
#define IOCPARM_LEN(x)  (((x) >> 16) & IOCPARM_MASK)
#define IOCBASECMD(x)   ((x) & ~(IOCPARM_MASK << 16))
#define IOCGROUP(x)     (((x) >> 8) & 0xff)

#define IOC_VOID   (0x20000000UL)
#define IOC_OUT    (0x40000000UL)
#define IOC_IN     (0x80000000UL)
#define IOC_INOUT  (IOC_IN | IOC_OUT)

#define _IOC(inout, group, num, len) \
    ((unsigned long)(inout) | ((unsigned long)(len) << 16) | \
     ((unsigned long)(group) << 8) | (unsigned long)(num))
#define _IO(g, n)       _IOC(IOC_VOID, (g), (n), 0)
#define _IOR(g, n, t)   _IOC(IOC_OUT, (g), (n), sizeof(t))
#define _IOW(g, n, t)   _IOC(IOC_IN, (g), (n), sizeof(t))
#define _IOWR(g, n, t)  _IOC(IOC_INOUT, (g), (n), sizeof(t))

#endif
