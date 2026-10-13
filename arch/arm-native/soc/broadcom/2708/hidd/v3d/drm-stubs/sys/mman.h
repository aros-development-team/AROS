/* AROS stub for sys/mman.h. Buffer objects are mapped by the DRM shim,
 * which hands back the allocation's own address - see v3d_drm_shim.c. */
#ifndef SYS_MMAN_H_AROS
#define SYS_MMAN_H_AROS

#include <stddef.h>

#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define MAP_SHARED  0x01
#define MAP_FAILED  ((void *)-1)

void *mmap(void *addr, unsigned long length, int prot, int flags, int fd,
           long offset);
int munmap(void *addr, unsigned long length);

#endif
