#ifndef COMPILER_H
#define COMPILER_H

#ifndef __AROS__
#error "write compiler.h!"
#endif

#include <aros/asmcall.h>

#define REGARGS(x) x
#define ALIGNED
#define REG(reg,var) var
#define MREG(reg,type,var) 

#define LibCall
#define ClassCall
#define RegCall
#define GetA4

#endif
