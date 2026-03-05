#ifndef _PREFSDATA_H_
#define _PREFSDATA_H_

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>

#define PREFS_PATH_ENV      "ENV:AROSTCP"
#define PREFS_PATH_ENVARC   "ENVARC:AROSTCP"

#define IPF_CONF_NAME       "ipf.conf"
#define IPNAT_CONF_NAME     "ipnat.conf"

#define FW_MAX_ADDR_LEN     48      /* IPv6 + /prefix */
#define FW_MAX_PORT_LEN     12      /* "65535" or range */
#define FW_MAX_IFNAME       16
#define FW_MAX_FILTER_RULES 256
#define FW_MAX_NAT_RULES    64

/* Filter rule actions */
enum {
    FW_ACTION_PASS = 0,
    FW_ACTION_BLOCK,
    FW_ACTION_COUNT,
    FW_ACTION_NUM
};

/* Filter rule directions */
enum {
    FW_DIR_IN = 0,
    FW_DIR_OUT,
    FW_DIR_NUM
};

/* Protocol selections */
enum {
    FW_PROTO_ANY = 0,
    FW_PROTO_TCP,
    FW_PROTO_UDP,
    FW_PROTO_ICMP,
    FW_PROTO_TCPUDP,
    FW_PROTO_NUM
};

/* NAT rule types */
enum {
    FW_NAT_MAP = 0,
    FW_NAT_RDR,
    FW_NAT_BIMAP,
    FW_NAT_NUM
};

struct FilterRule {
    LONG  action;                       /* FW_ACTION_* */
    LONG  direction;                    /* FW_DIR_* */
    BOOL  quick;
    BOOL  log;
    BOOL  keep_state;
    LONG  protocol;                     /* FW_PROTO_* */
    char  interface[FW_MAX_IFNAME];     /* empty = any */
    char  src_addr[FW_MAX_ADDR_LEN];   /* empty or "any" */
    char  src_port[FW_MAX_PORT_LEN];   /* empty = any */
    char  dst_addr[FW_MAX_ADDR_LEN];
    char  dst_port[FW_MAX_PORT_LEN];
};

struct NATRule {
    LONG  type;                         /* FW_NAT_* */
    LONG  protocol;                     /* FW_PROTO_* */
    char  interface[FW_MAX_IFNAME];
    char  src_addr[FW_MAX_ADDR_LEN];   /* match address/mask */
    char  src_port[FW_MAX_PORT_LEN];   /* for rdr: match port */
    char  nat_addr[FW_MAX_ADDR_LEN];   /* target address */
    char  nat_port[FW_MAX_PORT_LEN];   /* target port or range */
};

/* Global rule storage */
extern struct FilterRule filterRules[FW_MAX_FILTER_RULES];
extern int numFilterRules;

extern struct NATRule natRules[FW_MAX_NAT_RULES];
extern int numNatRules;

/* Functions */
void InitFirewallPrefs(STRPTR path);
BOOL WriteFilterConf(STRPTR dir);
BOOL WriteNATConf(STRPTR dir);
BOOL SaveFirewallPrefs(void);
BOOL UseFirewallPrefs(void);
BOOL ApplyFirewallRules(void);
void Prefs_HandleArgs(BOOL use, BOOL save);

/* Format a rule into human-readable text */
void FormatFilterRule(struct FilterRule *rule, char *buf, int bufsize);
void FormatNATRule(struct NATRule *rule, char *buf, int bufsize);

#endif /* _PREFSDATA_H_ */
