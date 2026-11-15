/*
 * ipf - IP packet filter rule management for AROSTCP
 *
 * Compatible with Roadshow ipf command syntax.
 * Communicates with the kernel ipfilter engine via IoctlSocket() ioctls.
 *
 * IPv6: a rule with an IPv6 address, the protocol icmpv6, or "family
 * inet6" (also just "inet6") is an IPv6 rule (struct ipf_rule6, sent with
 * SIOCADAFR6 and the like); one with an IPv4 address, the protocol icmp,
 * or "family inet" is an IPv4 rule; a rule that names neither ("pass in
 * all", "block in proto tcp from any to any port = 23") is sent as one of
 * each, so that a rule set covers both families. ICMPv6 types by name:
 * unreach, toobig, timex, paramprob, echo, echorep, mlqry, mlrep, mldone,
 * routersol, routerad, neighsol, neighad, redir. Neighbour discovery is
 * ICMPv6 (types 133-137): a rule set that blocks all incoming ICMPv6
 * stops IPv6 on the link.
 *
 * Copyright (C) 2026 The AROS Dev Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <proto/exec.h>
#include <proto/socket.h>
#include <proto/miami.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <net/ipfilter.h>

const TEXT version[] = "$VER: ipf 1.0 (05.03.2026)";

static int verbose = 0;
static int nochange = 0;
static int removing = 0;
static int sock_fd = -1;
static int no_ipv6_rules = 0;	/* the stack took no IPv6 rule: said once */

/* the family of a rule: either (both are sent), IPv4, IPv6 */
#define FAM_ANY		0
#define FAM_INET	4
#define FAM_INET6	6

/* an address and its mask as parsed: either family */
struct addrspec {
	int		fam;		/* FAM_ANY for "any" */
	struct in_addr	a4, m4;
	struct in6_addr	a6;
	int		plen6;
};

static void
usage(void)
{
	fprintf(stderr,
	    "usage: ipf [-AdDEInoPrsvVyzZ] [-l <block|pass|nomatch>]\n"
	    "           [-F <i|o|a|s|S>] -f <filename> [-f <filename> ...]\n");
	exit(1);
}

static int
ipf_ioctl(unsigned long cmd, void *data)
{
	if (nochange)
		return 0;
	return IoctlSocket(sock_fd, cmd, (caddr_t)data);
}

/*
 * Resolve a hostname or dotted-decimal address.
 * Returns 0 on success, -1 on failure.
 */
static int
resolve_addr(const char *name, struct in_addr *addr)
{
	struct hostent *hp;

	if (strcmp(name, "any") == 0 || strcmp(name, "0") == 0) {
		addr->s_addr = INADDR_ANY;
		return 0;
	}
	if (strcmp(name, "localhost") == 0) {
		addr->s_addr = htonl(INADDR_LOOPBACK);
		return 0;
	}
	if (inet_aton(name, addr))
		return 0;

	hp = gethostbyname(name);
	if (hp && hp->h_addrtype == AF_INET) {
		memcpy(addr, hp->h_addr, sizeof(*addr));
		return 0;
	}
	return -1;
}

/*
 * Generate a netmask from a prefix length.
 */
static void
prefix_to_mask(int prefix, struct in_addr *mask)
{
	if (prefix == 0)
		mask->s_addr = 0;
	else
		mask->s_addr = htonl(~((1U << (32 - prefix)) - 1));
}

/*
 * Resolve a port number from service name or decimal.
 */
static int
resolve_port(const char *name)
{
	struct servent *sp;
	char *ep;
	long val;

	val = strtol(name, &ep, 10);
	if (*ep == '\0' && val >= 0 && val <= 65535)
		return (int)val;

	sp = getservbyname(name, "tcp");
	if (sp == NULL)
		sp = getservbyname(name, "udp");
	if (sp)
		return ntohs(sp->s_port);

	return -1;
}

/*
 * Parse a protocol name/number.
 */
