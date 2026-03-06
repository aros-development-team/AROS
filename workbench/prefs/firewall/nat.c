/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Firewall Preferences -- IPF NAT rule parsing, formatting, and I/O.
    Reads and writes ipnat.rules in the format consumed by ipnat(1).
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

#include "prefsdata.h"

/* Global NAT rule storage */
struct NATRule natRules[FW_MAX_NAT_RULES];
int numNatRules = 0;

/* Labels for rule formatting */
static const char *natNames[]      = { "map", "rdr", "bimap" };
static const char *natProtoNames[] = { NULL, "tcp", "udp", "icmp", "tcp/udp" };

/*
 * Skip whitespace, return pointer to next non-space character.
 */
static const char *
skip_ws(const char *p)
{
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

/*
 * Copy a whitespace-delimited token from *pp into buf (max buflen-1 chars).
 * Advances *pp past the token and trailing whitespace.
 * Returns FALSE if no token available.
 */
static BOOL
next_token(const char **pp, char *buf, int buflen)
{
    const char *p = skip_ws(*pp);
    int i = 0;

    if (*p == '\0' || *p == '\n' || *p == '#')
        return FALSE;

    while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && i < buflen - 1)
        buf[i++] = *p++;
    buf[i] = '\0';

    *pp = p;
    return TRUE;
}

/*
 * Format a single NAT rule into ipnat.rules syntax.
 */
void
FormatNATRule(struct NATRule *rule, char *buf, int bufsize)
{
    char *p = buf;
    char *end = buf + bufsize - 1;
    int n;
    const char *proto;

    switch (rule->type)
    {
        case FW_NAT_MAP:
        case FW_NAT_BIMAP:
            /* map/bimap ifname src/mask -> nat_addr [portmap proto lo:hi] */
            n = snprintf(p, end - p, "%s %s %s -> %s",
                natNames[rule->type],
                rule->interface,
                rule->src_addr[0] ? rule->src_addr : "0/0",
                rule->nat_addr[0] ? rule->nat_addr : "0/32");
            p += n;

            if (rule->nat_port[0] != '\0' &&
                rule->protocol >= FW_PROTO_TCP &&
                rule->protocol <= FW_PROTO_TCPUDP)
            {
                proto = natProtoNames[rule->protocol];
                n = snprintf(p, end - p, " portmap %s %s", proto, rule->nat_port);
                p += n;
            }
            break;

        case FW_NAT_RDR:
            /* rdr ifname match_addr [port N] -> nat_addr [port M] proto */
            n = snprintf(p, end - p, "rdr %s %s",
                rule->interface,
                rule->src_addr[0] ? rule->src_addr : "0/0");
            p += n;

            if (rule->src_port[0] != '\0')
            {
                n = snprintf(p, end - p, " port %s", rule->src_port);
                p += n;
            }

            n = snprintf(p, end - p, " -> %s",
                rule->nat_addr[0] ? rule->nat_addr : "0/0");
            p += n;

            if (rule->nat_port[0] != '\0')
            {
                n = snprintf(p, end - p, " port %s", rule->nat_port);
                p += n;
            }

            if (rule->protocol >= FW_PROTO_TCP && rule->protocol <= FW_PROTO_TCPUDP)
            {
                n = snprintf(p, end - p, " %s", natProtoNames[rule->protocol]);
                p += n;
            }
            break;
    }

    *p = '\0';
}

/*
 * Parse a single ipnat.rules line into a NATRule.
 * Returns TRUE on success.
 */
