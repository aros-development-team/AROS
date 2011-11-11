#ifdef __x86_64__
#define __x86__
#define HAS_CPU_SPECIFIC
#endif

#ifdef __i386__
#define __x86__
#define HAS_CPU_SPECIFIC
#endif

#ifdef __arm__
#define HAS_CPU_SPECIFIC
#endif

#ifdef HAS_CPU_SPECIFIC
void PrintCPUSpecificInfo(ULONG i, APTR ProcessorBase);
#else
#define PrintCPUSpecificInfo(n, base)
#endif
