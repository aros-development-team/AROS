/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Exec interrupt handling
    Lang: english
*/
#include <hardware/intbits.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <aros/asmcall.h>
#include <signal.h>
#define timeval     linux_timeval
#include <sys/time.h>
#undef timeval

void disable(void);
static int sig2inttabl[NSIG];
char supervisor;

static void signals(int sig)
{
    struct IntVector *iv;
    supervisor++;
    iv=&SysBase->IntVects[sig2inttabl[sig]];
    AROS_UFC2(void,iv->iv_Code,
    	AROS_UFCA(APTR,iv->iv_Data,A1),
    	AROS_UFCA(struct ExecBase *,SysBase,A6));
    AROS_ASMSYMNAME(disable)();
    supervisor--;
    if(SysBase->AttnResched&0x8000)
    {
        SysBase->AttnResched&=~0x8000;
        Dispatch();
    }
}

void InitCore(void)
{
    static const int sig2int[][2]=
    {
        {   SIGALRM, INTB_VERTB   },
    };
    struct itimerval interval;
    int i;
    struct sigaction sa={ signals, ~0ul, 0, NULL };

    for(i=0;i<sizeof(sig2int)/sizeof(sig2int[0]);i++)
    {
        sig2inttabl[sig2int[i][0]]=sig2int[i][1];
        sigaction(sig2int[i][0],&sa,NULL);
    }

    interval.it_interval.tv_sec = interval.it_value.tv_sec = 0;
    interval.it_interval.tv_usec = interval.it_value.tv_usec = 1000000/50;

    setitimer (ITIMER_REAL, &interval, NULL);
}
