/*
 * ipnat - NAT rule management for AROSTCP
 *
 * Compatible with Roadshow ipnat command syntax.
 * Communicates with the kernel via IoctlSocket() ioctls.
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

static int verbose = 0;
static int nochange = 0;
static int removing = 0;
static int sock_fd = -1;

static void
usage(void)
{
	fprintf(stderr,
	    "usage: ipnat [-lnrsvCF] -f <filename>\n");
	exit(1);
}

static int
ipnat_ioctl(unsigned long cmd, void *data)
{
	if (nochange)
		return 0;
	return IoctlSocket(sock_fd, cmd, (caddr_t)data);
}

/*
 * Resolve a hostname or dotted-decimal address.
 */
static int
resolve_addr(const char *name, struct in_addr *addr)
{
	struct hostent *hp;

	if (strcmp(name, "any") == 0 || strcmp(name, "0") == 0) {
		addr->s_addr = INADDR_ANY;
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
 * Parse an address/mask: "addr/prefix" or "ifname/prefix"
 * The ifname form uses the interface's address (Roadshow convention).
 */
static int
parse_addr_mask(const char *s, struct in_addr *addr, struct in_addr *mask,
    char *ifname_out, int ifname_size)
{
	char buf[256];
	char *slash;

	strncpy(buf, s, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	slash = strchr(buf, '/');
	if (slash) {
		*slash = '\0';
		prefix_to_mask(atoi(slash + 1), mask);
	} else {
		mask->s_addr = 0xffffffff;
	}

	/* Check if it's an interface name (starts with uppercase) */
	if (isupper((unsigned char)buf[0]) && ifname_out) {
		strncpy(ifname_out, buf, ifname_size - 1);
		ifname_out[ifname_size - 1] = '\0';
		addr->s_addr = 0;
		return 1;	/* interface name */
	}

	if (resolve_addr(buf, addr) < 0) {
		fprintf(stderr, "ipnat: unknown host \"%s\"\n", buf);
		return -1;
	}
	return 0;
}

/*
 * Resolve a port number.
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
 * Parse protocol name for NAT.
 */
static int
parse_nat_proto(const char *name)
{
	if (strcasecmp(name, "tcp/udp") == 0)
		return IPF_PROTO_TCPUDP;
	if (strcasecmp(name, "tcp") == 0)
		return IPPROTO_TCP;
	if (strcasecmp(name, "udp") == 0)
		return IPPROTO_UDP;
	return -1;
}

/*
 * Tokenizer.
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
 * Parse a NAT rule line.
 * Returns 0 on success, 1 for empty/comment, -1 on error.
 */
static int
parse_nat_rule(char *line, struct ipnat_rule *rule)
{
	char *tokens[MAX_TOKENS];
	int ntok, idx;
	char ifname_buf[IPF_MAXIFNAME];

	memset(rule, 0, sizeof(*rule));
	ntok = tokenize(line, tokens, MAX_TOKENS);
	if (ntok == 0)
		return 1;

	idx = 0;

	/* Rule type: map, bimap, map-block, rdr */
	if (strcmp(tokens[idx], "map") == 0) {
		rule->type = IPNAT_MAP;
	} else if (strcmp(tokens[idx], "bimap") == 0) {
		rule->type = IPNAT_BIMAP;
	} else if (strcmp(tokens[idx], "map-block") == 0) {
		rule->type = IPNAT_MAPBLOCK;
	} else if (strcmp(tokens[idx], "rdr") == 0) {
		rule->type = IPNAT_RDR;
	} else {
		fprintf(stderr, "ipnat: unknown rule type \"%s\"\n", tokens[idx]);
		return -1;
	}
	idx++;

	/* Interface name */
	if (idx >= ntok) return -1;
	strncpy(rule->ifname, tokens[idx], sizeof(rule->ifname) - 1);
	idx++;

	if (rule->type == IPNAT_RDR) {
		/* rdr ifname addr/mask port N [-N] -> addr [,addr] port N [proto] [round-robin] */

		/* Match address */
		if (idx >= ntok) return -1;
		if (parse_addr_mask(tokens[idx], &rule->match_addr,
		    &rule->match_mask, NULL, 0) < 0)
			return -1;
		idx++;

		/* "port" portnum ["-" portnum] */
		if (idx < ntok && strcmp(tokens[idx], "port") == 0) {
			idx++;
			if (idx >= ntok) return -1;
			rule->match_port_lo = resolve_port(tokens[idx]);
			if (rule->match_port_lo < 0) return -1;
			idx++;
			if (idx < ntok && strcmp(tokens[idx], "-") == 0) {
				idx++;
				if (idx >= ntok) return -1;
				rule->match_port_hi = resolve_port(tokens[idx]);
				if (rule->match_port_hi < 0) return -1;
				idx++;
			} else {
				rule->match_port_hi = rule->match_port_lo;
			}
		}

		/* "->" */
		if (idx >= ntok || strcmp(tokens[idx], "->") != 0) {
			fprintf(stderr, "ipnat: expected '->'\n");
			return -1;
		}
		idx++;

		/* Target address [,address] */
		if (idx >= ntok) return -1;
		{
			char *comma = strchr(tokens[idx], ',');
			if (comma) {
				*comma = '\0';
				if (resolve_addr(tokens[idx], &rule->nat_addr) < 0)
					return -1;
				if (resolve_addr(comma + 1, &rule->nat_addr2) < 0)
					return -1;
				rule->round_robin = 1;
			} else {
				if (resolve_addr(tokens[idx], &rule->nat_addr) < 0)
					return -1;
			}
		}
		idx++;

		/* "port" portnum */
		if (idx < ntok && strcmp(tokens[idx], "port") == 0) {
			idx++;
			if (idx >= ntok) return -1;
			rule->nat_port_lo = resolve_port(tokens[idx]);
			if (rule->nat_port_lo < 0) return -1;
			rule->nat_port_hi = rule->nat_port_lo;
			idx++;
		}

		/* Optional protocol and round-robin */
		while (idx < ntok) {
			if (strcmp(tokens[idx], "round-robin") == 0) {
				rule->round_robin = 1;
			} else {
				int p = parse_nat_proto(tokens[idx]);
				if (p >= 0)
					rule->nat_proto = p;
			}
			idx++;
		}
	} else {
		/* map/bimap/map-block ifname src/mask -> dst/mask [portmap proto lo:hi] */
		/* Also supports: map ifname from src to any -> dst/mask ... */

		if (idx >= ntok) return -1;

		if (strcmp(tokens[idx], "from") == 0) {
			/* Extended form: from src to dst -> target */
			idx++;
			if (idx >= ntok) return -1;
			if (parse_addr_mask(tokens[idx], &rule->match_addr,
			    &rule->match_mask, NULL, 0) < 0)
				return -1;
			idx++;

			/* "to" ... (skip to ->) */
			if (idx < ntok && strcmp(tokens[idx], "to") == 0) {
				idx++;
				if (idx >= ntok) return -1;
				idx++;	/* skip dest (usually "any") */
			}
		} else {
			/* Simple form: addr/mask */
			if (parse_addr_mask(tokens[idx], &rule->match_addr,
			    &rule->match_mask, NULL, 0) < 0)
				return -1;
			idx++;
		}

		/* "->" */
		if (idx >= ntok || strcmp(tokens[idx], "->") != 0) {
			fprintf(stderr, "ipnat: expected '->'\n");
			return -1;
		}
		idx++;

		/* Target address/mask */
		if (idx >= ntok) return -1;
		memset(ifname_buf, 0, sizeof(ifname_buf));
		{
			int ret = parse_addr_mask(tokens[idx], &rule->nat_addr,
			    &rule->nat_mask, ifname_buf, sizeof(ifname_buf));
			if (ret < 0)
				return -1;
		}
		idx++;

		/* Optional: "proxy port ftp ftp/tcp" — skip proxy rules */
		if (idx < ntok && strcmp(tokens[idx], "proxy") == 0) {
			while (idx < ntok) idx++;
			return 0;
		}

		/* Optional: "portmap tcp/udp lo:hi" */
		if (idx < ntok && strcmp(tokens[idx], "portmap") == 0) {
			idx++;
			if (idx >= ntok) return -1;

			rule->nat_proto = parse_nat_proto(tokens[idx]);
			idx++;

			if (idx < ntok) {
				if (strcmp(tokens[idx], "auto") == 0) {
					rule->nat_port_lo = 1025;
					rule->nat_port_hi = 65000;
					idx++;
				} else {
					char *colon = strchr(tokens[idx], ':');
					if (colon) {
						*colon = '\0';
						rule->nat_port_lo = atoi(tokens[idx]);
						rule->nat_port_hi = atoi(colon + 1);
					} else {
						rule->nat_port_lo = atoi(tokens[idx]);
						rule->nat_port_hi = rule->nat_port_lo;
					}
					idx++;
				}
			}
		}

		/* Optional: "ports auto" (map-block) */
		if (idx < ntok && strcmp(tokens[idx], "ports") == 0) {
			idx++;
			if (idx < ntok) {
				if (strcmp(tokens[idx], "auto") == 0) {
					rule->nat_port_lo = 1025;
					rule->nat_port_hi = 65000;
				} else {
					rule->nat_port_lo = atoi(tokens[idx]);
					rule->nat_port_hi = rule->nat_port_lo;
				}
				idx++;
			}
		}
	}

	return 0;
}

/*
 * Process a NAT rules file.
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
		fprintf(stderr, "ipnat: cannot open \"%s\": %s\n",
		    filename, strerror(errno));
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		struct ipnat_rule rule;
		int ret;

		lineno++;
		line[strcspn(line, "\r\n")] = '\0';

		ret = parse_nat_rule(line, &rule);
		if (ret == 1)
			continue;
		if (ret < 0) {
			fprintf(stderr, "ipnat: parse error at line %d\n", lineno);
			errors++;
			continue;
		}

		if (verbose) {
			const char *type_str;
			switch (rule.type) {
			case IPNAT_MAP: type_str = "map"; break;
			case IPNAT_BIMAP: type_str = "bimap"; break;
			case IPNAT_MAPBLOCK: type_str = "map-block"; break;
			case IPNAT_RDR: type_str = "rdr"; break;
			default: type_str = "?"; break;
			}
			printf("%s %s rule on %s\n",
			    removing ? "Removing" : "Adding",
			    type_str, rule.ifname);
		}

		if (removing) {
			if (ipnat_ioctl(SIOCRMNAT, &rule) < 0) {
				fprintf(stderr,
				    "ipnat: remove rule failed at line %d\n",
				    lineno);
				errors++;
			}
		} else {
			if (ipnat_ioctl(SIOCADNAT, &rule) < 0) {
				fprintf(stderr,
				    "ipnat: add rule failed at line %d\n",
				    lineno);
				errors++;
			}
		}
	}

	if (fp != stdin)
		fclose(fp);

	return errors ? -1 : 0;
}

/*
 * List active NAT rules.
 */
static void
list_rules(void)
{
	struct ipnat_state ns;
	const char *type_str;

	if (ipnat_ioctl(SIOCGNATS, &ns) < 0) {
		fprintf(stderr, "ipnat: cannot get NAT state\n");
		return;
	}

	printf("NAT rules: %u, active entries: %u\n",
	    ns.rule_count, ns.entry_count);
}

/*
 * Show NAT statistics.
 */
static void
show_stats(void)
{
	struct ipnat_state ns;

	if (ipnat_ioctl(SIOCGNATS, &ns) < 0) {
		fprintf(stderr, "ipnat: cannot get NAT statistics\n");
		return;
	}

	printf("NAT Statistics:\n");
	printf("  Rules:   %u\n", ns.rule_count);
	printf("  Entries: %u\n", ns.entry_count);
}

int
main(int argc, char *argv[])
{
	int i;
	int do_clear = 0, do_flush = 0;
	int do_list = 0, do_stats = 0;
	int errors = 0;

	sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (sock_fd < 0) {
		sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock_fd < 0) {
			fprintf(stderr, "ipnat: cannot create socket\n");
			return 20;
		}
	}

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			fprintf(stderr, "ipnat: unexpected argument \"%s\"\n",
			    argv[i]);
			usage();
		}

		/* Handle combined flags like -CF */
		if (strlen(argv[i]) > 2 && argv[i][1] != 'f') {
			char *p;
			for (p = argv[i] + 1; *p; p++) {
				switch (*p) {
				case 'C': do_clear = 1; break;
				case 'F': do_flush = 1; break;
				case 'l': do_list = 1; break;
				case 'n': nochange = 1; break;
				case 'r': removing = 1; break;
				case 's': do_stats = 1; break;
				case 'v': verbose = 1; break;
				default:
					fprintf(stderr,
					    "ipnat: unknown option '-%c'\n", *p);
					break;
				}
			}
			continue;
		}

		switch (argv[i][1]) {
		case 'C':
			do_clear = 1;
			break;
		case 'F':
			do_flush = 1;
			break;
		case 'f':
			i++;
			if (i >= argc) {
				fprintf(stderr, "ipnat: -f requires filename\n");
				usage();
			}
			/* Execute pending clear/flush before loading rules */
			if (do_clear) {
				int n = 0;
				ipnat_ioctl(SIOCFLNAT, &n);
				if (verbose)
					printf("Cleared %d NAT rules\n", n);
				do_clear = 0;
			}
			if (do_flush) {
				int n = 0;
				ipnat_ioctl(SIOCCNATL, &n);
				if (verbose)
					printf("Flushed %d NAT entries\n", n);
				do_flush = 0;
			}
			if (process_file(argv[i]) < 0)
				errors++;
			break;
		case 'l':
			do_list = 1;
			break;
		case 'n':
			nochange = 1;
			break;
		case 'r':
			removing = 1;
			break;
		case 's':
			do_stats = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			fprintf(stderr, "ipnat: unknown option '%s'\n", argv[i]);
			usage();
		}
	}

	/* Execute pending operations */
	if (do_clear) {
		int n = 0;
		ipnat_ioctl(SIOCFLNAT, &n);
		if (verbose)
			printf("Cleared %d NAT rules\n", n);
	}
	if (do_flush) {
		int n = 0;
		ipnat_ioctl(SIOCCNATL, &n);
		if (verbose)
			printf("Flushed %d NAT entries\n", n);
	}
	if (do_list)
		list_rules();
	if (do_stats)
		show_stats();

	CloseSocket(sock_fd);

	return errors ? 5 : 0;
}
