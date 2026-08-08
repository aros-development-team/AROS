/*
    AROS stub for sys/mman.h
    mmap/munmap are implemented in mesa3dgl-side aros_drm_shim.c.
*/
#ifndef _SYS_MMAN_H_AROS_
#define _SYS_MMAN_H_AROS_

#include <stddef.h>

#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define MAP_SHARED  0x01
#define MAP_FAILED  ((void *)-1)

void *mmap(void *addr, unsigned long length, int prot, int flags, int fd, long offset);
int munmap(void *addr, unsigned long length);

#endif