static int
parse_proto(const char *name)
{
	if (strcasecmp(name, "tcp/udp") == 0)
		return IPF_PROTO_TCPUDP;
	if (strcasecmp(name, "tcp") == 0)
		return IPPROTO_TCP;
	if (strcasecmp(name, "udp") == 0)
		return IPPROTO_UDP;
	if (strcasecmp(name, "icmp") == 0)
		return IPPROTO_ICMP;
	if (strcasecmp(name, "icmpv6") == 0 || strcasecmp(name, "icmp6") == 0 ||
	    strcasecmp(name, "ipv6-icmp") == 0)
		return IPPROTO_ICMPV6;
	{
		char *ep;
		long val = strtol(name, &ep, 10);
		if (*ep == '\0' && val >= 0 && val <= 255)
			return (int)val;
	}
	return -1;
}

/*
 * Parse a port comparison operator.
 */
static int
parse_cmp(const char *s)
{
	if (strcmp(s, "=") == 0 || strcmp(s, "eq") == 0)
		return IPF_PORT_EQ;
	if (strcmp(s, "!=") == 0 || strcmp(s, "ne") == 0)
		return IPF_PORT_NE;
	if (strcmp(s, "<") == 0 || strcmp(s, "lt") == 0)
		return IPF_PORT_LT;
	if (strcmp(s, ">") == 0 || strcmp(s, "gt") == 0)
		return IPF_PORT_GT;
	if (strcmp(s, "<=") == 0 || strcmp(s, "le") == 0)
		return IPF_PORT_LE;
	if (strcmp(s, ">=") == 0 || strcmp(s, "ge") == 0)
		return IPF_PORT_GE;
	return -1;
}

/*
 * Parse a port range operator.
 */
static int
parse_range_op(const char *s)
{
	if (strcmp(s, "<>") == 0)
		return IPF_PORT_RANGE;
	if (strcmp(s, "><") == 0)
		return IPF_PORT_RANGEINCL;
	return -1;
}

/*
 * Parse TCP flags string like "S", "SA", "S/SA".
 */
static int
parse_flags(const char *s, u_int8_t *flags, u_int8_t *mask)
{
	const char *slash;
	const char *p;

	*flags = 0;
	*mask = 0;

	slash = strchr(s, '/');
	p = s;
	while (p < (slash ? slash : s + strlen(s))) {
		switch (toupper((unsigned char)*p)) {
		case 'F': *flags |= 0x01; break;	/* TH_FIN */
		case 'S': *flags |= 0x02; break;	/* TH_SYN */
		case 'R': *flags |= 0x04; break;	/* TH_RST */
		case 'P': *flags |= 0x08; break;	/* TH_PUSH */
		case 'A': *flags |= 0x10; break;	/* TH_ACK */
		case 'U': *flags |= 0x20; break;	/* TH_URG */
		default:
			return -1;
		}
		p++;
	}

	if (slash) {
		p = slash + 1;
		while (*p) {
			switch (toupper((unsigned char)*p)) {
			case 'F': *mask |= 0x01; break;
			case 'S': *mask |= 0x02; break;
			case 'R': *mask |= 0x04; break;
			case 'P': *mask |= 0x08; break;
			case 'A': *mask |= 0x10; break;
			case 'U': *mask |= 0x20; break;
			default:
				return -1;
			}
			p++;
		}
	} else {
		*mask = *flags;
	}

	return 0;
}

/*
 * Parse ICMP type name to number: ICMP's names for an IPv4 rule, ICMPv6's
 * for an IPv6 one.
 */
static int
parse_icmp_type(const char *name, int fam)
{
	static const struct icmpname {
		const char *name;
		int type;
	} icmp_types[] = {
		{ "echorep",	0 },
		{ "unreach",	3 },
		{ "squench",	4 },
		{ "redir",	5 },
		{ "echo",	8 },
		{ "routerad",	9 },
		{ "routersol",	10 },
		{ "timex",	11 },
		{ "paramprob",	12 },
		{ "timest",	13 },
		{ "timestrep",	14 },
		{ "inforeq",	15 },
		{ "inforep",	16 },
		{ "maskreq",	17 },
		{ "maskrep",	18 },
		{ NULL, 0 }
	}, icmp6_types[] = {
		{ "unreach",	1 },
		{ "toobig",	2 },
		{ "timex",	3 },
		{ "paramprob",	4 },
		{ "echo",	128 },
		{ "echorep",	129 },
		{ "mlqry",	130 },
		{ "mlrep",	131 },
		{ "mldone",	132 },
		{ "routersol",	133 },
		{ "routerad",	134 },
		{ "neighsol",	135 },
		{ "neighad",	136 },
		{ "redir",	137 },
		{ NULL, 0 }
	};
	const struct icmpname *t = fam == FAM_INET6 ? icmp6_types : icmp_types;
	int i;
	char *ep;
	long val;

	for (i = 0; t[i].name; i++) {
		if (strcasecmp(name, t[i].name) == 0)
			return t[i].type;
	}
	val = strtol(name, &ep, 10);
	if (*ep == '\0' && val >= 0 && val <= 255)
		return (int)val;
	return -1;
}

