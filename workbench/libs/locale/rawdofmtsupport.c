/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/* This used QUICK reg layout
 */
#ifdef __MORPHOS__
#define EMUL_QUICKMODE
#include <exec/types.h>
#include <emul/emulinterface.h>
#include <emul/emulregs.h>

char	*_PPCCallM68k_RawDoFmt(char		MyChar,
                               char*		(*PutChProc)(char*,char),
                               char		*PutChData,
                               ULONG 	OldA4,
                               struct ExecBase *sysBase)

{
struct EmulCaos	MyCaos;

  MyCaos.caos_Un.Function	=	PutChProc;
  MyCaos.reg_d0			=(ULONG) MyChar;
  MyCaos.reg_a2			=(ULONG) PutChProc;
  MyCaos.reg_a3			=(ULONG) PutChData;
  MyCaos.reg_a4			=(ULONG) OldA4;
  MyCaos.reg_a6			=(ULONG) sysBase;
  (*MyEmulHandle->EmulCallQuick68k)(&MyCaos);

  return(REG_A3);
}
#endif
