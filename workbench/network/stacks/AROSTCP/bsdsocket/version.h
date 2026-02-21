/* CPU_TYPE: compile-time CPU architecture string */
#if defined(__aarch64__) || defined(_M_ARM64)
  #define CPU_TYPE "AARCH64"

#elif defined(__arm__) || defined(_M_ARM)
  #define CPU_TYPE "ARM"

#elif defined(__x86_64__) || defined(_M_X64)
  #define CPU_TYPE "x86_64"

#elif defined(__i386__) || defined(_M_IX86)
  #define CPU_TYPE "x86"

#elif defined(__PPC__) || defined(__powerpc__) || defined(__powerpc64__) || defined(_M_PPC)
  #define CPU_TYPE "PowerPC"

#elif defined(__mc68000__)
  #define CPU_TYPE "M68K"

#elif defined(__riscv)
  #if defined(__riscv_xlen) && (__riscv_xlen == 64)
    #define CPU_TYPE "RISC-V 64bit"
  #else
    #define CPU_TYPE "RISC-V"
  #endif

#else
  #define CPU_TYPE "Unknown"
#endif

#ifndef STR
#define _STR(A) #A
#define STR(A) _STR(A)
#endif

#define RELEASESTRING "Network stack release 5 "
#define SOCLIBNAME      "bsdsocket.library"
#define MIAMILIBNAME    "miami.library"

#define VERSION         5
#define REVISION        0
#define DATE    "21.02.2026"
#define VERS    SOCLIBNAME "5.00"
#define VSTRING SOCLIBNAME STR(VERSION) "." STR(REVISION) "(" DATE ")"
#define VERSTAG "\0$VER:" SOCLIBNAME "5.00 (" DATE ")"

#define MIAMI_VERSION 13
#define MIAMI_REVISION 6
#define MIAMI_VSTRING  MIAMILIBNAME STR(MIAMI_VERSION) "." STR(MIAMI_REVISION) "(" DATE ")"

#if defined(__CONFIG_ROADSHOW__)
#define ROADSHOWSTR "Roadshow "
#else
#define ROADSHOWSTR ""
#endif

#define STACK_RELEASE "AROSTCP " ROADSHOWSTR "kernel v0.26 " CPU_TYPE " (" DATE ")"

