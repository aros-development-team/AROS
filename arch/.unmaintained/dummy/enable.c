#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <machine.h>

void enable(struct ExecBase *SysBase)
{
	SysBase->IDNestCnt--;
	if(SysBase->IDNestCnt<0&&
	   SysBase->AttnResched&0x80&&
	   SysBase->TDNestCnt<0)
	{
		SysBase->AttnResched&=~0x80;
		Switch();
	}
}