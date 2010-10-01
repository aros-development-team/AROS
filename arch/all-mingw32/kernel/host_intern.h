struct LeaveInterruptContext
{
    UINT_PTR pc;
    UINT_PTR r0;
};

extern HANDLE conin, conout;

void core_LeaveInterrupt(void);
