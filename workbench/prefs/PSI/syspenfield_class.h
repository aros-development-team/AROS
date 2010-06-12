#ifndef SYSPENFIELD_CLASS_H
#define SYSPENFIELD_CLASS_H

#include "psi.h"

struct MUI_CustomClass *CL_SysPenField;

Object *MakeMUIPen(int nr,Object **adr);
Object *MakeSysPen(int nr,Object **adr);

VOID SysPenField_Init(VOID);
VOID SysPenField_Exit(VOID);

#endif
