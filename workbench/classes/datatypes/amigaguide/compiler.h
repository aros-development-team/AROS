#ifndef COMPILER_H
#define COMPILER_H

#ifndef __AROS__
#define MREG(reg,type,var) type var = REG_##reg
#endif

#include <aros/asmcall.h>

#define REGARGS(x) x
#define ALIGNED
#define REG(reg,var) var
#ifndef MREG
#define MREG(reg,type,var) 
#endif

#define LibCall
#define ClassCall
#define RegCall
#define GetA4

#endif
