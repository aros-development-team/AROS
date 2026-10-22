/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
   Licensed under the AROS Public License (APL), Version 1.1. */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
struct Library { unsigned short lib_Version, lib_Revision; };
struct PosixCBase { struct Library lib; };
static struct PosixCBase base;
static int absent, called;
static struct PosixCBase *__aros_getbase_PosixCBase(void) { return absent ? NULL : &base; }
static ssize_t forwarded(int fd, const void *buf, size_t n, int64_t offset) {
    assert(fd == 7 && buf != NULL && n == 3 && offset == INT64_C(0x100000007));
    ++called; errno = EIO; return -1;
}
#define pread test_pread
#define pwrite test_pwrite
#define pread64 test_pread64
#define pwrite64 test_pwrite64
static ssize_t test_pread(int fd, void *buf, size_t n, off_t pos) { return forwarded(fd,buf,n,pos); }
static ssize_t test_pwrite(int fd, const void *buf, size_t n, off_t pos) { return forwarded(fd,buf,n,pos); }
static ssize_t test_pread64(int fd, void *buf, size_t n, int64_t pos) { return forwarded(fd,buf,n,pos); }
static ssize_t test_pwrite64(int fd, const void *buf, size_t n, int64_t pos) { return forwarded(fd,buf,n,pos); }
#include "production-positioned.c"
static void check(int expected) {
    char buf[3]; int before = called;
    errno = 0; assert(aros_rs_pread(7,buf,3,INT64_C(0x100000007)) == -1); assert(errno == expected);
    errno = 0; assert(aros_rs_pwrite(7,buf,3,INT64_C(0x100000007)) == -1); assert(errno == expected);
    errno = 0; assert(aros_rs_pread64(7,buf,3,INT64_C(0x100000007)) == -1); assert(errno == expected);
    errno = 0; assert(aros_rs_pwrite64(7,buf,3,INT64_C(0x100000007)) == -1); assert(errno == expected);
    assert(called == before + (expected == EIO ? 4 : 0));
}
int main(void) {
    absent = 1; check(ENOSYS); absent = 0;
    base.lib.lib_Revision = 40; check(ENOSYS);
    base.lib.lib_Revision = 41; check(EIO);
    base.lib.lib_Revision = 42; check(EIO);
    base.lib.lib_Revision = 0; base.lib.lib_Version = 1; check(EIO);
    return 0;
}
