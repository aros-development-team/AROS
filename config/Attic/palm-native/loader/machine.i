# Macros
#define AROS_CSYMNAME(n)       n
#define AROS_CDEFNAME(n)       n
#define AROS_SLIB_ENTRY(n,s)   s ## _ ## n
#define _FUNCTION(n)           .type   n,@function
#define _ALIGNMENT             .balign 16

# ExecBase
#define IDNestCnt     294
.equ IDNestCnt, 294
#define TaskReady     406
.equ TaskReady, 406
#define ThisTask      276
.equ ThisTask, 276

# struct Task
#define tc_State      0xf
.equ tc_State, 0xf
#define tc_Flags      0xe
.equ tc_Flags, 0xe
#define tc_Launch     0x46
.equ tc_Launch, 0x46
#define tc_Switch     0x42
.equ tc_Switch, 0x42
#define tc_SPReg      0x36
.equ tc_SPReg, 0x36
#define tc_IDNestCnt  0x10
.equ tc_IDNestCnt, 0x10


# Task Flags
#define TS_RUN        2
.equ TS_RUN, 2
#define TS_READY      3
#define TB_EXCEPT     5
#define TB_LAUNCH     7
.equ TB_LAUNCH, 7
#define TB_SWITCH     6
.equ TB_SWITCH, 6
#define TF_EXCEPT     0x0020
#define TF_SWITCH     0x0040
#define TF_LAUNCH     0x0080

# Exec functions
#define Reschedule    -48
#define Switch        -54
#define Dispatch      -60
#define Exception     -66
#define Alert         -108
#define Disable       -120
#define Enable        -126
#define Enqueue       -270
#define FindTask	   -294
#define StackSwap     -732
#define Supervisor    -30
#define CacheControl  -648

