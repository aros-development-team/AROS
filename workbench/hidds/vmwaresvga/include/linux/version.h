#ifndef _LINUX_VERSION_H_
#define _LINUX_VERSION_H_
/* The Linux release the vmwgfx driver was taken from. */
#define LINUX_VERSION_CODE      ((6 << 16) + (18 << 8) + 44)
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + ((c) > 255 ? 255 : (c)))
#define LINUX_VERSION_MAJOR     6
#define LINUX_VERSION_PATCHLEVEL 18
#define LINUX_VERSION_SUBLEVEL  44
#endif
