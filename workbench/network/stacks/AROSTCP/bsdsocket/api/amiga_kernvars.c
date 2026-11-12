#include <conf.h>

#include <exec/types.h>
#include <sys/types.h>
#include <api/amiga_kernvars.h>
#include <kern/amiga_netdb.h>
#include <net/pfil.h>
#include <net/netmodule.h>
#include <kern/netdiag.h>
#include <sys/synch.h>
#include <api/amiga_api.h>      /* syscall_semaphore */

extern struct ifnet *ifnet;
extern struct rtstat rtstat;
extern struct radix_node_head *rt_tables[];
extern struct SignalSemaphore spl_semaphore;
extern spl_t spl_level;
extern ULONG netisr;

/*
 * The core's own. What belongs to a module - the statistics of a family
 * ("_ipstat", "_icmpstat", "_ip6stat", "_icmp6stat", ...), the transports'
 * statistics and lists of PCBs ("_tcpstat", "_udpstat", "_tcb", "_udb") -
 * is not listed: the module registers it (net/netmodule.h) and
 * FindKernelVar() looks it up, so what belongs to a module that is not
 * part of the running stack is not found.
 */
struct kernel_var kvars[] = {
    { "_mbstat", &mbstat },
    { "_ifnet", &ifnet },
    { "_rtstat", &rtstat },
    { "_rt_tables", &rt_tables },
    /*
     * What a program reads in place when the stack has stopped serving
     * (kern/netdiag.h): addresses only, so a stuck stack need not answer.
     */
    { "_netdiag", &netdiag },
    { "_spl_semaphore", &spl_semaphore },
    { "_spl_level", &spl_level },
    { "_syscall_semaphore", &syscall_semaphore },
    { "_netisr", &netisr },
    { NULL, NULL }
};

struct MiamiBase;

AROS_LH1(void *, FindKernelVar,
         AROS_LHA(STRPTR, name, A0),
         struct MiamiBase *, MiamiBase, 59, Miami
        )
{
    AROS_LIBFUNC_INIT

    struct kernel_var *kvar;

    for(kvar = kvars; kvar->v_name; kvar++)
        if(!strcmp(name, kvar->v_name))
            return kvar->v_addr;
    /* a module's block (statistics, a list of PCBs), under its C name
     * with the underscore */
    if(name[0] == '_') {
        const struct netvar *nv = netvar_find(name + 1);

        if(nv != NULL && nv->nv_type == NVT_STATS)
            return nv->nv_addr;
    }
    return NULL;

    AROS_LIBFUNC_EXIT
}

