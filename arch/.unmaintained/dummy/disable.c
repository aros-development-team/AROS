#include <exec/execbase.h>

void disable(struct ExecBase *SysBase)
{
	SysBase->IDNestCnt++;
}