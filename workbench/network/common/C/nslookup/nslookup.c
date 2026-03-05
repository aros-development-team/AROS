/*
 * nslookup - DNS lookup utility for AROS
 *
 * Copyright (C) 2026 The AROS Dev Team
 *
 * Usage: nslookup [hostname|address [server]]
 *
 * If invoked with no arguments, enters interactive mode.
 * Supports IPv4 and IPv6 forward and reverse lookups.
 */

static const char version[] __attribute__((used)) =
    "$VER: nslookup 1.1 (05.03.2026)\n"
    "Copyright (C) 2026 The AROS Dev Team";

#include <proto/socket.h>
#include <proto/miami.h>
#include <proto/exec.h>

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

enum {
    QTYPE_A    = 1,
    QTYPE_AAAA = 2,
    QTYPE_PTR  = 4,
    QTYPE_MX   = 8,
    QTYPE_ANY  = (1|2|4)
};

static char default_server[128] = "";
static int query_type = 0;  /* 0 = auto */

/* Read the nameserver address from AROSTCP:db/netdb */
static int get_nameserver(char *buf, int buflen)
{
    FILE *fp;
    char line[256];

    fp = fopen("AROSTCP:db/netdb", "r");
    if (!fp)
        fp = fopen("ENV:AROSTCP/netdb", "r");
    if (!fp)
        return 0;

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == ';' || line[0] == '\n')
            continue;

        if (strncasecmp(line, "NAMESERVER", 10) == 0) {
            char *p = line + 10;
            int len;
            while (*p == ' ' || *p == '\t')
                p++;
            len = strlen(p);
            while (len > 0 && (p[len-1] == '\n' || p[len-1] == '\r' ||
                   p[len-1] == ' ' || p[len-1] == '\t'))
                p[--len] = '\0';
            if (*p) {
                strncpy(buf, p, buflen - 1);
                buf[buflen - 1] = '\0';
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 0;
}

static void print_server_info(void)
{
    if (default_server[0]) {
        struct hostent *nshp = NULL;
        struct in_addr nsaddr;
        struct in6_addr nsaddr6;

        if (inet_pton(AF_INET, default_server, &nsaddr) == 1)
            nshp = gethostbyaddr(
                (caddr_t)&nsaddr, sizeof(nsaddr), AF_INET);
        else if (inet_pton(AF_INET6, default_server, &nsaddr6) == 1)
            nshp = gethostbyaddr(
                (caddr_t)&nsaddr6, sizeof(nsaddr6), AF_INET6);

        printf("Server:\t\t%s\n", default_server);
        if (nshp && nshp->h_name)
            printf("Name:\t\t%s\n", nshp->h_name);
        printf("\n");
    }
}

/* Reverse lookup: address -> hostname */
static int do_reverse_lookup(const char *addrstr, int af,
                             const void *addr, int addrlen)
{
    struct hostent *hp;

    hp = gethostbyaddr((caddr_t)addr, addrlen, af);
    if (hp) {
        char **alias;

        printf("%s\tname = %s\n", addrstr, hp->h_name);
        for (alias = hp->h_aliases; alias && *alias; alias++)
            printf("\talias = %s\n", *alias);
        return 1;
    }

    printf("** server can't find %s: ", addrstr);
    switch (h_errno) {
    case HOST_NOT_FOUND: printf("NXDOMAIN\n"); break;
    case TRY_AGAIN:      printf("SERVFAIL\n"); break;
    case NO_RECOVERY:    printf("REFUSED\n"); break;
    case NO_DATA:        printf("NODATA\n"); break;
    default:             printf("UNKNOWN\n"); break;
    }
    return 0;
}

/* Forward lookup: hostname -> addresses */
static int do_forward_lookup(const char *hostname, int qtypes)
{
    int found = 0;

    if (qtypes & QTYPE_A) {
        struct hostent *hp = gethostbyname(hostname);
        if (hp && hp->h_addrtype == AF_INET) {
            char **ap;
            printf("Name:\t%s\n", hp->h_name);
            for (ap = hp->h_addr_list; ap && *ap; ap++) {
                struct in_addr *inaddr = (struct in_addr *)*ap;
                printf("Address: %s\n", Inet_NtoA(inaddr->s_addr));
                found++;
            }
        }
    }

    if (qtypes & QTYPE_AAAA) {
        struct hostent *hp6 = gethostbyname2(hostname, AF_INET6);
        if (hp6 && hp6->h_addrtype == AF_INET6) {
            char **ap;
            char buf[INET6_ADDRSTRLEN];
            if (!found)
                printf("Name:\t%s\n", hp6->h_name);
            for (ap = hp6->h_addr_list; ap && *ap; ap++) {
                if (inet_ntop(AF_INET6, *ap, buf, sizeof(buf)))
                    printf("Address: %s\n", buf);
                found++;
            }
        }
    }

    if (!found) {
        printf("** server can't find %s: ", hostname);
        switch (h_errno) {
        case HOST_NOT_FOUND: printf("NXDOMAIN\n"); break;
        case TRY_AGAIN:      printf("SERVFAIL\n"); break;
        case NO_RECOVERY:    printf("REFUSED\n"); break;
        case NO_DATA:        printf("NODATA\n"); break;
        default:             printf("UNKNOWN\n"); break;
        }
    }

    return found;
}

/* Perform a lookup on a single query string */
static int do_lookup(const char *query, int qtypes)
{
    struct in_addr addr4;
    struct in6_addr addr6;

    if (!qtypes)
        qtypes = QTYPE_A | QTYPE_AAAA;

    /* Detect numeric address for automatic reverse lookup */
    if (inet_pton(AF_INET, (char *)query, &addr4) == 1) {
        printf("Non-authoritative answer:\n");
        return do_reverse_lookup(query, AF_INET, &addr4, sizeof(addr4));
    }
    if (inet_pton(AF_INET6, (char *)query, &addr6) == 1) {
        printf("Non-authoritative answer:\n");
        return do_reverse_lookup(query, AF_INET6, &addr6, sizeof(addr6));
    }

    /* Forward lookup */
    if (qtypes & QTYPE_PTR) {
        qtypes &= ~QTYPE_PTR;
        if (!qtypes)
            qtypes = QTYPE_A | QTYPE_AAAA;
    }

    printf("Non-authoritative answer:\n");
    return do_forward_lookup(query, qtypes);
}

/* Strip leading/trailing whitespace in place, return pointer to content */
static char *strip(char *s)
{
    char *end;
    while (*s && isspace((unsigned char)*s))
        s++;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        end--;
    *end = '\0';
    return s;
}

/* Parse and execute an interactive command */
static int handle_command(char *line)
{
    char *cmd = strip(line);

    if (!*cmd)
        return 0;

    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0)
        return -1;

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        printf("Commands:\n");
        printf("  NAME            - look up NAME using current settings\n");
        printf("  server NAME     - set default server to NAME\n");
        printf("  set type=TYPE   - set query type (A, AAAA, PTR, ANY)\n");
        printf("  set all         - show current settings\n");
        printf("  server          - show default server\n");
        printf("  exit / quit     - exit nslookup\n");
        return 0;
    }

    if (strncmp(cmd, "server", 6) == 0) {
        char *arg = strip(cmd + 6);
        if (*arg) {
            strncpy(default_server, arg, sizeof(default_server) - 1);
            default_server[sizeof(default_server) - 1] = '\0';
            printf("Default server changed to %s\n\n", default_server);
        } else {
            print_server_info();
        }
        return 0;
    }

    if (strncmp(cmd, "lserver", 7) == 0) {
        char *arg = strip(cmd + 7);
        if (*arg) {
            strncpy(default_server, arg, sizeof(default_server) - 1);
            default_server[sizeof(default_server) - 1] = '\0';
            printf("Default server changed to %s\n\n", default_server);
        }
        return 0;
    }

    if (strncmp(cmd, "set ", 4) == 0) {
        char *arg = strip(cmd + 4);

        if (strcmp(arg, "all") == 0) {
            const char *tstr = "A+AAAA";
            if (query_type == QTYPE_A) tstr = "A";
            else if (query_type == QTYPE_AAAA) tstr = "AAAA";
            else if (query_type == QTYPE_PTR) tstr = "PTR";
            else if (query_type == QTYPE_ANY) tstr = "ANY";
            printf("Set options:\n");
            printf("  type=%s\n", tstr);
            printf("  server=%s\n", default_server[0] ? default_server : "(default)");
            return 0;
        }

        if (strncasecmp(arg, "type=", 5) == 0 ||
            strncasecmp(arg, "querytype=", 10) == 0 ||
            strncasecmp(arg, "q=", 2) == 0) {
            char *val = strchr(arg, '=') + 1;
            if (strcasecmp(val, "A") == 0)
                query_type = QTYPE_A;
            else if (strcasecmp(val, "AAAA") == 0)
                query_type = QTYPE_AAAA;
            else if (strcasecmp(val, "PTR") == 0)
                query_type = QTYPE_PTR;
            else if (strcasecmp(val, "ANY") == 0 ||
                     strcasecmp(val, "*") == 0)
                query_type = QTYPE_ANY;
            else
                printf("Unknown type: %s\n", val);
            return 0;
        }

        printf("Unknown set option: %s\n", arg);
        return 0;
    }

    /* Anything else is a hostname/address to look up */
    print_server_info();
    do_lookup(cmd, query_type);
    printf("\n");
    return 0;
}

/* Interactive mode */
static void interactive_mode(void)
{
    char line[512];

    print_server_info();

    for (;;) {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        if (handle_command(line) < 0)
            break;
    }
}

int main(int argc, char *argv[])
{
    get_nameserver(default_server, sizeof(default_server));

    if (argc < 2) {
        /* No arguments: interactive mode */
        interactive_mode();
    } else {
        /* Non-interactive: nslookup hostname [server] */
        if (argc >= 3) {
            strncpy(default_server, argv[2],
                sizeof(default_server) - 1);
            default_server[sizeof(default_server) - 1] = '\0';
        }

        print_server_info();
        if (do_lookup(argv[1], QTYPE_A | QTYPE_AAAA))
            return 0;
        return 5;
    }

    return 0;
}
