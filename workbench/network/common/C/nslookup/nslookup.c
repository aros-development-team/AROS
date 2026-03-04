/*
 * nslookup - DNS lookup utility for AROS
 *
 * Copyright (C) 2026 The AROS Dev Team
 *
 * Usage: nslookup <hostname|address> [TYPE A|AAAA|PTR|ANY]
 *
 * Performs forward (name->address) and reverse (address->name) DNS lookups.
 * Supports IPv4 and IPv6 addresses.
 */

static const char version[] __attribute__((used)) =
    "$VER: nslookup 1.0 (04.03.2026)\n"
    "Copyright (C) 2026 The AROS Dev Team";

#define USE_INLINE_STDARG

#include <proto/socket.h>
#include <proto/miami.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <netdb.h>

#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

enum {
    QTYPE_A    = 1,
    QTYPE_AAAA = 2,
    QTYPE_PTR  = 4,
    QTYPE_ANY  = (1|2|4)
};

/* Read the nameserver address from AROSTCP:db/netdb */
static int get_nameserver(char *buf, int buflen)
{
    BPTR fh;
    char line[256];
    int found = 0;

    fh = Open("AROSTCP:db/netdb", MODE_OLDFILE);
    if (!fh)
        fh = Open("ENV:AROSTCP/netdb", MODE_OLDFILE);
    if (!fh)
        return 0;

    while (FGets(fh, line, sizeof(line))) {
        /* Skip comments and blank lines */
        if (line[0] == '#' || line[0] == ';' || line[0] == '\n')
            continue;

        /* Look for NAMESERVER lines */
        if (strncasecmp(line, "NAMESERVER", 10) == 0) {
            char *p = line + 10;
            while (*p == ' ' || *p == '\t')
                p++;
            /* Strip trailing whitespace */
            {
                int len = strlen(p);
                while (len > 0 && (p[len-1] == '\n' || p[len-1] == '\r' ||
                       p[len-1] == ' ' || p[len-1] == '\t'))
                    p[--len] = '\0';
            }
            if (*p) {
                strncpy(buf, p, buflen - 1);
                buf[buflen - 1] = '\0';
                found = 1;
                break;
            }
        }
    }
    Close(fh);
    return found;
}

/* Reverse lookup: address -> hostname */
static int do_reverse_lookup(const char *addrstr, int af,
                             const void *addr, int addrlen)
{
    struct hostent *hp;

    hp = gethostbyaddr((caddr_t)addr, addrlen, af);
    if (hp) {
        char **alias;

        Printf("%s\tname = %s\n", (CONST_STRPTR)addrstr, (CONST_STRPTR)hp->h_name);

        alias = hp->h_aliases;
        while (alias && *alias) {
            Printf("\talias = %s\n", (CONST_STRPTR)*alias);
            alias++;
        }
        return 1;
    }

    Printf("** server can't find %s: ", (CONST_STRPTR)addrstr);
    switch (h_errno) {
    case HOST_NOT_FOUND:
        Printf("NXDOMAIN\n");
        break;
    case TRY_AGAIN:
        Printf("SERVFAIL\n");
        break;
    case NO_RECOVERY:
        Printf("REFUSED\n");
        break;
    case NO_DATA:
        Printf("NODATA\n");
        break;
    default:
        Printf("UNKNOWN\n");
        break;
    }
    return 0;
}

/* Forward lookup: hostname -> addresses */
static int do_forward_lookup(const char *hostname, int qtypes)
{
    int found = 0;

    /* IPv4 (A record) lookup */
    if (qtypes & QTYPE_A) {
        struct hostent *hp = gethostbyname(hostname);
        if (hp && hp->h_addrtype == AF_INET) {
            char **ap = hp->h_addr_list;
            Printf("Name:\t%s\n", (CONST_STRPTR)hp->h_name);
            while (ap && *ap) {
                struct in_addr *inaddr = (struct in_addr *)*ap;
                Printf("Address: %s\n", (CONST_STRPTR)Inet_NtoA(inaddr->s_addr));
                ap++;
                found++;
            }
        }
    }

    /* IPv6 (AAAA record) lookup */
    if (qtypes & QTYPE_AAAA) {
        struct hostent *hp6 = gethostbyname2(hostname, AF_INET6);
        if (hp6 && hp6->h_addrtype == AF_INET6) {
            char **ap = hp6->h_addr_list;
            char buf[INET6_ADDRSTRLEN];
            if (!found)
                Printf("Name:\t%s\n", (CONST_STRPTR)hp6->h_name);
            while (ap && *ap) {
                if (inet_ntop(AF_INET6, *ap, buf, sizeof(buf)))
                    Printf("Address: %s\n", (CONST_STRPTR)buf);
                ap++;
                found++;
            }
        }
    }

    if (!found) {
        Printf("** server can't find %s: ", (CONST_STRPTR)hostname);
        switch (h_errno) {
        case HOST_NOT_FOUND:
            Printf("NXDOMAIN\n");
            break;
        case TRY_AGAIN:
            Printf("SERVFAIL\n");
            break;
        case NO_RECOVERY:
            Printf("REFUSED\n");
            break;
        case NO_DATA:
            Printf("NODATA\n");
            break;
        default:
            Printf("UNKNOWN\n");
            break;
        }
    }

    return found;
}

