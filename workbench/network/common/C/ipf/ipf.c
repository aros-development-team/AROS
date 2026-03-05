/*
 * ipf - IP packet filter rule management for AROSTCP
 *
 * Compatible with Roadshow ipf command syntax.
 * Communicates with the kernel ipfilter engine via IoctlSocket() ioctls.
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
 * Parse ICMP type name to number.
 */
static int
parse_icmp_type(const char *name)
{
	static const struct {
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
	};
	int i;
	char *ep;
	long val;

	for (i = 0; icmp_types[i].name; i++) {
		if (strcasecmp(name, icmp_types[i].name) == 0)
			return icmp_types[i].type;
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
 * Forms: "any", "addr/prefix", "addr mask mask"
 * Returns updated token index.
 */
static int
parse_addr_mask(char **tokens, int ntok, int idx,
    struct in_addr *addr, struct in_addr *mask, u_int8_t *not_flag)
{
	char *slash;

	*not_flag = 0;

	if (idx >= ntok)
		return -1;

	if (strcmp(tokens[idx], "!") == 0) {
		*not_flag = 1;
		idx++;
		if (idx >= ntok)
			return -1;
	}

	if (strcmp(tokens[idx], "any") == 0) {
		addr->s_addr = 0;
		mask->s_addr = 0;
		return idx + 1;
	}

	/* Check for addr/prefix */
	slash = strchr(tokens[idx], '/');
	if (slash) {
		*slash = '\0';
		if (resolve_addr(tokens[idx], addr) < 0) {
			fprintf(stderr, "ipf: unknown host \"%s\"\n", tokens[idx]);
			return -1;
		}
		prefix_to_mask(atoi(slash + 1), mask);
		return idx + 1;
	}

	if (resolve_addr(tokens[idx], addr) < 0) {
		fprintf(stderr, "ipf: unknown host \"%s\"\n", tokens[idx]);
		return -1;
	}
	idx++;

	/* Check for "mask" keyword */
	if (idx < ntok && strcmp(tokens[idx], "mask") == 0) {
		idx++;
		if (idx >= ntok)
			return -1;
		if (tokens[idx][0] == '0' && tokens[idx][1] == 'x') {
			mask->s_addr = htonl(strtoul(tokens[idx], NULL, 16));
		} else {
			if (!inet_aton(tokens[idx], mask))
				return -1;
		}
		idx++;
	} else {
		mask->s_addr = 0xffffffff;
	}

	return idx;
}

/*
 * Parse a single filter rule line.
 * Returns 0 on success, -1 on parse error.
 */
static int
parse_rule(char *line, struct ipf_rule *rule, int *insert_pos)
{
	char *tokens[MAX_TOKENS];
	int ntok, idx;

	memset(rule, 0, sizeof(*rule));
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
		idx = parse_addr_mask(tokens, ntok, idx,
		    &rule->src_addr, &rule->src_mask, &rule->src_not);
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

		idx = parse_addr_mask(tokens, ntok, idx,
		    &rule->dst_addr, &rule->dst_mask, &rule->dst_not);
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
			rule->icmp_type = parse_icmp_type(tokens[idx]);
			if (rule->icmp_type < 0) {
				fprintf(stderr, "ipf: unknown icmp type \"%s\"\n",
				    tokens[idx]);
				return -1;
			}
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

	return 0;
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
		int insert_pos;
		int ret;

		lineno++;

		/* Strip trailing newline */
		line[strcspn(line, "\r\n")] = '\0';

		ret = parse_rule(line, &rule, &insert_pos);
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
			printf("\n");
		}

		if (removing) {
			if (ipf_ioctl(SIOCDELFR, &rule) < 0) {
				fprintf(stderr,
				    "ipf: remove rule failed at line %d\n",
				    lineno);
				errors++;
			}
		} else if (insert_pos >= 0) {
			rule.hits = insert_pos;
			if (ipf_ioctl(SIOCINAFR, &rule) < 0) {
				fprintf(stderr,
				    "ipf: insert rule failed at line %d\n",
				    lineno);
				errors++;
			}
		} else {
			if (ipf_ioctl(SIOCADDFR, &rule) < 0) {
				fprintf(stderr,
				    "ipf: add rule failed at line %d\n",
				    lineno);
				errors++;
			}
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
