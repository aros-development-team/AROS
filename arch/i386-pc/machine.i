# Macros
#define AROS_CSYMNAME(n)       n
#define AROS_CDEFNAME(n)       n
#define AROS_SLIB_ENTRY(n,s)   s ## _ ## n

# For sigprocmask
#define SIG_BLOCK     0
#define SIG_UNBLOCK   1

# ExecBase
#define AttnResched   306
#define IDNestCnt     302
#define TDNestCnt     303
#define TaskReady     428
#define ThisTask      284

# struct Task
#define tc_State      17
#define tc_Flags      16
#define tc_ExceptCode 44
#define tc_ExceptData 40
#define tc_SigExcept  32
#define tc_SigRecvd   28
#define tc_Launch     72
#define tc_Switch     68
#define tc_SPReg      56
#define tc_SPLower    60
#define tc_SPUpper    64
#define tc_IDNestCnt  18
#define tc_ETask      36

# struct IntETask
#define iet_Context   76
#define iet_FPU       80
#define iet_CR3	   84

# struct DosBase
#define dl_SysBase    112

# struct StackSwapStruct
#define stk_Lower     0
#define stk_Upper     4
#define stk_Pointer   8

# Task Flags
#define TS_RUN        2
#define TS_READY      3
#define TF_EXCEPT     0x0020
#define TF_SWITCH     0x0040
#define TF_LAUNCH     0x0080

# Exec functions
#define Reschedule    -32
#define Switch        -36
#define Dispatch      -40
#define Exception     -44
#define Alert         -72
#define Disable       -80
#define Enable        -84
#define Enqueue       -180
#define FindTask	   -196
#define StackSwap     -488

# Constants
#define AT_DeadEnd    0x80000000
#define AN_StackProbe 0x0100000E
#define UseExecstubs 1