int main(void)
{
    int retval = RETURN_ERROR;
    struct DosLibrary *DOSBase;

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37L);
    if (!DOSBase)
        return RETURN_FAIL;

    {
        const char *tmpl = "HOST/A,TYPE/K";
        struct {
            STRPTR host;
            STRPTR type;
        } args = { NULL, NULL };
        struct RDArgs *rdargs;

        rdargs = ReadArgs((UBYTE *)tmpl, (IPTR *)&args, NULL);
        if (!rdargs) {
            Printf("Usage: nslookup <hostname|address> [TYPE A|AAAA|PTR|ANY]\n");
            CloseLibrary((struct Library *)DOSBase);
            return RETURN_FAIL;
        }

        {
            struct in_addr addr4;
            struct in6_addr addr6;
            int is_v4 = 0, is_v6 = 0;
            int qtypes = 0;
            char nsbuf[64];

            /* Determine query type from TYPE argument */
            if (args.type) {
                if (strcasecmp(args.type, "A") == 0)
                    qtypes = QTYPE_A;
                else if (strcasecmp(args.type, "AAAA") == 0)
                    qtypes = QTYPE_AAAA;
                else if (strcasecmp(args.type, "PTR") == 0)
                    qtypes = QTYPE_PTR;
                else if (strcasecmp(args.type, "ANY") == 0)
                    qtypes = QTYPE_ANY;
                else {
                    Printf("Unknown query type: %s\n", (CONST_STRPTR)args.type);
                    Printf("Valid types: A, AAAA, PTR, ANY\n");
                    FreeArgs(rdargs);
                    CloseLibrary((struct Library *)DOSBase);
                    return RETURN_FAIL;
                }
            }

            /* Print server info */
            if (get_nameserver(nsbuf, sizeof(nsbuf))) {
                struct hostent *nshp = NULL;
                struct in_addr nsaddr;
                struct in6_addr nsaddr6;

                /* Try to reverse-resolve the nameserver */
                if (inet_pton(AF_INET, nsbuf, &nsaddr) == 1)
                    nshp = gethostbyaddr(
                        (caddr_t)&nsaddr, sizeof(nsaddr), AF_INET);
                else if (inet_pton(AF_INET6, nsbuf, &nsaddr6) == 1)
                    nshp = gethostbyaddr(
                        (caddr_t)&nsaddr6, sizeof(nsaddr6), AF_INET6);

                Printf("Server:\t\t%s\n", (CONST_STRPTR)nsbuf);
                if (nshp && nshp->h_name)
                    Printf("Name:\t\t%s\n", (CONST_STRPTR)nshp->h_name);
                Printf("\n");
            }

            /* Detect if input is a numeric address */
            if (inet_pton(AF_INET, args.host, &addr4) == 1)
                is_v4 = 1;
            else if (inet_pton(AF_INET6, args.host, &addr6) == 1)
                is_v6 = 1;

            if (is_v4 || is_v6) {
                /* Numeric address: do reverse lookup (PTR) */
                if (qtypes && !(qtypes & QTYPE_PTR))
                    Printf("** address given but TYPE is not PTR\n");
                Printf("Non-authoritative answer:\n");
                if (is_v4) {
                    if (do_reverse_lookup(args.host, AF_INET,
                                          &addr4, sizeof(addr4)))
                        retval = RETURN_OK;
                } else {
                    if (do_reverse_lookup(args.host, AF_INET6,
                                          &addr6, sizeof(addr6)))
                        retval = RETURN_OK;
                }
            } else {
                /* Hostname: do forward lookup */
                if (!qtypes)
                    qtypes = QTYPE_A | QTYPE_AAAA;

                if (qtypes & QTYPE_PTR) {
                    Printf("** hostname given but TYPE=PTR requested;"
                           " ignoring PTR\n");
                    qtypes &= ~QTYPE_PTR;
                    if (!qtypes)
                        qtypes = QTYPE_A | QTYPE_AAAA;
                }

                Printf("Non-authoritative answer:\n");
                if (do_forward_lookup(args.host, qtypes))
                    retval = RETURN_OK;
            }
        }

        FreeArgs(rdargs);
    }

    CloseLibrary((struct Library *)DOSBase);
    return retval;
}
