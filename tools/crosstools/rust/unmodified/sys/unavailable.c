/*
    Copyright (C) 2026 John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL), Version 1.1.

    Unavailable interfaces have one provider for std and application libc.
    Explicit adapter symbols avoid duplicate C definitions from Rust crates.
    These functions perform no requested operation and report ENOSYS.
*/
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#define UNUSED __attribute__((unused))
#define REFUSE(type, name, args, failure) \
    type aros_rs_##name args { errno = ENOSYS; return (failure); }

REFUSE(int, poll, (void *fds UNUSED, unsigned int nfds UNUSED, int timeout UNUSED), -1)
REFUSE(intptr_t, fork, (void), (intptr_t)-1)
REFUSE(int, mkfifo, (const char *path UNUSED, unsigned short mode UNUSED), -1)
REFUSE(int, lchown, (const char *path UNUSED, unsigned int uid UNUSED, unsigned int gid UNUSED), -1)
REFUSE(int, chroot, (const char *name UNUSED), -1)
REFUSE(int, setpgid, (intptr_t pid UNUSED, intptr_t group UNUSED), -1)
REFUSE(int, setgroups, (int count UNUSED, const unsigned int *groups UNUSED), -1)
REFUSE(int, mlock, (const void *addr UNUSED, size_t size UNUSED), -1)
REFUSE(int, munlock, (const void *addr UNUSED, size_t size UNUSED), -1)
REFUSE(int, mlockall, (int flags UNUSED), -1)
REFUSE(int, munlockall, (void), -1)
REFUSE(int, munmap, (void *addr UNUSED, size_t size UNUSED), -1)
REFUSE(int, setreuid, (unsigned int real UNUSED, unsigned int effective UNUSED), -1)
REFUSE(int, setregid, (unsigned int real UNUSED, unsigned int effective UNUSED), -1)
REFUSE(unsigned int, alarm, (unsigned int seconds UNUSED), UINT_MAX)
REFUSE(int, killpg, (intptr_t group UNUSED, int signal UNUSED), -1)
REFUSE(long, fpathconf, (int fd UNUSED, int name UNUSED), -1L)
REFUSE(int, getrusage, (int resource UNUSED, void *usage UNUSED), -1)
REFUSE(unsigned int, if_nametoindex, (const char *name UNUSED), 0U)
REFUSE(int, res_init, (void), -1)
REFUSE(int, gethostname, (char *name UNUSED, size_t size UNUSED), -1)
REFUSE(int, execvpe, (const char *file UNUSED, const char *const *argv UNUSED,
                    const char *const *envp UNUSED), -1)
REFUSE(int, execle, (const char *path UNUSED, const char *arg0 UNUSED, ...), -1)
REFUSE(int, linkat, (int oldfd UNUSED, const char *oldpath UNUSED,
                   int newfd UNUSED, const char *newpath UNUSED, int flags UNUSED), -1)
REFUSE(int, renameat, (int oldfd UNUSED, const char *oldpath UNUSED,
                     int newfd UNUSED, const char *newpath UNUSED), -1)
REFUSE(int, symlinkat, (const char *target UNUSED, int fd UNUSED, const char *path UNUSED), -1)
REFUSE(int, fchownat, (int fd UNUSED, const char *path UNUSED, unsigned int uid UNUSED,
                     unsigned int gid UNUSED, int flags UNUSED), -1)
REFUSE(int, lockf, (int fd UNUSED, int command UNUSED, long size UNUSED), -1)
REFUSE(int, ttyname_r, (int fd UNUSED, char *buffer UNUSED, size_t size UNUSED), -1)
REFUSE(int, tcsetpgrp, (int fd UNUSED, intptr_t group UNUSED), -1)
REFUSE(char *, mkdtemp, (char *template UNUSED), NULL)
REFUSE(void *, mmap, (void *addr UNUSED, size_t size UNUSED, int prot UNUSED,
                     int flags UNUSED, int fd UNUSED, long offset UNUSED), (void *)(intptr_t)-1)

const char *aros_rs_hstrerror(int error UNUSED)
{
    return "name resolution is not available";
}
