/*
 * Exception codes from winbase.h.
 * It appeared very problematic just to #include <windows.h> from AROS code because
 * AROS crosscompiler doesn't look into MinGW include path by default and this
 * path is different on different systems.
 * Looks like the solution needs to be found. The same applies to -idirafter /usr/include
 * for UNIX systems, it works only until someone tries to crosscompile for example
 * Linux-hosted port under MinGW.
 * For now i have these definitions copied here.
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

struct ExceptionTranslation
{
    unsigned int ExceptionCode; /* Windows exception code		       */
    char	 AmigaTrap;	/* m68k trap number for exec.library	       */
    char	 CPUTrap;	/* Native CPU trap number for kernel.resource */
};

extern struct ExceptionTranslation Traps[];
