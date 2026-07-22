#include <aros/cpu.h>

#if AROS_BIG_ENDIAN
#define ENDIANNESS_DEF ENDIANNESS_BE
#else
#define ENDIANNESS_DEF ENDIANNESS_LE
#endif

#ifdef __i386__
#define PROCESSORARCH_DEF PROCESSORARCH_X86
#endif
#ifdef __x86_64__
#define PROCESSORARCH_DEF PROCESSORARCH_X86
#endif
#ifdef __mc68000__
#define PROCESSORARCH_DEF PROCESSORARCH_M68K
#endif
#ifdef __ppc__
#define PROCESSORARCH_DEF PROCESSORARCH_PPC
#endif
#ifdef __arm__
#define PROCESSORARCH_DEF PROCESSORARCH_ARM
#endif
#ifdef __aarch64__
/* AArch64 is the 64-bit execution state of the ARM architecture. There is no
   separate PROCESSORARCH_ value, so report it as ARM (the architecture family);
   the 64-bit distinction is conveyed by the arch name / the CPUInfo command. */
#define PROCESSORARCH_DEF PROCESSORARCH_ARM
#endif
#ifndef PROCESSORARCH_DEF
#define PROCESSORARCH_DEF PROCESSORARCH_UNKNOWN
#endif
