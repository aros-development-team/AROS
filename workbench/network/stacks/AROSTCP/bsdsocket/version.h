#ifdef __mc68000
#define CPU_TYPE "M68K"
#endif
#ifdef __PPC__
#define CPU_TYPE "PowerPC"
#endif
#ifdef __i386__
#define CPU_TYPE "x86"
#endif
#ifdef __arm__
#define CPU_TYPE "ARM"
#endif
#ifdef __x86_64__
#define CPU_TYPE "amd64"
#endif
#ifndef CPU_TYPE
#error Unknown CPU, please define
#endif

#ifndef STR
#define _STR(A) #A
#define STR(A) _STR(A)
#endif

#define RELEASESTRING "Network stack release 4 "
#define SOCLIBNAME      "bsdsocket.library"
#define MIAMILIBNAME    "miami.library"

#define VERSION         4
#define REVISION        55
#define DATE    "09.04.2012"
#define VERS    SOCLIBNAME "4.55"
#define VSTRING SOCLIBNAME STR(VERSION) "." STR(REVISION) "(" DATE ")"
#define VERSTAG "\0$VER:" SOCLIBNAME "4.55 (" DATE ")"

#define MIAMI_VERSION 13
#define MIAMI_REVISION 5
#define MIAMI_VSTRING  MIAMILIBNAME STR(MIAMI_VERSION) "." STR(MIAMI_REVISION) "(" DATE ")"

#if defined(__CONFIG_ROADSHOW__)
#define ROADSHOWSTR "Roadshow "
#else
#define ROADSHOWSTR ""
#endif

#define STACK_RELEASE "AROSTCP " ROADSHOWSTR "kernel v0.23 " CPU_TYPE " (" DATE ")"

