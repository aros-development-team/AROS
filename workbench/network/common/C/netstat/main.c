/*
 * Copyright (c) 1983, 1988, 1993
 *	Regents of the University of California.  All rights reserved.
 * Copyright (c) 2006
 *	Pavel Fedin
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*

"@(#) Copyright (c) 1983, 1988, 1993
	Regents of the University of California.  All rights reserved."

"@(#)main.c	 8.4 (Berkeley) 3/1/94"

*/

#include <sys/param.h>
#include <sys/file.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <ctype.h>
#include <errno.h>
#include <kvm.h>
#include <limits.h>
#include <netdb.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <proto/miami.h>
#include "netstat.h"

struct nlist nl[] = {
#define	N_MBSTAT	0
	{ "_mbstat" },
#define	N_IPSTAT	1
	{ "_ipstat" },
#define	N_TCB		2
	{ "_tcb" },
#define	N_TCPSTAT	3
	{ "_tcpstat" },
#define	N_UDB		4
	{ "_udb" },
#define	N_UDPSTAT	5
	{ "_udpstat" },
#define	N_IFNET		6
	{ "_ifnet" },
#define	N_IMP		7
	{ "_imp_softc" },
#define	N_ICMPSTAT	8
	{ "_icmpstat" },
#define	N_RTSTAT	9
	{ "_rtstat" },
#define	N_UNIXSW	10
	{ "_unixsw" },
#define N_IDP		11
	{ "_nspcb"},
#define N_IDPSTAT	12
	{ "_idpstat"},
#define N_SPPSTAT	13
	{ "_spp_istat"},
#define N_NSERR		14
	{ "_ns_errstat"},
#define	N_CLNPSTAT	15
	{ "_clnp_stat"},
#define	IN_NOTUSED	16
	{ "_tp_inpcb" },
#define	ISO_TP		17
	{ "_tp_refinfo" },
#define	N_TPSTAT	18
	{ "_tp_stat" },
#define	N_ESISSTAT	19
	{ "_esis_stat"},
#define N_NIMP		20
	{ "_nimp"},
#define N_RTREE		21
	{ "_rt_tables"},
#define N_CLTP		22
	{ "_cltb"},
#define N_CLTPSTAT	23
	{ "_cltpstat"},
#define	N_NFILE		24
	{ "_nfile" },
#define	N_FILE		25
	{ "_file" },
#define N_IGMPSTAT	26
	{ "_igmpstat" },
#define N_MRTPROTO	27
	{ "_ip_mrtproto" },
#define N_MRTSTAT	28
	{ "_mrtstat" },
#define N_MRTTABLE	29
	{ "_mrttable" },
#define N_VIFTABLE	30
	{ "_viftable" },
	"",
};

struct protox {
	u_char	pr_index;		/* index into nlist of cb head */
	u_char	pr_sindex;		/* index into nlist of stat block */
	u_char	pr_wanted;		/* 1 if wanted, 0 otherwise */
	void	(*pr_cblocks)();	/* control blocks printing routine */
	void	(*pr_stats)();		/* statistics printing routine */
	char	*pr_name;		/* well-known name */
} protox[] = {
	{ N_TCB,	N_TCPSTAT,	1,	protopr,
	  tcp_stats,	"tcp" },
	{ N_UDB,	N_UDPSTAT,	1,	protopr,
	  udp_stats,	"udp" },
	{ -1,		N_IPSTAT,	1,	0,
	  ip_stats,	"ip" },
	{ -1,		N_ICMPSTAT,	1,	0,
	  icmp_stats,	"icmp" },
#ifdef ENABLE_IGMP
	{ -1,		N_IGMPSTAT,	1,	0,
	  igmp_stats,	"igmp" },
#endif
	{ -1,		-1,		0,	0,
	  0,		0 }
};

#ifdef NETNS
struct protox nsprotox[] = {
	{ N_IDP,	N_IDPSTAT,	1,	nsprotopr,
	  idp_stats,	"idp" },
	{ N_IDP,	N_SPPSTAT,	1,	nsprotopr,
	  spp_stats,	"spp" },
	{ -1,		N_NSERR,	1,	0,
	  nserr_stats,	"ns_err" },
	{ -1,		-1,		0,	0,
	  0,		0 }
};
#endif

