/*
 * This driver uses direct hardware access, so it runs only on x86 machines.
 * Additionally it uses iopl syscall which is available only on Linux.
 *
 * Since this driver is so machine-specific, we use direct syscalls and inline them
 * here. If this driver is ever made portable and machine-independent, this will need to
 * be replaced with libc functions called via hostlib.resource (on some UNIXes, for
 * example on Darwin, syscalls are private API and no binary compatibility is
 * guaranteed accross OS versions).
 */

static inline int syscall1(IPTR num, IPTR arg1)
{
    int ret;

    asm volatile(
		"int $0x80"
		:"=a"(ret)
		:"a"(num), "b"(arg1)
    );
    return ret;
}

static inline int syscall2(IPTR num, IPTR arg1, IPTR arg2)
{
    int ret;

    asm volatile(
		"int $0x80"
		:"=a"(ret)
		:"a"(num), "b"(arg1), "c"(arg2)
    );
    return ret;
}
