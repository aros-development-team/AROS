#include <stdarg.h>

/*
 * Exception codes from winbase.h.
 * It's very problematic just to #include <windows.h> from AROS code because
 * AROS crosscompiler doesn't look into MinGW include path by default and this
 * path is different on different systems.
 * Looks like the solution needs to be found. For now i have these definitions
 * copied here.
 */
#define EXCEPTION_GUARD_PAGE		0x80000001
#define EXCEPTION_DATATYPE_MISALIGNMENT	0x80000002
#define EXCEPTION_BREAKPOINT		0x80000003
#define EXCEPTION_SINGLE_STEP		0x80000004
#define EXCEPTION_ACCESS_VIOLATION	0xC0000005
#define EXCEPTION_IN_PAGE_ERROR		0xC0000006
#define EXCEPTION_ILLEGAL_INSTRUCTION	0xC000001D
#define EXCEPTION_ARRAY_BOUNDS_EXCEEDED	0xC000008C
#define EXCEPTION_FLT_DIVIDE_BY_ZERO	0xC000008E
#define EXCEPTION_INT_DIVIDE_BY_ZERO	0xC0000094
#define EXCEPTION_PRIV_INSTRUCTION	0xC0000096

/* WinAPI Memory protection constants */
#define PAGE_NOACCESS		0x0001
#define PAGE_READONLY		0x0002
#define PAGE_READWRITE		0x0004
#define PAGE_WRITECOPY		0x0008
#define PAGE_EXECUTE		0x0010
#define PAGE_EXECUTE_READ	0x0020
#define PAGE_EXECUTE_READWRITE	0x0040
#define PAGE_EXECUTE_WRITECOPY	0x0080
#define PAGE_GUARD		0x0100
#define PAGE_NOCACHE		0x0200
#define PAGE_WRITECOMBINE	0x0400

extern struct HostInterface *HostIFace;

int core_TrapHandler(unsigned int num, IPTR *args, CONTEXT *regs);
void core_IRQHandler(unsigned int num, CONTEXT *regs);

#undef kprintf
#undef vkprintf
#undef rkprintf

int mykprintf(const char * fmt, ...);
int myvkprintf(const char *fmt, va_list args);
int myrkprintf(const char *foo, const char *bar, int baz, const char *fmt, ...);