/*
 * Tokenizer: split a line into tokens.
 */
#define MAX_TOKENS 64

static int
tokenize(char *line, char **tokens, int maxtok)
{
	int n = 0;
	char *p = line;

	while (*p && n < maxtok) {
		while (*p && isspace((unsigned char)*p))
			p++;
		if (*p == '\0' || *p == '#')
			break;
		tokens[n++] = p;
		while (*p && !isspace((unsigned char)*p))
			p++;
		if (*p)
			*p++ = '\0';
	}
	return n;
}

/*
 * Parse an address/mask specification.
 * Forms: "any", "addr/prefix", "addr mask mask" (IPv4), "addr6/prefix",
 * "addr6" (IPv6, prefix 128). `fam' is the rule's family if it was given
 * ("localhost" is ::1 in an IPv6 rule).
 * Returns updated token index.
 */
static int
parse_addr_mask(char **tokens, int ntok, int idx, struct addrspec *as,
    u_int8_t *not_flag, int fam)
{
	char *slash;

	*not_flag = 0;
	memset(as, 0, sizeof(*as));

	if (idx >= ntok)
		return -1;

	if (strcmp(tokens[idx], "!") == 0) {
		*not_flag = 1;
		idx++;
		if (idx >= ntok)
			return -1;
	}

	if (strcmp(tokens[idx], "any") == 0)
		return idx + 1;

	slash = strchr(tokens[idx], '/');
	if (slash)
		*slash = '\0';

	/* IPv6: a literal (it has a colon), or localhost in an IPv6 rule */
	if (strchr(tokens[idx], ':') != NULL ||
	    (fam == FAM_INET6 && strcmp(tokens[idx], "localhost") == 0)) {
		as->fam = FAM_INET6;
		if (strcmp(tokens[idx], "localhost") == 0) {
			static const struct in6_addr lo6 = IN6ADDR_LOOPBACK_INIT;

			as->a6 = lo6;
		}
		else if (inet_pton(AF_INET6, tokens[idx], &as->a6) != 1) {
			fprintf(stderr, "ipf: bad IPv6 address \"%s\"\n", tokens[idx]);
			return -1;
		}
		as->plen6 = 128;
		if (slash) {
			char *ep;
			long plen = strtol(slash + 1, &ep, 10);

			if (*ep != '\0' || plen < 0 || plen > 128) {
				fprintf(stderr, "ipf: bad IPv6 prefix length \"%s\"\n", slash + 1);
				return -1;
			}
			as->plen6 = (int)plen;
		}
		return idx + 1;
	}

	as->fam = FAM_INET;
	if (resolve_addr(tokens[idx], &as->a4) < 0) {
		fprintf(stderr, "ipf: unknown host \"%s\"\n", tokens[idx]);
		return -1;
	}
	if (slash) {
		prefix_to_mask(atoi(slash + 1), &as->m4);
		return idx + 1;
	}
	idx++;

	/* Check for "mask" keyword */
	if (idx < ntok && strcmp(tokens[idx], "mask") == 0) {
		idx++;
		if (idx >= ntok)
			return -1;
		if (tokens[idx][0] == '0' && tokens[idx][1] == 'x') {
			as->m4.s_addr = htonl(strtoul(tokens[idx], NULL, 16));
		} else {
			if (!inet_aton(tokens[idx], &as->m4))
				return -1;
		}
		idx++;
	} else {
		as->m4.s_addr = 0xffffffff;
	}

	return idx;
}

/*
 * The family of the rule from what it names, against a family given:
 * FAM_ANY, FAM_INET, FAM_INET6, or -1 when they contradict each other.
 */
