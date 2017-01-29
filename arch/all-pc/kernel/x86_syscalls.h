#ifndef X86_SYSCALLS_H
#define X86_SYSCALLS_H

/*
 * These are x86 specific/private SysCalls that
 * the kernel modules may provide/support
 */

#define SC_X86CHANGEPMSTATE     0xFF
#define SC_X86CPUWAKE           0xFE

#define krnSysCallCPUWake(wakedata) 				        \
({								        \
    int __value;						        \
    __asm__ __volatile__ ("int $0x80":"=a"(__value):"a"(SC_X86CPUWAKE),"b"(wakedata):"memory");	\
    __value;						                \
})

#define krnSysCallChangePMState(state) 				        \
({								        \
    int __value;						        \
    __asm__ __volatile__ ("int $0x80":"=a"(__value):"a"(SC_X86CHANGEPMSTATE),"b"(state):"memory");	\
    __value;						                \
})

#endif /* !X86_SYSCALLS_H */
