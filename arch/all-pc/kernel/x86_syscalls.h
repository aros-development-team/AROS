#ifndef X86_SYSCALLS_H
#define X86_SYSCALLS_H

/*
 * These are x86 specific/private SysCalls that
 * the kernel modules may provide/support
 */

#define SC_X86SHUTDOWN          0xFF
#define SC_X86CPUWAKE             0xFE

#endif /* !X86_SYSCALLS_H */
