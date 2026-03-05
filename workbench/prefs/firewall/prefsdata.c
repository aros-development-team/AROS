/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Firewall Preferences -- configuration file I/O and rule application.
    Reads and writes ipf.conf (filter rules) and ipnat.conf (NAT rules)
    in the format consumed by the ipf(1) and ipnat(1) commands.
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

#include "prefsdata.h"

/* Global rule storage */
struct FilterRule filterRules[FW_MAX_FILTER_RULES];
int numFilterRules = 0;

struct NATRule natRules[FW_MAX_NAT_RULES];
int numNatRules = 0;

/* Labels for rule formatting */
static const char *actionNames[] = { "pass", "block", "count" };
static const char *dirNames[]    = { "in", "out" };
static const char *protoNames[]  = { NULL, "tcp", "udp", "icmp", "tcp/udp" };
static const char *natNames[]    = { "map", "rdr", "bimap" };

/*
 * Format a single filter rule into ipf.conf syntax.
 */
void
FormatFilterRule(struct FilterRule *rule, char *buf, int bufsize)
{
    char *p = buf;
    char *end = buf + bufsize - 1;
    int n;

    /* action */
    n = snprintf(p, end - p, "%s", actionNames[rule->action]);
    p += n;

    /* direction */
    n = snprintf(p, end - p, " %s", dirNames[rule->direction]);
    p += n;

    /* log */
    if (rule->log)
    {
        n = snprintf(p, end - p, " log");
        p += n;
    }

    /* quick */
    if (rule->quick)
    {
        n = snprintf(p, end - p, " quick");
        p += n;
    }

    /* interface */
    if (rule->interface[0] != '\0')
    {
        n = snprintf(p, end - p, " on %s", rule->interface);
        p += n;
    }

    /* protocol */
    if (rule->protocol != FW_PROTO_ANY)
    {
        n = snprintf(p, end - p, " proto %s", protoNames[rule->protocol]);
        p += n;
    }

    /* source */
    {
        const char *src = (rule->src_addr[0] != '\0') ? rule->src_addr : "any";
        n = snprintf(p, end - p, " from %s", src);
        p += n;
        if (rule->src_port[0] != '\0')
        {
            n = snprintf(p, end - p, " port = %s", rule->src_port);
            p += n;
        }
    }

    /* destination */
    {
        const char *dst = (rule->dst_addr[0] != '\0') ? rule->dst_addr : "any";
        n = snprintf(p, end - p, " to %s", dst);
        p += n;
        if (rule->dst_port[0] != '\0')
        {
            n = snprintf(p, end - p, " port = %s", rule->dst_port);
            p += n;
        }
    }

    /* keep state */
    if (rule->keep_state)
    {
        n = snprintf(p, end - p, " keep state");
        p += n;
    }

    *p = '\0';
}

/*
 * Format a single NAT rule into ipnat.conf syntax.
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
                proto = protoNames[rule->protocol];
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
                n = snprintf(p, end - p, " %s", protoNames[rule->protocol]);
                p += n;
            }
            break;
    }

    *p = '\0';
}

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
 * Parse a single ipf.conf line into a FilterRule.
 * Returns TRUE on success.
 */