#ifdef ISO
struct protox isoprotox[] = {
	{ ISO_TP,	N_TPSTAT,	1,	iso_protopr,
	  tp_stats,	"tp" },
	{ N_CLTP,	N_CLTPSTAT,	1,	iso_protopr,
	  cltp_stats,	"cltp" },
	{ -1,		N_CLNPSTAT,	1,	 0,
	  clnp_stats,	"clnp"},
	{ -1,		N_ESISSTAT,	1,	 0,
	  esis_stats,	"esis"},
	{ -1,		-1,		0,	0,
	  0,		0 }
};
#endif

struct protox *protoprotox[] = { protox,
#ifdef NETNS
				 nsprotox,
#endif
#ifdef ISO
				 isoprotox,
#endif
                                 NULL };

static void printproto __P((struct protox *, char *));
static void usage __P((void));
static struct protox *name2protox __P((char *));
static struct protox *knownname __P((char *));

kvm_t *kvmd;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	register struct protoent *p;
	register struct protox *tp;	/* for printing cblocks & stats */
	register char *cp;
	int ch;
	char *nlistf = NULL, *memf = NULL;
	char buf[_POSIX2_LINE_MAX];
	char buf2[_POSIX2_LINE_MAX];

	if (cp = rindex(argv[0], '/'))
		prog = cp + 1;
	else
		prog = argv[0];
	af = AF_UNSPEC;

	while ((ch = getopt(argc, argv, "Aabdf:ghI:iM:mN:np:rstuw:")) != EOF)
		switch(ch) {
		case 'A':
			Aflag = 1;
			break;
		case 'a':
			aflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			if (strcmp(optarg, "ns") == 0)
				af = AF_NS;
			else if (strcmp(optarg, "inet") == 0)
				af = AF_INET;
			else if (strcmp(optarg, "unix") == 0)
				af = AF_UNIX;
			else if (strcmp(optarg, "iso") == 0)
				af = AF_ISO;
			else {
				errx(1, "%s: unknown address family", optarg);
			}
			break;
		case 'g':
			gflag = 1;
			break;
		case 'I': {
			char *cp;

			iflag = 1;
			for (cp = interface = optarg; isalpha(*cp); cp++)
				continue;
			unit = atoi(cp);
			*cp = '\0';
			break;
		}
		case 'i':
			iflag = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'p':
			if ((tp = name2protox(optarg)) == NULL) {
				errx(1,
				     "%s: unknown or uninstrumented protocol",
				     optarg);
			}
			pflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 's':
			++sflag;
			break;
		case 't':
			tflag = 1;
			break;
		case 'u':
			af = AF_UNIX;
			break;
		case 'w':
			interval = atoi(optarg);
			iflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		if (isdigit(**argv)) {
			interval = atoi(*argv);
			if (interval <= 0)
				usage();
			++argv;
			iflag = 1;
		}
		if (*argv) {
			nlistf = *argv;
			if (*++argv)
				memf = *argv;
		}
	}
#endif

	kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf);
	if (kvmd == NULL) {
		errx(1, "kvm_open: %s", buf);
	}
	if (kvm_nlist(kvmd, nl) < 0) {
		if(nlistf)
			errx(1, "%s: kvm_nlist: %s", nlistf, kvm_geterr(kvmd));
		else
			errx(1, "kvm_nlist: %s", kvm_geterr(kvmd));
	}

	if (nl[0].n_type == 0) {
		if(nlistf)
			errx(1, "%s: no namelist", nlistf);
		else
			errx(1, "no namelist");
	}
	if (mflag) {
		mbpr(nl[N_MBSTAT].n_value);
		exit(0);
	}
	if (pflag) {
		if (tp->pr_stats)
			(*tp->pr_stats)(nl[tp->pr_sindex].n_value,
				tp->pr_name);
		else
			printf("%s: no stats routine\n", tp->pr_name);
		exit(0);
	}
	/*
	 * Keep file descriptors open to avoid overhead
	 * of open/close on each call to get* routines.
	 */
	if (iflag) {
		intpr(interval, nl[N_IFNET].n_value);
		exit(0);
	}
	if (rflag) {
		if (sflag)
			rt_stats(nl[N_RTSTAT].n_value);
		else
			routepr(nl[N_RTREE].n_value);
		exit(0);
	}
	if (gflag) {
#ifdef ENABLE_MULTICAST
		if (sflag)
			mrt_stats(nl[N_MRTPROTO].n_value,
			    nl[N_MRTSTAT].n_value);
		else
			mroutepr(nl[N_MRTPROTO].n_value,
			    nl[N_MRTTABLE].n_value,
			    nl[N_VIFTABLE].n_value);
#else
		printf ("Multicast routing is not supported\n");
#endif
		exit(0);
	}
	if (af == AF_INET || af == AF_UNSPEC) {
		/* ugh, this is O(MN) ... why do we do this? */
		while (p = getprotoent()) {
			for (tp = protox; tp->pr_name; tp++)
				if (strcmp(tp->pr_name, p->p_name) == 0)
					break;
			if (tp->pr_name == 0 || tp->pr_wanted == 0)
				continue;
			printproto(tp, p->p_name);
		}
		endprotoent();
	}
