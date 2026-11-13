/* sys/mman.h stub for AROS - mmap not available */
#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H 1

/* AROS does not have mmap; BINARY_LEASES must not be defined */

/* Define minimal constants so code that guards mmap via HAVE_MMAP
 * or uses these constants compiles. The actual mmap calls in conflex.c
 * are replaced with malloc+read via the __AROS__ guard below. */
#define PROT_READ   0x1
#define MAP_SHARED  0x1
#define MAP_FAILED  ((void *)-1)

static inline void *mmap(void *addr, unsigned long len, int prot, int flags,
			  int fd, long offset)
{
	(void)addr; (void)prot; (void)flags; (void)offset;
	/* Return MAP_FAILED so callers fall back to read-based I/O */
	return MAP_FAILED;
}

static inline int munmap(void *addr, unsigned long len)
{
	(void)addr; (void)len;
	return 0;
}

#endif /* _SYS_MMAN_H */
