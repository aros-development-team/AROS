#ifndef X86_SYSCALLS_H
#define X86_SYSCALLS_H

/*
 * These are x86 specific/private SysCalls that
 * the kernel modules may provide/support
 */

#define SC_X86CHANGEPMSTATE     0xFF
#define SC_X86CPUWAKE           0xFE
#define SC_X86CPUSPINLOCK       0xFD
#define SC_X86SWITCH            0xFC
#define SC_X86RESCHEDTASK       0xFB

#define krnSysCallSwitch() 				                                                \
({								                                        \
    __asm__ __volatile__ ("int $0x80"::"a"(SC_X86SWITCH):"memory");	                                \
})

#define krnSysCallReschedTask(task) 				                                        \
({								                                        \
    __asm__ __volatile__ ("int $0x80"::"a"(SC_X86RESCHEDTASK),"b"(task):"memory");                       \
})

#define krnSysCallSpinLock(spindata) 				                                        \
({								                                        \
    spinlock_t *__value;						                                \
    __asm__ __volatile__ ("int $0x80":"=a"(__value):"a"(SC_X86CPUSPINLOCK),"b"(spindata):"memory");     \
    __value;						                                                \
})

#define krnSysCallCPUWake(wakedata) 				                                        \
({								                                        \
    int __value;						                                        \
    __asm__ __volatile__ ("int $0x80":"=a"(__value):"a"(SC_X86CPUWAKE),"b"(wakedata):"memory");         \
    __value;						                                                \
})

#define krnSysCallChangePMState(state) 				                                        \
({								                                        \
    int __value;						                                        \
    __asm__ __volatile__ ("int $0x80":"=a"(__value):"a"(SC_X86CHANGEPMSTATE),"b"(state):"memory");      \
    __value;						                                                \
})

#endif /* !X86_SYSCALLS_H */
