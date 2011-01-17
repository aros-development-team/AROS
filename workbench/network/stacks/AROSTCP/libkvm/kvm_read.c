#include <exec/types.h>
#include <fcntl.h>
#include <kvm.h>
#include <string.h>
#include "kvm_errors.h"

ssize_t kvm_read(kvm_t *kd, unsigned long addr, void *buf, size_t nbytes)
{
    if (kd == (kvm_t *)(O_WRONLY|0xdead0000)) {
	__kvm_errno = KVMERR_NOACCESS;
	return -1;
    }
    if (addr) {
	memcpy (buf, (void *)addr, nbytes);
	return nbytes;
    } else {
	__kvm_errno = KVMERR_BADADDRESS;
	return -1;
    }
}
