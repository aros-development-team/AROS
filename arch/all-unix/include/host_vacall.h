/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Calling variadic host functions from AROS code.
*/

#ifndef HOST_VACALL_H
#define HOST_VACALL_H

/*
 * Darwin's arm64 ABI passes variadic arguments on the stack, while AROS code
 * uses plain AAPCS64, which passes them in registers. A host variadic function
 * (open(), ioctl(), fcntl()) called through a "..." pointer therefore reads its
 * third argument from the stack and gets garbage. Call it through a prototype
 * with six register fillers instead, so that argument lands in the first stack
 * slot, where Darwin's va_arg looks for it. On other hosts it's the plain call.
 */
#if defined(HOST_OS_darwin) && defined(__aarch64__)
#define HOST_VACALL3(fn, a, b, c) \
    (((int (*)(long, long, long, long, long, long, long, long, long))(fn)) \
        ((long)(a), (long)(b), 0, 0, 0, 0, 0, 0, (long)(c)))
#else
#define HOST_VACALL3(fn, a, b, c) ((fn)(a, b, c))
#endif

#endif /* HOST_VACALL_H */
