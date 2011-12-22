#ifdef __x86_64__
#define __aros __attribute__((sysv_abi))
#else
#define __aros
#endif

struct LeaveInterruptContext
{
    UINT_PTR pc;
    UINT_PTR r0;
};

extern HANDLE conin, conout;

void core_LeaveInterrupt(void);
extern unsigned char core_LeaveInt_End;
