#include <stdarg.h>

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
int core_IRQHandler(unsigned char *irqs, CONTEXT *regs);
