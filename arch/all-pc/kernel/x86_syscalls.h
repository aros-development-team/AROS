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
#define SC_X86SYSHALT           0xFA

#define krnSysCallSystemHalt() 				                                        \
({								                                        \
    __asm__ __volatile__ ("int $0xfe"::"a"(SC_X86SYSHALT):"memory");	                                \
})

#define krnSysCallSwitch() 				                                                \
({								                                        \
    __asm__ __volatile__ ("int $0xfe"::"a"(SC_X86SWITCH):"memory");	                                \
})

#define krnSysCallReschedTask(task, state) 				                                        \
({								                                        \
    __asm__ __volatile__ ("int $0xfe"::"a"(SC_X86RESCHEDTASK),"b"(task),"c"(state):"memory");                       \
})

#define krnSysCallSpinLock(spindata) 				                                        \
({								                                        \
    spinlock_t *__value;						                                \
    __asm__ __volatile__ ("int $0xfe":"=a"(__value):"a"(SC_X86CPUSPINLOCK),"b"(spindata):"memory");     \
    __value;						                                                \
})

#define krnSysCallCPUWake(wakedata) 				                                        \
({								                                        \
    int __value;						                                        \
    __asm__ __volatile__ ("int $0xfe":"=a"(__value):"a"(SC_X86CPUWAKE),"b"(wakedata):"memory");         \
    __value;						                                                \
})

#define krnSysCallChangePMState(state) 				                                        \
({								                                        \
    int __value;						                                        \
    __asm__ __volatile__ ("int $0xfe":"=a"(__value):"a"(SC_X86CHANGEPMSTATE),"b"(state):"memory");      \
    __value;						                                                \
})

extern void X86_HandleSysHaltSC(struct ExceptionContext *);
extern void X86_HandleChangePMStateSC(struct ExceptionContext *);
extern struct syscallx86_Handler x86_SCRebootHandler;
extern struct syscallx86_Handler x86_SCChangePMStateHandler;
extern struct syscallx86_Handler x86_SCSysHaltHandler;

#endif /* !X86_SYSCALLS_H */
