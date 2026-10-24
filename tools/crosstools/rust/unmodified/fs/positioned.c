/*
    Copyright (C) 2026 John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL), Version 1.1.

    Rust positioned I/O uses the native POSIX library's shared-fcb lock.
    Revision 0.41 introduces that contract and the pwrite exports. No
    unlocked seek/read/write fallback is used with an older runtime.
*/
#define _LARGEFILE64_SOURCE 1
#include <errno.h>
#include <unistd.h>
#include <libraries/posixc.h>

/* These declarations also allow a read-only 0.40 SDK plus privately generated
   0.41 import stubs. Signatures match the native public unistd.h. */
extern ssize_t pwrite(int, const void *, size_t, off_t);
extern ssize_t pwrite64(int, const void *, size_t, __off64_t);

static int positioned_available(void)
{
    const struct PosixCBase *base = __aros_getbase_PosixCBase();
    if (base && (base->lib.lib_Version > 0 || base->lib.lib_Revision >= 41))
        return 1;
    errno = ENOSYS;
    return 0;
}

ssize_t aros_rs_pread(int fd, void *buf, size_t count, off_t offset)
{
    return positioned_available() ? pread(fd, buf, count, offset) : -1;
}
ssize_t aros_rs_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    return positioned_available() ? pwrite(fd, buf, count, offset) : -1;
}
ssize_t aros_rs_pread64(int fd, void *buf, size_t count, __off64_t offset)
{
    return positioned_available() ? pread64(fd, buf, count, offset) : -1;
}
ssize_t aros_rs_pwrite64(int fd, const void *buf, size_t count, __off64_t offset)
{
    return positioned_available() ? pwrite64(fd, buf, count, offset) : -1;
}