static int
rule_family(int fam, int proto, const struct addrspec *src, const struct addrspec *dst)
{
	int want[3], i;

	want[0] = src->fam;
	want[1] = dst->fam;
	want[2] = proto == IPPROTO_ICMP ? FAM_INET :
	    proto == IPPROTO_ICMPV6 ? FAM_INET6 : FAM_ANY;
	for (i = 0; i < 3; i++) {
		if (want[i] == FAM_ANY)
			continue;
		if (fam != FAM_ANY && fam != want[i])
			return -1;
		fam = want[i];
	}
	return fam;
}

/*
 * Parse a single filter rule line.
 * Returns 0 on success, -1 on parse error.
 */
static int
parse_rule(char *line, struct ipf_rule *rule, struct ipf_rule6 *rule6,
    int *famp, int *insert_pos)
{
	char *tokens[MAX_TOKENS];
	int ntok, idx;
	int fam = FAM_ANY;
	const char *icmp_name = NULL;
	struct addrspec src, dst;

	memset(rule, 0, sizeof(*rule));
	memset(rule6, 0, sizeof(*rule6));
	memset(&src, 0, sizeof(src));
	memset(&dst, 0, sizeof(dst));
	rule->icmp_type = -1;
	rule->icmp_code = -1;
	*insert_pos = -1;

	ntok = tokenize(line, tokens, MAX_TOKENS);
	if (ntok == 0)
		return 1;	/* empty/comment line */

	idx = 0;

	/* Optional insert position: @N */
	if (tokens[idx][0] == '@') {
		*insert_pos = atoi(tokens[idx] + 1);
		idx++;
	}

	if (idx >= ntok)
		return -1;

	/* Action */
	if (strcmp(tokens[idx], "block") == 0) {
		rule->action = IPF_BLOCK;
		idx++;
		/* Optional return-rst / return-icmp */
		if (idx < ntok && strcmp(tokens[idx], "return-rst") == 0) {
			rule->rflags |= IPF_RETURN_RST;
			idx++;
		} else if (idx < ntok && strncmp(tokens[idx], "return-icmp", 11) == 0) {
			rule->rflags |= IPF_RETURN_ICMP;
			idx++;
		}
	} else if (strcmp(tokens[idx], "pass") == 0) {
		rule->action = IPF_PASS;
		idx++;
	} else if (strcmp(tokens[idx], "log") == 0) {
		rule->action = IPF_LOG;
		idx++;
	} else if (strcmp(tokens[idx], "count") == 0) {
		rule->action = IPF_COUNT;
		idx++;
	} else {
		fprintf(stderr, "ipf: unknown action \"%s\"\n", tokens[idx]);
		return -1;
	}

	/* Direction: in/out */
	if (idx >= ntok)
		return -1;
	if (strcmp(tokens[idx], "in") == 0) {
		rule->dir = IPF_IN;
		idx++;
	} else if (strcmp(tokens[idx], "out") == 0) {
		rule->dir = IPF_OUT;
		idx++;
	} else {
		fprintf(stderr, "ipf: expected 'in' or 'out', got \"%s\"\n",
		    tokens[idx]);
		return -1;
	}

	/* Options: [log] [quick] [on ifname] */
	while (idx < ntok) {
		if (strcmp(tokens[idx], "log") == 0) {
			rule->rflags |= IPF_LOG_BODY;
			idx++;
			/* optional: body, first, or-block */
			while (idx < ntok) {
				if (strcmp(tokens[idx], "body") == 0) {
					rule->rflags |= IPF_LOG_BODY;
					idx++;
				} else if (strcmp(tokens[idx], "first") == 0) {
					rule->rflags |= IPF_LOG_FIRST;
					idx++;
				} else if (strcmp(tokens[idx], "or-block") == 0) {
					rule->rflags |= IPF_LOG_ORBLOCK;
					idx++;
				} else if (strcmp(tokens[idx], "level") == 0) {
					idx += 2;	/* skip level value */
				} else {
					break;
				}
			}
		} else if (strcmp(tokens[idx], "quick") == 0) {
			rule->rflags |= IPF_QUICK;
			idx++;
		} else if (strcmp(tokens[idx], "inet") == 0 ||
			   strcmp(tokens[idx], "inet6") == 0 ||
			   strcmp(tokens[idx], "family") == 0) {
			if (strcmp(tokens[idx], "family") == 0) {
				idx++;
				if (idx >= ntok)
					return -1;
			}
			if (strcmp(tokens[idx], "inet6") == 0)
				fam = FAM_INET6;
			else if (strcmp(tokens[idx], "inet") == 0)
				fam = FAM_INET;
			else {
				fprintf(stderr, "ipf: unknown family \"%s\"\n",
				    tokens[idx]);
				return -1;
			}
			idx++;
		} else if (strcmp(tokens[idx], "on") == 0) {
			idx++;
			if (idx >= ntok)
				return -1;
			strncpy(rule->ifname, tokens[idx],
			    sizeof(rule->ifname) - 1);
			idx++;
		} else {
			break;
		}
	}

	/* Proto */
	if (idx < ntok && strcmp(tokens[idx], "proto") == 0) {
		idx++;
		if (idx >= ntok)
			return -1;
		rule->proto = parse_proto(tokens[idx]);
		if (rule->proto < 0) {
			fprintf(stderr, "ipf: unknown protocol \"%s\"\n",
			    tokens[idx]);
			return -1;
		}
		idx++;
	}

	/* Source/destination: "all" or "from X to Y" */
	if (idx < ntok && strcmp(tokens[idx], "all") == 0) {
		idx++;
	} else if (idx < ntok && strcmp(tokens[idx], "from") == 0) {
		idx++;
		idx = parse_addr_mask(tokens, ntok, idx, &src, &rule->src_not, fam);
		if (idx < 0)
			return -1;

		/* Source port */
		if (idx < ntok && strcmp(tokens[idx], "port") == 0) {
			idx++;
			if (idx >= ntok)
				return -1;
			/* Check for range operator */
			if (idx + 2 < ntok) {
				int rop = parse_range_op(tokens[idx + 1]);
				if (rop >= 0) {
					rule->sport_op = rop;
					rule->sport_lo = resolve_port(tokens[idx]);
					rule->sport_hi = resolve_port(tokens[idx + 2]);
					if (rule->sport_lo < 0 || rule->sport_hi < 0)
						return -1;
					idx += 3;
					goto src_port_done;
				}
			}
			/* Comparison operator */
			{
				int cop = parse_cmp(tokens[idx]);
				if (cop >= 0) {
					rule->sport_op = cop;
					idx++;
					if (idx >= ntok)
						return -1;
					rule->sport_lo = resolve_port(tokens[idx]);
					if (rule->sport_lo < 0)
						return -1;
					idx++;
				}
			}
		}
src_port_done:

		if (idx >= ntok || strcmp(tokens[idx], "to") != 0) {
			fprintf(stderr, "ipf: expected 'to'\n");
			return -1;
		}
		idx++;

		idx = parse_addr_mask(tokens, ntok, idx, &dst, &rule->dst_not, fam);
		if (idx < 0)
			return -1;

		/* Destination port */
		if (idx < ntok && strcmp(tokens[idx], "port") == 0) {
			idx++;
			if (idx >= ntok)
				return -1;
			/* Check for range operator */
			if (idx + 2 < ntok) {
				int rop = parse_range_op(tokens[idx + 1]);
				if (rop >= 0) {
					rule->dport_op = rop;
					rule->dport_lo = resolve_port(tokens[idx]);
					rule->dport_hi = resolve_port(tokens[idx + 2]);
					if (rule->dport_lo < 0 || rule->dport_hi < 0)
						return -1;
					idx += 3;
					goto dst_port_done;
				}
			}
			/* Comparison operator */
			{
				int cop = parse_cmp(tokens[idx]);
				if (cop >= 0) {
					rule->dport_op = cop;
					idx++;
					if (idx >= ntok)
						return -1;
					rule->dport_lo = resolve_port(tokens[idx]);
					if (rule->dport_lo < 0)
						return -1;
					idx++;
				}
			}
		}
dst_port_done:
		;
	}

	/* Remaining options: flags, with, icmp-type, keep */
	while (idx < ntok) {
		if (strcmp(tokens[idx], "flags") == 0) {
			idx++;
			if (idx >= ntok)
				return -1;
			if (parse_flags(tokens[idx],
			    &rule->tcp_flags, &rule->tcp_flagmask) < 0) {
				fprintf(stderr, "ipf: bad flags \"%s\"\n",
				    tokens[idx]);
				return -1;
			}
			idx++;
		} else if (strcmp(tokens[idx], "with") == 0 ||
			   strcmp(tokens[idx], "and") == 0) {
			idx++;
			while (idx < ntok) {
				if (strcmp(tokens[idx], "not") == 0 ||
				    strcmp(tokens[idx], "no") == 0) {
					idx++;
					continue;
				}
				if (strcmp(tokens[idx], "short") == 0) {
					rule->rflags |= IPF_SHORT;
					idx++;
				} else if (strcmp(tokens[idx], "ipopts") == 0) {
					rule->rflags |= IPF_IPOPTS;
					idx++;
				} else if (strcmp(tokens[idx], "frag") == 0) {
					rule->rflags |= IPF_FRAG;
					idx++;
				} else {
					break;
				}
			}
		} else if (strcmp(tokens[idx], "icmp-type") == 0) {
			idx++;
			if (idx >= ntok)
				return -1;
			/* by name once the family is known: below */
			icmp_name = tokens[idx];
			idx++;
			/* Optional "code N" */
			if (idx + 1 < ntok && strcmp(tokens[idx], "code") == 0) {
				idx++;
				rule->icmp_code = atoi(tokens[idx]);
				idx++;
			}
		} else if (strcmp(tokens[idx], "keep") == 0) {
			idx++;
			if (idx >= ntok)
				return -1;
			if (strcmp(tokens[idx], "state") == 0)
				rule->rflags |= IPF_KEEP_STATE;
			else if (strcmp(tokens[idx], "frags") == 0)
				rule->rflags |= IPF_KEEP_FRAGS;
			idx++;
		} else {
			/* Ignore unrecognised trailing tokens */
			idx++;
		}
	}

	/* the family: given, or from the addresses and the protocol */
	if ((fam = rule_family(fam, rule->proto, &src, &dst)) < 0) {
		fprintf(stderr, "ipf: the rule mixes IPv4 and IPv6\n");
		return -1;
	}
	if (icmp_name != NULL) {
		/* an ICMP type without a family is ICMP's (IPv4), as before */
		if (fam == FAM_ANY)
			fam = FAM_INET;
		rule->icmp_type = parse_icmp_type(icmp_name, fam);
		if (rule->icmp_type < 0) {
			fprintf(stderr, "ipf: unknown icmp type \"%s\"\n",
			    icmp_name);
			return -1;
		}
	}
	*famp = fam;

	rule->src_addr = src.a4;
	rule->src_mask = src.m4;
	rule->dst_addr = dst.a4;
	rule->dst_mask = dst.m4;

	/* the same rule for IPv6 */
	rule6->size = sizeof(*rule6);
	rule6->action = rule->action;
	rule6->dir = rule->dir;
	rule6->rflags = rule->rflags;
	memcpy(rule6->ifname, rule->ifname, sizeof(rule6->ifname));
	rule6->proto = rule->proto;
	rule6->src_addr = src.a6;
	rule6->src_plen = src.plen6;
	rule6->src_not = rule->src_not;
	rule6->dst_addr = dst.a6;
	rule6->dst_plen = dst.plen6;
	rule6->dst_not = rule->dst_not;
	rule6->sport_op = rule->sport_op;
	rule6->sport_lo = rule->sport_lo;
	rule6->sport_hi = rule->sport_hi;
	rule6->dport_op = rule->dport_op;
	rule6->dport_lo = rule->dport_lo;
	rule6->dport_hi = rule->dport_hi;
	rule6->tcp_flags = rule->tcp_flags;
	rule6->tcp_flagmask = rule->tcp_flagmask;
	rule6->icmp_type = rule->icmp_type;
	rule6->icmp_code = rule->icmp_code;

	return 0;
}

