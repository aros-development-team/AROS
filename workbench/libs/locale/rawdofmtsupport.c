/* This used QUICK reg layout
 */
#define EMUL_QUICKMODE
#include <exec/types.h>
#include <emul/emulinterface.h>
#include <emul/emulregs.h>

#define	DEBUG(x)	;

char	*_PPCCallM68k_RawDoFmt(char		MyChar,
                               char*		(*PutChProc)(char*,char),
                               char		*PutChData)

{
struct EmulCaos	MyCaos;

  MyCaos.caos_Un.Function	=	PutChProc;
  MyCaos.reg_d0			=(ULONG) MyChar;
  MyCaos.reg_a2			=(ULONG) PutChProc;
  MyCaos.reg_a3			=(ULONG) PutChData;
  (*MyEmulHandle->EmulCallQuick68k)(&MyCaos);

  return(REG_A3);
}
