#define AROS_CDEFNAME(n)       _ ## n
#define AROS_SLIB_ENTRY(n,s)   s ## _ ## n

# Exec functions
#define Supervisor    -30
#define Reschedule    -48
#define Switch        -54
#define Dispatch      -60
#define Exception     -66
#define Alert         -108
#define Disable       -120
#define Enable        -126
#define SetSR         -144
#define Enqueue       -270
#define FindTask      -294
#define CacheControl  -648
#define StackSwap     -732

/*
 * Define the following when function calls pass arguments via registers.
 * This is used for conditional compilation in some assembly functions here.
 */
#undef DoRegisterCalls