/*
 * Send a rule of one family: added, inserted at insert_pos (>= 0), or
 * removed. 0, or -1 when the stack refused it.
 */
static int
send_rule(int fam, struct ipf_rule *rule, struct ipf_rule6 *rule6, int insert_pos)
{
	if (fam == FAM_INET6) {
		if (removing)
			return ipf_ioctl(SIOCRMAFR6, rule6);
		if (insert_pos >= 0) {
			rule6->hits = insert_pos;
			return ipf_ioctl(SIOCINAFR6, rule6);
		}
		return ipf_ioctl(SIOCADAFR6, rule6);
	}
	if (removing)
		return ipf_ioctl(SIOCDELFR, rule);
	if (insert_pos >= 0) {
		rule->hits = insert_pos;
		return ipf_ioctl(SIOCINAFR, rule);
	}
	return ipf_ioctl(SIOCADDFR, rule);
}

/*
 * Process a rule file.
 */
static int
process_file(const char *filename)
{
	FILE *fp;
	char line[1024];
	int lineno = 0;
	int errors = 0;

	if (strcmp(filename, "-") == 0)
		fp = stdin;
	else
		fp = fopen(filename, "r");

	if (fp == NULL) {
		fprintf(stderr, "ipf: cannot open \"%s\": %s\n",
		    filename, strerror(errno));
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		struct ipf_rule rule;
		struct ipf_rule6 rule6;
		int insert_pos, fam;
		int ret, failed = 0;

		lineno++;

		/* Strip trailing newline */
		line[strcspn(line, "\r\n")] = '\0';

		ret = parse_rule(line, &rule, &rule6, &fam, &insert_pos);
		if (ret == 1)
			continue;	/* empty/comment */
		if (ret < 0) {
			fprintf(stderr, "ipf: parse error at line %d\n", lineno);
			errors++;
			continue;
		}

		if (verbose) {
			printf("%s rule: %s %s",
			    removing ? "Removing" : "Adding",
			    rule.action == IPF_PASS ? "pass" :
			    rule.action == IPF_BLOCK ? "block" :
			    rule.action == IPF_LOG ? "log" : "count",
			    rule.dir == IPF_IN ? "in" : "out");
			if (rule.ifname[0])
				printf(" on %s", rule.ifname);
			printf(" %s\n", fam == FAM_INET ? "(IPv4)" :
			    fam == FAM_INET6 ? "(IPv6)" : "(IPv4 and IPv6)");
		}

		if (fam != FAM_INET6 && send_rule(FAM_INET, &rule, &rule6, insert_pos) < 0)
			failed = 1;
		if (fam != FAM_INET && send_rule(FAM_INET6, &rule, &rule6, insert_pos) < 0) {
			/*
			 * A rule of either family on a stack that takes no
			 * IPv6 rules (built before them): IPv4 alone, said
			 * once. An IPv6 rule there is an error.
			 */
			if (fam == FAM_ANY && Errno() != ESRCH) {
				if (!no_ipv6_rules)
					fprintf(stderr, "ipf: this stack takes no IPv6 rules: "
					    "IPv6 is not filtered\n");
				no_ipv6_rules = 1;
			} else
				failed = 1;
		}
		if (failed) {
			fprintf(stderr, "ipf: %s rule failed at line %d\n",
			    removing ? "remove" : insert_pos >= 0 ? "insert" : "add",
			    lineno);
			errors++;
		}
	}

	if (fp != stdin)
		fclose(fp);

	return errors ? -1 : 0;
}