BOOL
ParseNATLine(const char *line, struct NATRule *rule)
{
    char tok[64];
    const char *p = skip_ws(line);

    memset(rule, 0, sizeof(*rule));

    if (*p == '\0' || *p == '#' || *p == '\n')
        return FALSE;

    /* type: map / rdr / bimap / map-block */
    if (!next_token(&p, tok, sizeof(tok)))
        return FALSE;

    if (strcmp(tok, "map") == 0)
        rule->type = FW_NAT_MAP;
    else if (strcmp(tok, "rdr") == 0)
        rule->type = FW_NAT_RDR;
    else if (strcmp(tok, "bimap") == 0)
        rule->type = FW_NAT_BIMAP;
    else
        return FALSE;

    /* interface name */
    if (!next_token(&p, rule->interface, FW_MAX_IFNAME))
        return FALSE;

    /* source address/mask */
    if (!next_token(&p, rule->src_addr, FW_MAX_ADDR_LEN))
        return FALSE;

    /* For rdr: check for "port N" before "->" */
    if (rule->type == FW_NAT_RDR)
    {
        const char *saved = p;
        if (next_token(&p, tok, sizeof(tok)) && strcmp(tok, "port") == 0)
        {
            next_token(&p, rule->src_port, FW_MAX_PORT_LEN);
        }
        else
        {
            p = saved;
        }
    }

    /* Expect "->" */
    if (!next_token(&p, tok, sizeof(tok)) || strcmp(tok, "->") != 0)
        return FALSE;

    /* target address */
    if (!next_token(&p, rule->nat_addr, FW_MAX_ADDR_LEN))
        return FALSE;

    /* Remaining tokens: port, portmap, protocol */
    while (next_token(&p, tok, sizeof(tok)))
    {
        if (strcmp(tok, "port") == 0)
        {
            next_token(&p, rule->nat_port, FW_MAX_PORT_LEN);
        }
        else if (strcmp(tok, "portmap") == 0)
        {
            /* portmap proto lo:hi */
            if (next_token(&p, tok, sizeof(tok)))
            {
                if (strcmp(tok, "tcp") == 0) rule->protocol = FW_PROTO_TCP;
                else if (strcmp(tok, "udp") == 0) rule->protocol = FW_PROTO_UDP;
                else if (strcmp(tok, "tcp/udp") == 0) rule->protocol = FW_PROTO_TCPUDP;
            }
            next_token(&p, rule->nat_port, FW_MAX_PORT_LEN);
        }
        else if (strcmp(tok, "tcp") == 0)
            rule->protocol = FW_PROTO_TCP;
        else if (strcmp(tok, "udp") == 0)
            rule->protocol = FW_PROTO_UDP;
        else if (strcmp(tok, "tcp/udp") == 0)
            rule->protocol = FW_PROTO_TCPUDP;
    }

    return TRUE;
}

/*
 * Read NAT rules from an ipnat.rules file.
 */
void
ReadNATConf(STRPTR path)
{
    char filename[256];
    char line[512];
    BPTR fh;

    snprintf(filename, sizeof(filename), "%s/%s", path, IPNAT_CONF_NAME);
    fh = Open(filename, MODE_OLDFILE);
    if (fh == BNULL)
        return;

    numNatRules = 0;
    while (FGets(fh, line, sizeof(line) - 1) && numNatRules < FW_MAX_NAT_RULES)
    {
        struct NATRule rule;
        if (ParseNATLine(line, &rule))
            natRules[numNatRules++] = rule;
    }

    Close(fh);
}

/*
 * Write NAT rules to ipnat.rules in the given directory.
 */
BOOL
WriteNATConf(STRPTR dir)
{
    char filename[256];
    char line[512];
    BPTR fh;
    int i;

    snprintf(filename, sizeof(filename), "%s/%s", dir, IPNAT_CONF_NAME);
    fh = Open(filename, MODE_NEWFILE);
    if (fh == BNULL)
        return FALSE;

    FPuts(fh, "# AROS Firewall Preferences -- NAT rules for ipnat(1)\n");
    FPuts(fh, "# Generated file -- edit with Firewall preferences application\n");

    for (i = 0; i < numNatRules; i++)
    {
        FormatNATRule(&natRules[i], line, sizeof(line));
        FPuts(fh, line);
        FPuts(fh, "\n");
    }

    Close(fh);
    return TRUE;
}