static BOOL
ParseFilterLine(const char *line, struct FilterRule *rule)
{
    char tok[64];
    const char *p = skip_ws(line);

    memset(rule, 0, sizeof(*rule));

    /* Skip comments and blank lines */
    if (*p == '\0' || *p == '#' || *p == '\n')
        return FALSE;

    /* Skip @position prefix */
    if (*p == '@')
    {
        while (*p && *p != ' ' && *p != '\t')
            p++;
    }

    /* action: pass / block / count / log */
    if (!next_token(&p, tok, sizeof(tok)))
        return FALSE;

    if (strcmp(tok, "pass") == 0)
        rule->action = FW_ACTION_PASS;
    else if (strcmp(tok, "block") == 0)
        rule->action = FW_ACTION_BLOCK;
    else if (strcmp(tok, "count") == 0)
        rule->action = FW_ACTION_COUNT;
    else if (strcmp(tok, "log") == 0)
    {
        rule->action = FW_ACTION_PASS;
        rule->log = TRUE;
    }
    else
        return FALSE;

    /* Parse remaining tokens */
    while (next_token(&p, tok, sizeof(tok)))
    {
        if (strcmp(tok, "in") == 0)
            rule->direction = FW_DIR_IN;
        else if (strcmp(tok, "out") == 0)
            rule->direction = FW_DIR_OUT;
        else if (strcmp(tok, "quick") == 0)
            rule->quick = TRUE;
        else if (strcmp(tok, "log") == 0)
            rule->log = TRUE;
        else if (strcmp(tok, "on") == 0)
        {
            next_token(&p, rule->interface, FW_MAX_IFNAME);
        }
        else if (strcmp(tok, "proto") == 0)
        {
            if (next_token(&p, tok, sizeof(tok)))
            {
                if (strcmp(tok, "tcp") == 0) rule->protocol = FW_PROTO_TCP;
                else if (strcmp(tok, "udp") == 0) rule->protocol = FW_PROTO_UDP;
                else if (strcmp(tok, "icmp") == 0) rule->protocol = FW_PROTO_ICMP;
                else if (strcmp(tok, "tcp/udp") == 0) rule->protocol = FW_PROTO_TCPUDP;
            }
        }
        else if (strcmp(tok, "from") == 0)
        {
            next_token(&p, rule->src_addr, FW_MAX_ADDR_LEN);
            if (strcmp(rule->src_addr, "any") == 0)
                rule->src_addr[0] = '\0';

            /* Check for "port" after source address */
            const char *saved = p;
            if (next_token(&p, tok, sizeof(tok)) && strcmp(tok, "port") == 0)
            {
                /* Skip operator (=, <, >, etc.) */
                if (next_token(&p, tok, sizeof(tok)))
                {
                    if (tok[0] == '=' || tok[0] == '<' || tok[0] == '>' ||
                        strcmp(tok, "eq") == 0 || strcmp(tok, "ne") == 0)
                    {
                        next_token(&p, rule->src_port, FW_MAX_PORT_LEN);
                    }
                    else
                    {
                        /* No operator, token is the port itself */
                        strlcpy(rule->src_port, tok, FW_MAX_PORT_LEN);
                    }
                }
            }
            else
            {
                p = saved;
            }
        }
        else if (strcmp(tok, "to") == 0)
        {
            next_token(&p, rule->dst_addr, FW_MAX_ADDR_LEN);
            if (strcmp(rule->dst_addr, "any") == 0)
                rule->dst_addr[0] = '\0';

            /* Check for "port" after destination address */
            const char *saved = p;
            if (next_token(&p, tok, sizeof(tok)) && strcmp(tok, "port") == 0)
            {
                if (next_token(&p, tok, sizeof(tok)))
                {
                    if (tok[0] == '=' || tok[0] == '<' || tok[0] == '>' ||
                        strcmp(tok, "eq") == 0 || strcmp(tok, "ne") == 0)
                    {
                        next_token(&p, rule->dst_port, FW_MAX_PORT_LEN);
                    }
                    else
                    {
                        strlcpy(rule->dst_port, tok, FW_MAX_PORT_LEN);
                    }
                }
            }
            else
            {
                p = saved;
            }
        }
        else if (strcmp(tok, "all") == 0)
        {
            /* "all" = from any to any, already default */
        }
        else if (strcmp(tok, "keep") == 0)
        {
            if (next_token(&p, tok, sizeof(tok)) && strcmp(tok, "state") == 0)
                rule->keep_state = TRUE;
        }
        else if (strcmp(tok, "return-rst") == 0 || strcmp(tok, "return-icmp") == 0)
        {
            /* Block sub-type, keep action as block */
        }
    }

    return TRUE;
}

/*
 * Parse a single ipnat.conf line into a NATRule.
 * Returns TRUE on success.
 */
static BOOL
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
 * Read filter rules from an ipf.conf file.
 */
