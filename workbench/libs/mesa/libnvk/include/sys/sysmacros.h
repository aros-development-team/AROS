/* <sys/sysmacros.h> for NVK on AROS: the DRM node numbers it reports are
   placeholders, there being no device nodes. */
#ifndef _NVK_SYS_SYSMACROS_H
#define _NVK_SYS_SYSMACROS_H
#define major(dev) ((unsigned int)(((dev) >> 8) & 0xfff))
#define minor(dev) ((unsigned int)((dev) & 0xff))
#define makedev(maj, min) ((((maj) & 0xfff) << 8) | ((min) & 0xff))
#endif
