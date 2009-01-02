#include <conf.h>

#include <exec/types.h>
#include <sys/types.h>
#include <api/amiga_kernvars.h>
#include <kern/amiga_netdb.h>
#include <net/pfil.h>

extern struct ifnet *ifnet;
extern struct icmpstat icmpstat;
extern struct rtstat rtstat;
extern struct radix_node_head *rt_tables[];

struct kernel_var kvars[] = {
	{ "_mbstat" , &mbstat },
	{ "_ipstat" , &ipstat },
	{ "_tcb" , &tcb },
	{ "_tcpstat", &tcpstat },
	{ "_udb" , &udb },
	{ "_udpstat" , &udpstat },
	{ "_ifnet" , &ifnet },
	{ "_icmpstat" , &icmpstat },
	{ "_rtstat" , &rtstat },
	{ "_rt_tables" , &rt_tables },
#ifdef ENABLE_IGMP
	{ "_igmpstat" , &igmpstat },
#endif
#ifdef ENABLE_MULTICAST
	{ "_ip_mrtproto" , &ip_mrtptoto },
	{ "_mrtstat" , &mrtstat },
	{ "_mrttable" , &mrttable },
	{ "_viftable" , &viftable },
#endif
	{ NULL , NULL }
};

AROS_LH1(void *, FindKernelVar,
         AROS_LHA(STRPTR, name, A0),
         struct MiamiBase *, MiamiBase, 59, Miami
)
{
	AROS_LIBFUNC_INIT

	struct kernel_var *kvar;

	for (kvar = kvars; kvar->v_name; kvar++)
		if (!strcmp(name, kvar->v_name))
			return kvar->v_addr;
	return NULL;

	AROS_LIBFUNC_EXIT
}