static void
ReadFilterConf(STRPTR path)
{
    char filename[256];
    char line[512];
    BPTR fh;

    snprintf(filename, sizeof(filename), "%s/%s", path, IPF_CONF_NAME);
    fh = Open(filename, MODE_OLDFILE);
    if (fh == BNULL)
        return;

    numFilterRules = 0;
    while (FGets(fh, line, sizeof(line) - 1) && numFilterRules < FW_MAX_FILTER_RULES)
    {
        struct FilterRule rule;
        if (ParseFilterLine(line, &rule))
            filterRules[numFilterRules++] = rule;
    }

    Close(fh);
}

/*
 * Read NAT rules from an ipnat.conf file.
 */
static void
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
 * Write filter rules to ipf.conf in the given directory.
 */
BOOL
WriteFilterConf(STRPTR dir)
{
    char filename[256];
    char line[512];
    BPTR fh;
    int i;

    snprintf(filename, sizeof(filename), "%s/%s", dir, IPF_CONF_NAME);
    fh = Open(filename, MODE_NEWFILE);
    if (fh == BNULL)
        return FALSE;

    FPuts(fh, "# AROS Firewall Preferences -- filter rules for ipf(1)\n");
    FPuts(fh, "# Generated file -- edit with Firewall preferences application\n");

    for (i = 0; i < numFilterRules; i++)
    {
        FormatFilterRule(&filterRules[i], line, sizeof(line));
        FPuts(fh, line);
        FPuts(fh, "\n");
    }

    Close(fh);
    return TRUE;
}

/*
 * Write NAT rules to ipnat.conf in the given directory.
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

/*
 * Load existing rules from the given directory.
 */
void
InitFirewallPrefs(STRPTR path)
{
    ReadFilterConf(path);
    ReadNATConf(path);
}

/*
 * Apply the current rules by running ipf and ipnat commands.
 */
BOOL
ApplyFirewallRules(void)
{
    char cmd[512];

    /* Flush existing filter rules and load new ones */
    SystemTags("ipf -Fa", SYS_Asynch, FALSE, SYS_Output, BNULL, TAG_DONE);

    if (numFilterRules > 0)
    {
        snprintf(cmd, sizeof(cmd), "ipf -f %s/%s", PREFS_PATH_ENV, IPF_CONF_NAME);
        SystemTags(cmd, SYS_Asynch, FALSE, SYS_Output, BNULL, TAG_DONE);
    }

    /* Flush existing NAT rules and load new ones */
    SystemTags("ipnat -CF", SYS_Asynch, FALSE, SYS_Output, BNULL, TAG_DONE);

    if (numNatRules > 0)
    {
        snprintf(cmd, sizeof(cmd), "ipnat -f %s/%s", PREFS_PATH_ENV, IPNAT_CONF_NAME);
        SystemTags(cmd, SYS_Asynch, FALSE, SYS_Output, BNULL, TAG_DONE);
    }

    return TRUE;
}

/*
 * Save: write to ENVARC (persistent) and ENV (runtime), then apply.
 */
BOOL
SaveFirewallPrefs(void)
{
    BOOL ok = TRUE;

    ok = ok && WriteFilterConf(PREFS_PATH_ENVARC);
    ok = ok && WriteNATConf(PREFS_PATH_ENVARC);
    ok = ok && WriteFilterConf(PREFS_PATH_ENV);
    ok = ok && WriteNATConf(PREFS_PATH_ENV);

    if (ok)
        ApplyFirewallRules();

    return ok;
}

/*
 * Use: write to ENV (runtime) only, then apply.
 */
BOOL
UseFirewallPrefs(void)
{
    BOOL ok = TRUE;

    ok = ok && WriteFilterConf(PREFS_PATH_ENV);
    ok = ok && WriteNATConf(PREFS_PATH_ENV);

    if (ok)
        ApplyFirewallRules();

    return ok;
}

/*
 * Handle USE/SAVE command-line arguments (no GUI).
 */
void
Prefs_HandleArgs(BOOL use, BOOL save)
{
    if (save)
        SaveFirewallPrefs();
    else if (use)
        UseFirewallPrefs();
}
