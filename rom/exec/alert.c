#include <exec/execbase.h>
#include <exec/alerts.h>
#include <aros/libcall.h>
#include <stdio.h>
#include <stdlib.h>

__AROS_LH1(void, Alert,
    __AROS_LA(ULONG, alertNum, D7),
    struct ExecBase *, SysBase, 18, Exec)
{
    __AROS_FUNC_INIT

    printf("Alert:%08lx Task:%08lx\n",alertNum,(ULONG)SysBase->ThisTask);

    if(alertNum&AT_DeadEnd)
	exit(20);
    __AROS_FUNC_EXIT
}