#ifdef NETNS
	if (af == AF_NS || af == AF_UNSPEC)
		for (tp = nsprotox; tp->pr_name; tp++)
			printproto(tp, tp->pr_name);
#endif
#ifdef ISO
	if (af == AF_ISO || af == AF_UNSPEC)
		for (tp = isoprotox; tp->pr_name; tp++)
			printproto(tp, tp->pr_name);
#endif
#ifdef ENABLE_UNIX_DOMAIN
	if ((af == AF_UNIX || af == AF_UNSPEC) && !sflag)
		unixpr(nl[N_UNIXSW].n_value);
#endif
	exit(0);
}

/*
 * Print out protocol statistics or control blocks (per sflag).
 * If the interface was not specifically requested, and the symbol
 * is not in the namelist, ignore this one.
 */
static void
printproto(tp, name)
	register struct protox *tp;
	char *name;
{
	void (*pr)();
	u_long off;

	if (sflag) {
		pr = tp->pr_stats;
		off = nl[tp->pr_sindex].n_value;
	} else {
		pr = tp->pr_cblocks;
		off = nl[tp->pr_index].n_value;
	}
	if (pr != NULL && (off || af != AF_UNSPEC))
		(*pr)(off, name);
}

/*
 * Read kernel memory, return 0 on success.
 */
int
kread(addr, buf, size)
	u_long addr;
	char *buf;
	int size;
{

	if (kvm_read(kvmd, addr, buf, size) != size) {
		warnx("kvm_read: %s", kvm_geterr(kvmd));
		return (-1);
	}
	return (0);
}

char *
plural(n)
	int n;
{
	return (n != 1 ? "s" : "");
}

char *
plurales(n)
	int n;
{
	return (n != 1 ? "es" : "");
}

/*
 * Find the protox for the given "well-known" name.
 */
static struct protox *
knownname(name)
	char *name;
{
	struct protox **tpp, *tp;

	for (tpp = protoprotox; *tpp; tpp++)
		for (tp = *tpp; tp->pr_name; tp++)
			if (strcmp(tp->pr_name, name) == 0)
				return (tp);
	return (NULL);
}

/*
 * Find the protox corresponding to name.
 */
static struct protox *
name2protox(name)
	char *name;
{
	struct protox *tp;
	char **alias;			/* alias from p->aliases */
	struct protoent *p;

	/*
	 * Try to find the name in the list of "well-known" names. If that
	 * fails, check if name is an alias for an Internet protocol.
	 */
	if (tp = knownname(name))
		return (tp);

	while (p = getprotoent()) {
		/* assert: name not same as p->name */
		for (alias = p->p_aliases; *alias; alias++)
			if (strcmp(name, *alias) == 0) {
				endprotoent();
				return (knownname(p->p_name));
			}
	}
	endprotoent();
	return (NULL);
}

static void
usage()
{
	(void)fprintf(stderr,
"usage: %s [-Aan] [-f address_family] [-M core] [-N system]\n", prog);
	(void)fprintf(stderr,
"       %s [-bdghimnrs] [-f address_family] [-M core] [-N system]\n", prog);
	(void)fprintf(stderr,
"       %s [-bdn] [-I interface] [-M core] [-N system] [-w wait]\n", prog);
	(void)fprintf(stderr,
"       %s [-M core] [-N system] [-p protocol]\n", prog);
	exit(1);
}