int
main(int argc, char *argv[])
{
	int i;
	int enable = 0, disable = 0;
	int flush_flags = 0;
	int do_swap = 0;
	int do_zero = 0;
	int do_version = 0;
	int errors = 0;

	sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (sock_fd < 0) {
		/* Try DGRAM as fallback */
		sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
		/* a stack with IPv6 alone: the requests take any socket */
		if (sock_fd < 0)
			sock_fd = socket(AF_INET6, SOCK_DGRAM, 0);
		if (sock_fd < 0) {
			fprintf(stderr, "ipf: cannot create socket\n");
			return 20;
		}
	}

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			fprintf(stderr, "ipf: unexpected argument \"%s\"\n",
			    argv[i]);
			usage();
		}

		/* Handle combined flags like -DFa */
		if (argv[i][1] != 'f' && argv[i][1] != 'l' &&
		    argv[i][1] != 'F' && strlen(argv[i]) > 2) {
			/* Expand combined flags */
			char *p;
			for (p = argv[i] + 1; *p; p++) {
				switch (*p) {
				case 'D': disable = 1; break;
				case 'E': enable = 1; break;
				case 'd': break;	/* debug, ignored */
				case 'n': nochange = 1; break;
				case 'r': removing = 1; break;
				case 's': do_swap = 1; break;
				case 'v': verbose = 1; break;
				case 'V': do_version = 1; break;
				case 'z': break;	/* zero per-rule stats */
				case 'Z': do_zero = 1; break;
				case 'A': break;	/* active list (default) */
				case 'I': break;	/* inactive list */
				case 'o': break;	/* output list default */
				case 'P': break;	/* preauth */
				case 'y': break;	/* resync */
				case 'F':
					p++;
					switch (*p) {
					case 'i': flush_flags = IPF_FLUSH_IN; break;
					case 'o': flush_flags = IPF_FLUSH_OUT; break;
					case 'a': flush_flags = IPF_FLUSH_ALL; break;
					default:
						fprintf(stderr,
						    "ipf: unknown flush flag '%c'\n", *p);
						break;
					}
					break;
				default:
					fprintf(stderr, "ipf: unknown option '-%c'\n", *p);
					break;
				}
			}
			continue;
		}

		switch (argv[i][1]) {
		case 'D':
			disable = 1;
			break;
		case 'E':
			enable = 1;
			break;
		case 'd':
			break;
		case 'f':
			i++;
			if (i >= argc) {
				fprintf(stderr, "ipf: -f requires filename\n");
				usage();
			}
			/* Process the file - but first do any pending
			 * disable/flush/enable operations */
			if (disable) {
				u_int val = 0;
				ipf_ioctl(SIOCFRENB, &val);
				disable = 0;
			}
			if (flush_flags) {
				int ff = flush_flags;
				int flushed = 0;
				ipf_ioctl(SIOCIPFFL, &ff);
				flushed = ff;
				if (verbose)
					printf("Flushed %d rules\n", flushed);
				flush_flags = 0;
			}
			if (process_file(argv[i]) < 0)
				errors++;
			if (enable) {
				u_int val = 1;
				ipf_ioctl(SIOCFRENB, &val);
				enable = 0;
			}
			break;
		case 'F':
			i++;
			if (i >= argc) {
				fprintf(stderr, "ipf: -F requires argument\n");
				usage();
			}
			switch (argv[i][0]) {
			case 'i': flush_flags = IPF_FLUSH_IN; break;
			case 'o': flush_flags = IPF_FLUSH_OUT; break;
			case 'a': flush_flags = IPF_FLUSH_ALL; break;
			case 's': case 'S':
				/* State flush not yet implemented */
				break;
			default:
				fprintf(stderr, "ipf: unknown flush type '%s'\n",
				    argv[i]);
				break;
			}
			break;
		case 'l':
			i++;
			if (i >= argc) {
				fprintf(stderr, "ipf: -l requires argument\n");
				usage();
			}
			/* Log control - set via SIOCSETFF */
			break;
		case 'n':
			nochange = 1;
			break;
		case 'r':
			removing = 1;
			break;
		case 's':
			do_swap = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'V':
			do_version = 1;
			break;
		case 'z':
			break;
		case 'Z':
			do_zero = 1;
			break;
		case 'A':
		case 'I':
		case 'o':
		case 'P':
		case 'y':
			break;
		default:
			fprintf(stderr, "ipf: unknown option '%s'\n", argv[i]);
			usage();
		}
	}

	/* Execute pending operations */
	if (disable) {
		u_int val = 0;
		ipf_ioctl(SIOCFRENB, &val);
	}

	if (flush_flags) {
		int ff = flush_flags;
		ipf_ioctl(SIOCIPFFL, &ff);
		if (verbose)
			printf("Flushed %d rules\n", ff);
	}

	if (enable) {
		u_int val = 1;
		ipf_ioctl(SIOCFRENB, &val);
	}

	if (do_swap) {
		u_int val;
		ipf_ioctl(SIOCSWAPA, &val);
		if (verbose)
			printf("Swapped to set %u\n", val);
	}

	if (do_zero) {
		struct ipf_state st;
		ipf_ioctl(SIOCFRZST, &st);
		if (verbose)
			printf("Statistics zeroed (was: %lu passed, %lu blocked)\n",
			    (unsigned long)st.passed, (unsigned long)st.blocked);
	}

	if (do_version) {
		struct ipf_state st;
		printf("ipf: AROSTCP IP Filter v1.0\n");
		if (!nochange && ipf_ioctl(SIOCGETFS, &st) == 0) {
			printf("Running: %s\n",
			    st.enabled ? "yes" : "no");
			printf("Rules:   %u\n", st.rule_count);
			printf("Passed:  %lu\n", (unsigned long)st.passed);
			printf("Blocked: %lu\n", (unsigned long)st.blocked);
			printf("Logged:  %lu\n", (unsigned long)st.logged);
		}
	}

	CloseSocket(sock_fd);

	return errors ? 5 : 0;
}
