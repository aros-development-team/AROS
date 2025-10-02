/* dhcpd.h

   Definitions for dhcpd... */

/*
 * Copyright (c) 2004-2005 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996-2003 by Internet Software Consortium
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   http://www.isc.org/
 *
 * This software has been written for Internet Systems Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about Internet Systems Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#ifndef __CYGWIN32__
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <netdb.h>
#else
#define fd_set cygwin_fd_set
#include <sys/types.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

#include "cdefs.h"
#include "dhcp_osdep.h"

#include "arpa/nameser.h"
#if defined (NSUPDATE)
# include "minires/minires.h"
#endif

struct hash_table;
typedef struct hash_table group_hash_t;
typedef struct hash_table universe_hash_t;
typedef struct hash_table option_hash_t;
typedef struct hash_table dns_zone_hash_t;
typedef struct hash_table lease_hash_t;
typedef struct hash_table host_hash_t;
typedef struct hash_table class_hash_t;

#include "dhcp.h"
#include "statement.h"
#include "tree.h"
#include "inet.h"
#include "dhctoken.h"

#include <isc-dhcp/result.h>
#include <omapip/omapip_p.h>

#if !defined (OPTION_HASH_SIZE)
# define OPTION_HASH_SIZE 17
# define OPTION_HASH_PTWO 32	/* Next power of two above option hash. */
# define OPTION_HASH_EXP 5	/* The exponent for that power of two. */
#endif

#define compute_option_hash(x) \
	(((x) & (OPTION_HASH_PTWO - 1)) + \
	 (((x) >> OPTION_HASH_EXP) & \
	  (OPTION_HASH_PTWO - 1))) % OPTION_HASH_SIZE;

enum dhcp_shutdown_state {
	shutdown_listeners,
	shutdown_omapi_connections,
	shutdown_drop_omapi_connections,
	shutdown_dhcp,
	shutdown_done
};

/* Client FQDN option, failover FQDN option, etc. */
typedef struct {
	u_int8_t codes [2];
	unsigned length;
	u_int8_t *data;
} ddns_fqdn_t;

#include "failover.h"

/* A parsing context. */

struct parse {
	int lexline;
	int lexchar;
	char *token_line;
	char *prev_line;
	char *cur_line;
	const char *tlname;
	int eol_token;

	char line1 [81];
	char line2 [81];
	int lpos;
	int line;
	int tlpos;
	int tline;
	enum dhcp_token token;
	int ugflag;
	char *tval;
	int tlen;
	char tokbuf [1500];

#ifdef OLD_LEXER
	char comments [4096];
	int comment_index;
#endif
	int warnings_occurred;
	int file;
	char *inbuf;
	unsigned bufix, buflen;
	unsigned bufsiz;
};

/* Variable-length array of data. */

struct string_list {
	struct string_list *next;
	char string [1];
};

/* A name server, from /etc/resolv.conf. */
struct name_server {
	struct name_server *next;
	struct sockaddr_in addr;
	TIME rcdate;
};

/* A domain search list element. */
struct domain_search_list {
	struct domain_search_list *next;
	char *domain;
	TIME rcdate;
};

/* Option tag structures are used to build chains of option tags, for
   when we're sure we're not going to have enough of them to justify
   maintaining an array. */

struct option_tag {
	struct option_tag *next;
	u_int8_t data [1];
};

/* An agent option structure.   We need a special structure for the
   Relay Agent Information option because if more than one appears in
   a message, we have to keep them seperate. */

struct agent_options {
	struct agent_options *next;
	int length;
	struct option_tag *first;
};

struct option_cache {
	int refcnt;
	struct option_cache *next;
	struct expression *expression;
	struct option *option;
	struct data_string data;
};

struct option_state {
	int refcnt;
	int universe_count;
	int site_universe;
	int site_code_min;
	VOIDPTR universes [1];
};

/* A dhcp packet and the pointers to its option values. */
struct packet {
	struct dhcp_packet *raw;
	int refcnt;
	unsigned packet_length;
	int packet_type;
	int options_valid;
	int client_port;
	struct iaddr client_addr;
	struct interface_info *interface;	/* Interface on which packet
						   was received. */
	struct hardware *haddr;		/* Physical link address
					   of local sender (maybe gateway). */

	/* Information for relay agent options (see
	   draft-ietf-dhc-agent-options-xx.txt). */
	u_int8_t *circuit_id;		/* Circuit ID of client connection. */
	int circuit_id_len;
	u_int8_t *remote_id;		/* Remote ID of client. */
	int remote_id_len;

	int got_requested_address;	/* True if client sent the
					   dhcp-requested-address option. */

	struct shared_network *shared_network;
	struct option_state *options;

#if !defined (PACKET_MAX_CLASSES)
# define PACKET_MAX_CLASSES 5
#endif
	int class_count;
	struct class *classes [PACKET_MAX_CLASSES];

	int known;
	int authenticated;
};

/* A network interface's MAC address. */

struct hardware {
	u_int8_t hlen;
	u_int8_t hbuf [17];
};

typedef enum {
	server_startup = 0,
	server_running = 1,
	server_shutdown = 2,
	server_hibernate = 3,
	server_awaken = 4
} control_object_state_t;

typedef struct {
	OMAPI_OBJECT_PREAMBLE;
	control_object_state_t state;
} dhcp_control_object_t;

/* Lease states: */
#define	FTS_FREE	1
#define	FTS_ACTIVE	2
#define	FTS_EXPIRED	3
#define	FTS_RELEASED	4
#define	FTS_ABANDONED	5
#define	FTS_RESET	6
#define	FTS_BACKUP	7
typedef u_int8_t binding_state_t;

/* FTS_LAST is the highest value that is valid for a lease binding state. */
#define FTS_LAST FTS_BACKUP

/* A dhcp lease declaration structure. */
struct lease {
	OMAPI_OBJECT_PREAMBLE;
	struct lease *next;
	struct lease *n_uid, *n_hw;

	struct iaddr ip_addr;
	TIME starts, ends, timestamp, sort_time;
	char *client_hostname;
	struct binding_scope *scope;
	struct host_decl *host;
	struct subnet *subnet;
	struct pool *pool;
	struct class *billing_class;
	struct option_chain_head *agent_options;

	struct executable_statement *on_expiry;
	struct executable_statement *on_commit;
	struct executable_statement *on_release;

	unsigned char *uid;
	unsigned short uid_len;
	unsigned short uid_max;
	unsigned char uid_buf [7];
	struct hardware hardware_addr;

	u_int8_t flags;
#       define STATIC_LEASE		1
#	define BOOTP_LEASE		2
#	define PERSISTENT_FLAGS		(ON_ACK_QUEUE | ON_UPDATE_QUEUE)
#	define MS_NULL_TERMINATION	8
#	define ON_UPDATE_QUEUE		16
#	define ON_ACK_QUEUE		32
#	define UNICAST_BROADCAST_HACK	64
#	define ON_DEFERRED_QUEUE	128
#	define EPHEMERAL_FLAGS		(MS_NULL_TERMINATION | \
					 UNICAST_BROADCAST_HACK)

	binding_state_t binding_state;
	binding_state_t next_binding_state;
	binding_state_t desired_binding_state;
	
	struct lease_state *state;

	TIME tstp;	/* Time sent to partner. */
	TIME tsfp;	/* Time sent from partner. */
	TIME cltt;	/* Client last transaction time. */
	struct lease *next_pending;
};

struct lease_state {
	struct lease_state *next;

	struct interface_info *ip;

	struct packet *packet;	/* The incoming packet. */

	TIME offered_expiry;

	struct option_state *options;
	struct data_string parameter_request_list;
	int max_message_size;
	u_int32_t expiry, renewal, rebind;
	struct data_string filename, server_name;
	int got_requested_address;
	int got_server_identifier;
	struct shared_network *shared_network;	/* Shared network of interface
						   on which request arrived. */

	u_int32_t xid;
	u_int16_t secs;
	u_int16_t bootp_flags;
	struct in_addr ciaddr;
	struct in_addr siaddr;
	struct in_addr giaddr;
	u_int8_t hops;
	u_int8_t offer;
	struct iaddr from;
};

#define	ROOT_GROUP	0
#define HOST_DECL	1
#define SHARED_NET_DECL	2
#define SUBNET_DECL	3
#define CLASS_DECL	4
#define	GROUP_DECL	5
#define POOL_DECL	6

/* Possible modes in which discover_interfaces can run. */

#define DISCOVER_RUNNING	0
#define DISCOVER_SERVER		1
#define DISCOVER_UNCONFIGURED	2
#define DISCOVER_RELAY		3
#define DISCOVER_REQUESTED	4

/* Server option names. */

#define SV_DEFAULT_LEASE_TIME		1
#define SV_MAX_LEASE_TIME		2
#define SV_MIN_LEASE_TIME		3
#define SV_BOOTP_LEASE_CUTOFF		4
#define SV_BOOTP_LEASE_LENGTH		5
#define SV_BOOT_UNKNOWN_CLIENTS		6
#define SV_DYNAMIC_BOOTP		7
#define	SV_ALLOW_BOOTP			8
#define	SV_ALLOW_BOOTING		9
#define	SV_ONE_LEASE_PER_CLIENT		10
#define	SV_GET_LEASE_HOSTNAMES		11
#define	SV_USE_HOST_DECL_NAMES		12
#define	SV_USE_LEASE_ADDR_FOR_DEFAULT_ROUTE	13
#define	SV_MIN_SECS			14
#define	SV_FILENAME			15
#define SV_SERVER_NAME			16
#define	SV_NEXT_SERVER			17
#define SV_AUTHORITATIVE		18
#define SV_VENDOR_OPTION_SPACE		19
#define SV_ALWAYS_REPLY_RFC1048		20
#define SV_SITE_OPTION_SPACE		21
#define SV_ALWAYS_BROADCAST		22
#define SV_DDNS_DOMAIN_NAME		23
#define SV_DDNS_HOST_NAME		24
#define SV_DDNS_REV_DOMAIN_NAME		25
#define SV_LEASE_FILE_NAME		26
#define SV_PID_FILE_NAME		27
#define SV_DUPLICATES			28
#define SV_DECLINES			29
#define SV_DDNS_UPDATES			30
#define SV_OMAPI_PORT			31
#define SV_LOCAL_PORT			32
#define SV_LIMITED_BROADCAST_ADDRESS	33
#define SV_REMOTE_PORT			34
#define SV_LOCAL_ADDRESS		35
#define SV_OMAPI_KEY			36
#define SV_STASH_AGENT_OPTIONS		37
#define SV_DDNS_TTL			38
#define SV_DDNS_UPDATE_STYLE		39
#define SV_CLIENT_UPDATES		40
#define SV_UPDATE_OPTIMIZATION		41
#define SV_PING_CHECKS			42
#define SV_UPDATE_STATIC_LEASES		43
#define SV_LOG_FACILITY			44
#define SV_DO_FORWARD_UPDATES		45
#define SV_PING_TIMEOUT         46

#if !defined (DEFAULT_PING_TIMEOUT)
# define DEFAULT_PING_TIMEOUT 1
#endif

#if !defined (DEFAULT_DEFAULT_LEASE_TIME)
# define DEFAULT_DEFAULT_LEASE_TIME 43200
#endif

#if !defined (DEFAULT_MIN_LEASE_TIME)
# define DEFAULT_MIN_LEASE_TIME 0
#endif

#if !defined (DEFAULT_MAX_LEASE_TIME)
# define DEFAULT_MAX_LEASE_TIME 86400
#endif

#if !defined (DEFAULT_DDNS_TTL)
# define DEFAULT_DDNS_TTL 3600
#endif

/* Client option names */

#define	CL_TIMEOUT		1
#define	CL_SELECT_INTERVAL	2
#define CL_REBOOT_TIMEOUT	3
#define CL_RETRY_INTERVAL	4
#define CL_BACKOFF_CUTOFF	5
#define CL_INITIAL_INTERVAL	6
#define CL_BOOTP_POLICY		7
#define	CL_SCRIPT_NAME		8
#define CL_REQUESTED_OPTIONS	9
#define CL_REQUESTED_LEASE_TIME	10
#define CL_SEND_OPTIONS		11
#define CL_MEDIA		12
#define	CL_REJECT_LIST		13

#ifndef CL_DEFAULT_TIMEOUT
# define CL_DEFAULT_TIMEOUT	60
#endif

#ifndef CL_DEFAULT_SELECT_INTERVAL
# define CL_DEFAULT_SELECT_INTERVAL 0
#endif

#ifndef CL_DEFAULT_REBOOT_TIMEOUT
# define CL_DEFAULT_REBOOT_TIMEOUT 10
#endif

#ifndef CL_DEFAULT_RETRY_INTERVAL
# define CL_DEFAULT_RETRY_INTERVAL 300
#endif

#ifndef CL_DEFAULT_BACKOFF_CUTOFF
# define CL_DEFAULT_BACKOFF_CUTOFF 120
#endif

#ifndef CL_DEFAULT_INITIAL_INTERVAL
# define CL_DEFAULT_INITIAL_INTERVAL 10
#endif

#ifndef CL_DEFAULT_BOOTP_POLICY
# define CL_DEFAULT_BOOTP_POLICY P_ACCEPT
#endif

#ifndef CL_DEFAULT_REQUESTED_OPTIONS
# define CL_DEFAULT_REQUESTED_OPTIONS \
	{ DHO_SUBNET_MASK, \
	  DHO_BROADCAST_ADDRESS, \
	  DHO_TIME_OFFSET, \
	  DHO_ROUTERS, \
	  DHO_DOMAIN_NAME, \
	  DHO_DOMAIN_NAME_SERVERS, \
	  DHO_HOST_NAME }
#endif

struct group_object {
	OMAPI_OBJECT_PREAMBLE;

	struct group_object *n_dynamic;
	struct group *group;
	char *name;
	int flags;
#define GROUP_OBJECT_DELETED	1
#define GROUP_OBJECT_DYNAMIC	2
#define GROUP_OBJECT_STATIC	4
};

/* Group of declarations that share common parameters. */
struct group {
	struct group *next;

	int refcnt;
	struct group_object *object;
	struct subnet *subnet;
	struct shared_network *shared_network;
	int authoritative;
	struct executable_statement *statements;
};

/* A dhcp host declaration structure. */
struct host_decl {
	OMAPI_OBJECT_PREAMBLE;
	struct host_decl *n_ipaddr;
	struct host_decl *n_dynamic;
	char *name;
	struct hardware interface;
	struct data_string client_identifier;
	struct option_cache *fixed_addr;
	struct group *group;
	struct group_object *named_group;
	struct data_string auth_key_id;
	int flags;
#define HOST_DECL_DELETED	1
#define HOST_DECL_DYNAMIC	2
#define HOST_DECL_STATIC	4
};

struct permit {
	struct permit *next;
	enum {
		permit_unknown_clients,
		permit_known_clients,
		permit_authenticated_clients,
		permit_unauthenticated_clients,
		permit_all_clients,
		permit_dynamic_bootp_clients,
		permit_class
	} type;
	struct class *class;
};

struct pool {
	OMAPI_OBJECT_PREAMBLE;
	struct pool *next;
	struct group *group;
	struct shared_network *shared_network;
	struct permit *permit_list;
	struct permit *prohibit_list;
	struct lease *active;
	struct lease *expired;
	struct lease *free;
	struct lease *backup;
	struct lease *abandoned;
	TIME next_event_time;
	int lease_count;
	int free_leases;
	int backup_leases;
	int index;
#if defined (FAILOVER_PROTOCOL)
	dhcp_failover_state_t *failover_peer;
#endif
};

struct shared_network {
	OMAPI_OBJECT_PREAMBLE;
	struct shared_network *next;
	char *name;
	struct subnet *subnets;
	struct interface_info *interface;
	struct pool *pools;
	struct group *group;
#if defined (FAILOVER_PROTOCOL)
	dhcp_failover_state_t *failover_peer;
#endif
};

struct subnet {
	OMAPI_OBJECT_PREAMBLE;
	struct subnet *next_subnet;
	struct subnet *next_sibling;
	struct shared_network *shared_network;
	struct interface_info *interface;
	struct iaddr interface_address;
	struct iaddr net;
	struct iaddr netmask;

	struct group *group;
};

struct collection {
	struct collection *next;
	
	const char *name;
	struct class *classes;
};

/* XXX classes must be reference-counted. */
struct class {
	OMAPI_OBJECT_PREAMBLE;
	struct class *nic;		/* Next in collection. */
	struct class *superclass;	/* Set for spawned classes only. */
	char *name;			/* Not set for spawned classes. */

	/* A class may be configured to permit a limited number of leases. */
	int lease_limit;
	int leases_consumed;
	struct lease **billed_leases;

	/* If nonzero, class has not been saved since it was last
	   modified. */
	int dirty;

	/* Hash table containing subclasses. */
	class_hash_t *hash;
	struct data_string hash_string;

	/* Expression used to match class. */
	struct expression *expr;

	/* Expression used to compute subclass identifiers for spawning
	   and to do subclass matching. */
	struct expression *submatch;
	int spawning;
	
	struct group *group;

	/* Statements to execute if class matches. */
	struct executable_statement *statements;
};

/* DHCP client lease structure... */
struct client_lease {
	struct client_lease *next;		      /* Next lease in list. */
	TIME expiry, renewal, rebind;			  /* Lease timeouts. */
	struct iaddr address;			    /* Address being leased. */
	char *server_name;			     /* Name of boot server. */
	char *filename;		     /* Name of file we're supposed to boot. */
	struct string_list *medium;			  /* Network medium. */
	struct auth_key *key;      /* Key used in basic DHCP authentication. */

	unsigned int is_static : 1;    /* If set, lease is from config file. */
	unsigned int is_bootp: 1;  /* If set, lease was acquired with BOOTP. */

	struct option_state *options;	     /* Options supplied with lease. */
};

/* Possible states in which the client can be. */
enum dhcp_state {
	S_REBOOTING = 1,
	S_INIT = 2,
	S_SELECTING = 3,
	S_REQUESTING = 4, 
	S_BOUND = 5,
	S_RENEWING = 6,
	S_REBINDING = 7,
	S_STOPPED = 8
};

/* Authentication and BOOTP policy possibilities (not all values work
   for each). */
enum policy { P_IGNORE, P_ACCEPT, P_PREFER, P_REQUIRE, P_DONT };

/* Configuration information from the config file... */
struct client_config {
	/*
	 * When a message has been received, run these statements
	 * over it.
	 */
	struct group *on_receipt;

	/*
	 * When a message is sent, run these statements.
	 */
	struct group *on_transmission;

	u_int32_t *required_options; /* Options server must supply. */
	u_int32_t *requested_options; /* Options to request from server. */

	TIME timeout;			/* Start to panic if we don't get a
					   lease in this time period when
					   SELECTING. */
	TIME initial_interval;		/* All exponential backoff intervals
					   start here. */
	TIME retry_interval;		/* If the protocol failed to produce
					   an address before the timeout,
					   try the protocol again after this
					   many seconds. */
	TIME select_interval;		/* Wait this many seconds from the
					   first DHCPDISCOVER before
					   picking an offered lease. */
	TIME reboot_timeout;		/* When in INIT-REBOOT, wait this
					   long before giving up and going
					   to INIT. */
	TIME backoff_cutoff;		/* When doing exponential backoff,
					   never back off to an interval
					   longer than this amount. */
	u_int32_t requested_lease;	/* Requested lease time, if user
					   doesn't configure one. */
	struct string_list *media;	/* Possible network media values. */
	char *script_name;		/* Name of config script. */
	char *vendor_space_name;	/* Name of config script. */
	enum policy bootp_policy;
					/* Ignore, accept or prefer BOOTP
					   responses. */
	enum policy auth_policy;
					/* Require authentication, prefer
					   authentication, or don't try to
					   authenticate. */
	struct string_list *medium;	/* Current network medium. */

	struct iaddrlist *reject_list;	/* Servers to reject. */

	int omapi_port;			/* port on which to accept OMAPI
					   connections, or -1 for no
					   listener. */
	int do_forward_update;		/* If nonzero, and if we have the
					   information we need, update the
					   A record for the address we get. */
};

/* Per-interface state used in the dhcp client... */
struct client_state {
	struct client_state *next;
	struct interface_info *interface;
	char *name;

	struct client_lease *active;		  /* Currently active lease. */
	struct client_lease *new;			       /* New lease. */
	struct client_lease *offered_leases;	    /* Leases offered to us. */
	struct client_lease *leases;		/* Leases we currently hold. */
	struct client_lease *alias;			     /* Alias lease. */

	enum dhcp_state state;		/* Current state for this interface. */
	struct iaddr destination;		    /* Where to send packet. */
	u_int32_t xid;					  /* Transaction ID. */
	u_int16_t secs;			    /* secs value from DHCPDISCOVER. */
	TIME first_sending;			/* When was first copy sent? */
	TIME interval;		      /* What's the current resend interval? */
	int dns_update_timeout;		 /* Last timeout set for DNS update. */
	struct string_list *medium;		   /* Last media type tried. */
	struct dhcp_packet packet;		    /* Outgoing DHCP packet. */
	unsigned packet_length;	       /* Actual length of generated packet. */

	struct iaddr requested_address;	    /* Address we would like to get. */

	struct client_config *config;		    /* Client configuration. */
	struct string_list *env;	       /* Client script environment. */
	int envc;			/* Number of entries in environment. */

	struct option_state *sent_options;	/* Options we sent. */
};

/* Information about each network interface. */

struct interface_info {
	OMAPI_OBJECT_PREAMBLE;
	struct interface_info *next;	/* Next interface in list... */
	struct shared_network *shared_network;
				/* Networks connected to this interface. */
	struct hardware hw_address;	/* Its physical address. */
	struct in_addr primary_address;	/* Primary interface address. */

	u_int8_t *circuit_id;		/* Circuit ID associated with this
					   interface. */
	unsigned circuit_id_len;	/* Length of Circuit ID, if there
					   is one. */
	u_int8_t *remote_id;		/* Remote ID associated with this
					   interface (if any). */
	unsigned remote_id_len;		/* Length of Remote ID. */

	char name [IFNAMSIZ];		/* Its name... */
	int index;			/* Its index. */
	int rfdesc;			/* Its read file descriptor. */
	int wfdesc;			/* Its write file descriptor, if
					   different. */
	unsigned char *rbuf;		/* Read buffer, if required. */
	unsigned int rbuf_max;		/* Size of read buffer. */
	size_t rbuf_offset;		/* Current offset into buffer. */
	size_t rbuf_len;		/* Length of data in buffer. */

	struct ifreq *ifp;		/* Pointer to ifreq struct. */
	u_int32_t flags;		/* Control flags... */
#define INTERFACE_REQUESTED 1
#define INTERFACE_AUTOMATIC 2
#define INTERFACE_RUNNING 4

	/* Only used by DHCP client code. */
	struct client_state *client;
# if defined (USE_DLPI_SEND) || defined (USE_DLPI_RECEIVE)
	int dlpi_sap_length;
	struct hardware dlpi_broadcast_addr;
# endif /* DLPI_SEND || DLPI_RECEIVE */
};

struct hardware_link {
	struct hardware_link *next;
	char name [IFNAMSIZ];
	struct hardware address;
};

typedef void (*tvref_t)(void *, void *, const char *, int);
typedef void (*tvunref_t)(void *, const char *, int);
struct timeout {
	struct timeout *next;
	TIME when;
	void (*func) PROTO ((void *));
	void *what;
	tvref_t ref;
	tvunref_t unref;
};

struct protocol {
	struct protocol *next;
	int fd;
	void (*handler) PROTO ((struct protocol *));
	void *local;
};

struct dns_query; /* forward */

struct dns_wakeup {
	struct dns_wakeup *next;	/* Next wakeup in chain. */
	void (*func) PROTO ((struct dns_query *));
};

struct dns_question {
	u_int16_t type;			/* Type of query. */
	u_int16_t class;		/* Class of query. */
	unsigned char data [1];		/* Query data. */
};

struct dns_answer {
	u_int16_t type;			/* Type of answer. */
	u_int16_t class;		/* Class of answer. */
	int count;			/* Number of answers. */
	unsigned char *answers[1];	/* Pointers to answers. */
};

struct dns_query {
	struct dns_query *next;		/* Next query in hash bucket. */
	u_int32_t hash;			/* Hash bucket index. */
	TIME expiry;			/* Query expiry time (zero if not yet
					   answered. */
	u_int16_t id;			/* Query ID (also hash table index) */
	caddr_t waiters;		/* Pointer to list of things waiting
					   on this query. */

	struct dns_question *question;	/* Question, internal format. */
	struct dns_answer *answer;	/* Answer, internal format. */

	unsigned char *query;		/* Query formatted for DNS server. */
	unsigned len;			/* Length of entire query. */
	int sent;			/* The query has been sent. */
	struct dns_wakeup *wakeups;	/* Wakeups to call if this query is
					   answered. */
	struct name_server *next_server;	/* Next server to try. */
	int backoff;			/* Current backoff, in seconds. */
};

struct dns_zone {
	int refcnt;
	TIME timeout;
	char *name;
	struct option_cache *primary;
	struct option_cache *secondary;
	struct auth_key *key;
};

struct icmp_state {
	OMAPI_OBJECT_PREAMBLE;
	int socket;
	void (*icmp_handler) PROTO ((struct iaddr, u_int8_t *, int));
};

#include "ctrace.h"

/* Bitmask of dhcp option codes. */
typedef unsigned char option_mask [16];

/* DHCP Option mask manipulation macros... */
#define OPTION_ZERO(mask)	(memset (mask, 0, 16))
#define OPTION_SET(mask, bit)	(mask [bit >> 8] |= (1 << (bit & 7)))
#define OPTION_CLR(mask, bit)	(mask [bit >> 8] &= ~(1 << (bit & 7)))
#define OPTION_ISSET(mask, bit)	(mask [bit >> 8] & (1 << (bit & 7)))
#define OPTION_ISCLR(mask, bit)	(!OPTION_ISSET (mask, bit))

/* An option occupies its length plus two header bytes (code and
    length) for every 255 bytes that must be stored. */
#define OPTION_SPACE(x)		((x) + 2 * ((x) / 255 + 1))

/* Default path to dhcpd config file. */
#ifdef DEBUG
#undef _PATH_DHCPD_CONF
#define _PATH_DHCPD_CONF	"dhcpd.conf"
#undef _PATH_DHCPD_DB
#define _PATH_DHCPD_DB		"dhcpd.leases"
#undef _PATH_DHCPD_PID
#define _PATH_DHCPD_PID		"dhcpd.pid"
#else
#ifndef _PATH_DHCPD_CONF
#define _PATH_DHCPD_CONF	"/etc/dhcpd.conf"
#endif

#ifndef _PATH_DHCPD_DB
#define _PATH_DHCPD_DB		"/etc/dhcpd.leases"
#endif

#ifndef _PATH_DHCPD_PID
#define _PATH_DHCPD_PID		"/var/run/dhcpd.pid"
#endif
#endif

#ifndef _PATH_DHCLIENT_CONF
#define _PATH_DHCLIENT_CONF	"/etc/dhclient.conf"
#endif

#ifndef _PATH_DHCLIENT_SCRIPT
#define _PATH_DHCLIENT_SCRIPT	"/sbin/dhclient-script"
#endif

#ifndef _PATH_DHCLIENT_PID
#define _PATH_DHCLIENT_PID	"/var/run/dhclient.pid"
#endif

#ifndef _PATH_DHCLIENT_DB
#define _PATH_DHCLIENT_DB	"/etc/dhclient.leases"
#endif

#ifndef _PATH_RESOLV_CONF
#define _PATH_RESOLV_CONF	"/etc/resolv.conf"
#endif

#ifndef _PATH_DHCRELAY_PID
#define _PATH_DHCRELAY_PID	"/var/run/dhcrelay.pid"
#endif

#ifndef DHCPD_LOG_FACILITY
#define DHCPD_LOG_FACILITY	LOG_DAEMON
#endif

#define MAX_TIME 0x7fffffff
#define MIN_TIME 0

/* External definitions... */

HASH_FUNCTIONS_DECL (group, const char *, struct group_object, group_hash_t)
HASH_FUNCTIONS_DECL (universe, const char *, struct universe, universe_hash_t)
HASH_FUNCTIONS_DECL (option, const char *, struct option, option_hash_t)
HASH_FUNCTIONS_DECL (dns_zone, const char *, struct dns_zone, dns_zone_hash_t)
HASH_FUNCTIONS_DECL (lease, const unsigned char *, struct lease, lease_hash_t)
HASH_FUNCTIONS_DECL (host, const unsigned char *, struct host_decl, host_hash_t)
HASH_FUNCTIONS_DECL (class, const char *, struct class, class_hash_t)

/* options.c */

extern struct option *vendor_cfg_option;
extern int parse_options PROTO ((struct packet *));
extern int parse_option_buffer PROTO ((struct option_state *, const unsigned char *,
				unsigned, struct universe *));
extern struct universe *find_option_universe (struct option *, const char *);
extern int parse_encapsulated_suboptions (struct option_state *, struct option *,
				   const unsigned char *, unsigned,
				   struct universe *, const char *);
extern int cons_options PROTO ((struct packet *, struct dhcp_packet *, struct lease *,
			 struct client_state *,
			 int, struct option_state *, struct option_state *,
			 struct binding_scope **,
			 int, int, int, struct data_string *, const char *));
extern int fqdn_universe_decode (struct option_state *,
			  const unsigned char *, unsigned, struct universe *);
extern int store_options PROTO ((int *, unsigned char *, unsigned, struct packet *,
			  struct lease *, struct client_state *,
			  struct option_state *,
			  struct option_state *, struct binding_scope **,
			  unsigned *, int, unsigned, unsigned,
			  int, const char *));
extern const char *pretty_print_option PROTO ((struct option *, const unsigned char *,
					unsigned, int, int));
extern int get_option (struct data_string *, struct universe *,
		struct packet *, struct lease *, struct client_state *,
		struct option_state *, struct option_state *,
		struct option_state *, struct binding_scope **, unsigned,
		const char *, int);
extern void set_option (struct universe *, struct option_state *,
		 struct option_cache *, enum statement_op);
extern struct option_cache *lookup_option PROTO ((struct universe *,
					   struct option_state *, unsigned));
extern struct option_cache *lookup_hashed_option PROTO ((struct universe *,
						  struct option_state *,
						  unsigned));
extern int save_option_buffer (struct universe *, struct option_state *,
			struct buffer *, unsigned char *, unsigned,
			struct option *, int);
extern void save_option PROTO ((struct universe *,
			 struct option_state *, struct option_cache *));
extern void save_hashed_option PROTO ((struct universe *,
				struct option_state *, struct option_cache *));
extern void delete_option PROTO ((struct universe *, struct option_state *, int));
extern void delete_hashed_option PROTO ((struct universe *,
				  struct option_state *, int));
extern int option_cache_dereference PROTO ((struct option_cache **,
				     const char *, int));
extern int hashed_option_state_dereference PROTO ((struct universe *,
					    struct option_state *,
					    const char *, int));
extern int store_option PROTO ((struct data_string *,
			 struct universe *, struct packet *, struct lease *,
			 struct client_state *,
			 struct option_state *, struct option_state *,
			 struct binding_scope **, struct option_cache *));
extern int option_space_encapsulate PROTO ((struct data_string *,
				     struct packet *, struct lease *,
				     struct client_state *,
				     struct option_state *,
				     struct option_state *,
				     struct binding_scope **,
				     struct data_string *));
extern int hashed_option_space_encapsulate PROTO ((struct data_string *,
					    struct packet *, struct lease *,
					    struct client_state *,
					    struct option_state *,
					    struct option_state *,
					    struct binding_scope **,
					    struct universe *));
extern int nwip_option_space_encapsulate PROTO ((struct data_string *,
					  struct packet *, struct lease *,
					  struct client_state *,
					  struct option_state *,
					  struct option_state *,
					  struct binding_scope **,
					  struct universe *));
extern int fqdn_option_space_encapsulate (struct data_string *,
				   struct packet *, struct lease *,
				   struct client_state *,
				   struct option_state *,
				   struct option_state *,
				   struct binding_scope **,
				   struct universe *);
extern void suboption_foreach (struct packet *, struct lease *, struct client_state *,
			struct option_state *, struct option_state *,
			struct binding_scope **, struct universe *, void *,
			void (*) (struct option_cache *, struct packet *,
				  struct lease *, struct client_state *,
				  struct option_state *, struct option_state *,
				  struct binding_scope **,
				  struct universe *, void *),
			struct option_cache *, const char *);
extern void option_space_foreach (struct packet *, struct lease *,
			   struct client_state *,
			   struct option_state *,
			   struct option_state *,
			   struct binding_scope **,
			   struct universe *, void *,
			   void (*) (struct option_cache *,
				     struct packet *,
				     struct lease *, struct client_state *,
				     struct option_state *,
				     struct option_state *,
				     struct binding_scope **,
				     struct universe *, void *));
extern void hashed_option_space_foreach (struct packet *, struct lease *,
				  struct client_state *,
				  struct option_state *,
				  struct option_state *,
				  struct binding_scope **,
				  struct universe *, void *,
				  void (*) (struct option_cache *,
					    struct packet *,
					    struct lease *,
					    struct client_state *,
					    struct option_state *,
					    struct option_state *,
					    struct binding_scope **,
					    struct universe *, void *));
extern int linked_option_get PROTO ((struct data_string *, struct universe *,
			      struct packet *, struct lease *,
			      struct client_state *,
			      struct option_state *, struct option_state *,
			      struct option_state *, struct binding_scope **,
			      unsigned));
extern int linked_option_state_dereference PROTO ((struct universe *,
					    struct option_state *,
					    const char *, int));
extern void save_linked_option (struct universe *, struct option_state *,
			 struct option_cache *);
extern void linked_option_space_foreach (struct packet *, struct lease *,
				  struct client_state *,
				  struct option_state *,
				  struct option_state *,
				  struct binding_scope **,
				  struct universe *, void *,
				  void (*) (struct option_cache *,
					    struct packet *,
					    struct lease *,
					    struct client_state *,
					    struct option_state *,
					    struct option_state *,
					    struct binding_scope **,
					    struct universe *, void *));
extern int linked_option_space_encapsulate (struct data_string *, struct packet *,
				     struct lease *, struct client_state *,
				     struct option_state *,
				     struct option_state *,
				     struct binding_scope **,
				     struct universe *);
extern void delete_linked_option (struct universe *, struct option_state *, int);
extern struct option_cache *lookup_linked_option (struct universe *,
					   struct option_state *, unsigned);
extern void do_packet PROTO ((struct interface_info *,
		       struct dhcp_packet *, unsigned,
		       unsigned int, struct iaddr, struct hardware *));

/* dhcpd.c */
extern TIME cur_time;

extern int ddns_update_style;

extern const char *path_dhcpd_conf;
extern const char *path_dhcpd_db;
extern const char *path_dhcpd_pid;

extern int dhcp_max_agent_option_packet_length;

extern int main PROTO ((int, char **, char **));
extern void postconf_initialization (int);
extern void postdb_startup (void);
extern void cleanup PROTO ((void));
extern void lease_pinged PROTO ((struct iaddr, u_int8_t *, int));
extern void lease_ping_timeout PROTO ((void *));
extern int dhcpd_interface_setup_hook (struct interface_info *ip, struct iaddr *ia);
extern enum dhcp_shutdown_state shutdown_state;
extern isc_result_t dhcp_io_shutdown (omapi_object_t *, void *);
extern isc_result_t dhcp_set_control_state (control_object_state_t oldstate,
				     control_object_state_t newstate);

/* conflex.c */
extern isc_result_t new_parse PROTO ((struct parse **, int,
			       char *, unsigned, const char *, int));
extern isc_result_t end_parse PROTO ((struct parse **));
extern enum dhcp_token next_token PROTO ((const char **, unsigned *, struct parse *));
extern enum dhcp_token peek_token PROTO ((const char **, unsigned *, struct parse *));

/* confpars.c */
extern void parse_trace_setup (void);
extern isc_result_t readconf PROTO ((void));
extern isc_result_t read_conf_file (const char *, struct group *, int, int);
#if defined (TRACING)
extern void trace_conf_input (trace_type_t *, unsigned, char *);
extern void trace_conf_stop (trace_type_t *ttype);
#endif
extern isc_result_t conf_file_subparse (struct parse *, struct group *, int);
extern isc_result_t lease_file_subparse (struct parse *);
extern int parse_statement PROTO ((struct parse *,
			    struct group *, int, struct host_decl *, int));
#if defined (FAILOVER_PROTOCOL)
extern void parse_failover_peer PROTO ((struct parse *, struct group *, int));
extern void parse_failover_state_declaration (struct parse *,
				       dhcp_failover_state_t *);
extern void parse_failover_state PROTO ((struct parse *,
				  enum failover_state *, TIME *));
#endif
extern int permit_list_match (struct permit *, struct permit *);
extern void parse_pool_statement PROTO ((struct parse *, struct group *, int));
extern int parse_boolean PROTO ((struct parse *));
extern int parse_lbrace PROTO ((struct parse *));
extern void parse_host_declaration PROTO ((struct parse *, struct group *));
extern int parse_class_declaration PROTO ((struct class **, struct parse *,
				    struct group *, int));
extern void parse_shared_net_declaration PROTO ((struct parse *, struct group *));
extern void parse_subnet_declaration PROTO ((struct parse *,
				      struct shared_network *));
extern void parse_group_declaration PROTO ((struct parse *, struct group *));
extern int parse_fixed_addr_param PROTO ((struct option_cache **, struct parse *));
extern TIME parse_timestamp PROTO ((struct parse *));
extern int parse_lease_declaration PROTO ((struct lease **, struct parse *));
extern void parse_address_range PROTO ((struct parse *, struct group *, int,
				 struct pool *, struct lease **));

/* ddns.c */
extern int ddns_updates PROTO ((struct packet *, struct lease *, struct lease *,
			 struct lease_state *));
extern int ddns_removals PROTO ((struct lease *));

/* parse.c */
extern void add_enumeration (struct enumeration *);
extern struct enumeration *find_enumeration (const char *, int);
extern struct enumeration_value *find_enumeration_value (const char *, int,
						  const char *);
extern void skip_to_semi PROTO ((struct parse *));
extern void skip_to_rbrace PROTO ((struct parse *, int));
extern int parse_semi PROTO ((struct parse *));
extern int parse_string PROTO ((struct parse *, char **, unsigned *));
extern char *parse_host_name PROTO ((struct parse *));
extern int parse_ip_addr_or_hostname PROTO ((struct expression **,
				      struct parse *, int));
extern void parse_hardware_param PROTO ((struct parse *, struct hardware *));
extern void parse_lease_time PROTO ((struct parse *, TIME *));
extern unsigned char *parse_numeric_aggregate PROTO ((struct parse *,
					       unsigned char *, unsigned *,
					       int, int, unsigned));
extern void convert_num PROTO ((struct parse *, unsigned char *, const char *,
			 int, unsigned));
extern TIME parse_date PROTO ((struct parse *));
extern struct option *parse_option_name PROTO ((struct parse *, int, int *));
extern void parse_option_space_decl PROTO ((struct parse *));
extern int parse_option_code_definition PROTO ((struct parse *, struct option *));
extern int parse_base64 (struct data_string *, struct parse *);
extern int parse_cshl PROTO ((struct data_string *, struct parse *));
extern int parse_executable_statement PROTO ((struct executable_statement **,
				       struct parse *, int *,
				       enum expression_context));
extern int parse_executable_statements PROTO ((struct executable_statement **,
					struct parse *, int *,
					enum expression_context));
extern int parse_zone (struct dns_zone *, struct parse *);
extern int parse_key (struct parse *);
extern int parse_on_statement PROTO ((struct executable_statement **,
			       struct parse *, int *));
extern int parse_switch_statement PROTO ((struct executable_statement **,
				   struct parse *, int *));
extern int parse_case_statement PROTO ((struct executable_statement **,
				 struct parse *, int *,
				 enum expression_context));
extern int parse_if_statement PROTO ((struct executable_statement **,
			       struct parse *, int *));
extern int parse_boolean_expression PROTO ((struct expression **,
				     struct parse *, int *));
extern int parse_data_expression PROTO ((struct expression **,
				  struct parse *, int *));
extern int parse_numeric_expression PROTO ((struct expression **,
				     struct parse *, int *));
extern int parse_dns_expression PROTO ((struct expression **, struct parse *, int *));
extern int parse_non_binary PROTO ((struct expression **, struct parse *, int *,
			     enum expression_context));
extern int parse_expression PROTO ((struct expression **, struct parse *, int *,
			     enum expression_context,
			     struct expression **, enum expr_op));
extern int parse_option_statement PROTO ((struct executable_statement **,
				   struct parse *, int,
				   struct option *, enum statement_op));
extern int parse_option_token PROTO ((struct expression **, struct parse *,
			       const char **, struct expression *, int, int));
extern int parse_allow_deny PROTO ((struct option_cache **, struct parse *, int));
extern int parse_auth_key PROTO ((struct data_string *, struct parse *));
extern int parse_warn (struct parse *, const char *, ...)
	__attribute__((__format__(__printf__,2,3)));

/* tree.c */
#if defined (NSUPDATE)
extern struct __res_state resolver_state;
extern int resolver_inited;
#endif

extern struct binding_scope *global_scope;
extern pair cons PROTO ((caddr_t, pair));
extern int make_const_option_cache PROTO ((struct option_cache **, struct buffer **,
				    u_int8_t *, unsigned, struct option *,
				    const char *, int));
extern int make_host_lookup PROTO ((struct expression **, const char *));
extern int enter_dns_host PROTO ((struct dns_host_entry **, const char *));
extern int make_const_data (struct expression **,
		     const unsigned char *, unsigned, int, int,
		     const char *, int);
extern int make_const_int PROTO ((struct expression **, unsigned long));
extern int make_concat PROTO ((struct expression **,
			struct expression *, struct expression *));
extern int make_encapsulation PROTO ((struct expression **, struct data_string *));
extern int make_substring PROTO ((struct expression **, struct expression *,
			   struct expression *, struct expression *));
extern int make_limit PROTO ((struct expression **, struct expression *, int));
extern int make_let PROTO ((struct executable_statement **, const char *));
extern int option_cache PROTO ((struct option_cache **, struct data_string *,
			 struct expression *, struct option *,
			 const char *, int));
extern int evaluate_expression (struct binding_value **, struct packet *,
			 struct lease *, struct client_state *,
			 struct option_state *, struct option_state *,
			 struct binding_scope **, struct expression *,
			 const char *, int);
extern int binding_value_dereference (struct binding_value **, const char *, int);
#if defined (NSUPDATE)
extern int evaluate_dns_expression PROTO ((ns_updrec **, struct packet *,
				    struct lease *, 
				    struct client_state *,
				    struct option_state *,
				    struct option_state *,
				    struct binding_scope **,
				    struct expression *));
#endif
extern int evaluate_boolean_expression PROTO ((int *,
					struct packet *,  struct lease *,
					struct client_state *,
					struct option_state *,
					struct option_state *,
					struct binding_scope **,
					struct expression *));
extern int evaluate_data_expression PROTO ((struct data_string *,
				     struct packet *, struct lease *,
				     struct client_state *,
				     struct option_state *,
				     struct option_state *,
				     struct binding_scope **,
				     struct expression *, const char *, int));
extern int evaluate_numeric_expression (unsigned long *, struct packet *,
				 struct lease *, struct client_state *,
				 struct option_state *, struct option_state *,
				 struct binding_scope **,
				 struct expression *);
extern int evaluate_option_cache PROTO ((struct data_string *,
				  struct packet *, struct lease *,
				  struct client_state *,
				  struct option_state *, struct option_state *,
				  struct binding_scope **,
				  struct option_cache *,
				  const char *, int));
extern int evaluate_boolean_option_cache PROTO ((int *,
					  struct packet *, struct lease *,
					  struct client_state *,
					  struct option_state *,
					  struct option_state *,
					  struct binding_scope **,
					  struct option_cache *,
					  const char *, int));
extern int evaluate_boolean_expression_result PROTO ((int *,
					       struct packet *, struct lease *,
					       struct client_state *,
					       struct option_state *,
					       struct option_state *,
					       struct binding_scope **,
					       struct expression *));
extern void expression_dereference PROTO ((struct expression **, const char *, int));
extern int is_dns_expression PROTO ((struct expression *));
extern int is_boolean_expression PROTO ((struct expression *));
extern int is_data_expression PROTO ((struct expression *));
extern int is_numeric_expression PROTO ((struct expression *));
extern int is_compound_expression PROTO ((struct expression *));
extern int op_precedence PROTO ((enum expr_op, enum expr_op));
extern enum expression_context expression_context (struct expression *);
extern enum expression_context op_context PROTO ((enum expr_op));
extern int write_expression PROTO ((FILE *, struct expression *, int, int, int));
extern struct binding *find_binding PROTO ((struct binding_scope *, const char *));
extern int free_bindings PROTO ((struct binding_scope *, const char *, int));
extern int binding_scope_dereference PROTO ((struct binding_scope **,
				      const char *, int));
extern int fundef_dereference (struct fundef **, const char *, int);
extern int data_subexpression_length (int *, struct expression *);
extern int expr_valid_for_context (struct expression *, enum expression_context);
extern struct binding *create_binding (struct binding_scope **, const char *);
extern int bind_ds_value (struct binding_scope **,
		   const char *, struct data_string *);
extern int find_bound_string (struct data_string *,
		       struct binding_scope *, const char *);
extern int unset (struct binding_scope *, const char *);

/* dhcp.c */
extern int outstanding_pings;

extern void dhcp PROTO ((struct packet *));
extern void dhcpdiscover PROTO ((struct packet *, int));
extern void dhcprequest PROTO ((struct packet *, int, struct lease *));
extern void dhcprelease PROTO ((struct packet *, int));
extern void dhcpdecline PROTO ((struct packet *, int));
extern void dhcpinform PROTO ((struct packet *, int));
extern void nak_lease PROTO ((struct packet *, struct iaddr *cip));
extern void ack_lease PROTO ((struct packet *, struct lease *,
		       unsigned int, TIME, char *, int, struct host_decl *));
extern void dhcp_reply PROTO ((struct lease *));
extern int find_lease PROTO ((struct lease **, struct packet *,
		       struct shared_network *, int *, int *, struct lease *,
		       const char *, int));
extern int mockup_lease PROTO ((struct lease **, struct packet *,
			 struct shared_network *,
			 struct host_decl *));
extern void static_lease_dereference PROTO ((struct lease *, const char *, int));

extern int allocate_lease PROTO ((struct lease **, struct packet *,
			   struct pool *, int *));
extern int permitted PROTO ((struct packet *, struct permit *));
extern int locate_network PROTO ((struct packet *));
extern int parse_agent_information_option PROTO ((struct packet *, int, u_int8_t *));
extern unsigned cons_agent_information_options PROTO ((struct option_state *,
						struct dhcp_packet *,
						unsigned, unsigned));

/* bootp.c */
extern void bootp PROTO ((struct packet *));

/* memory.c */
extern struct group *root_group;
extern group_hash_t *group_name_hash;

extern int (*group_write_hook) (struct group_object *);
extern isc_result_t delete_group (struct group_object *, int);
extern isc_result_t supersede_group (struct group_object *, int);
extern int clone_group (struct group **, struct group *, const char *, int);
extern int write_group PROTO ((struct group_object *));

/* salloc.c */
extern void relinquish_lease_hunks (void);
extern struct lease *new_leases PROTO ((unsigned, const char *, int));
#if defined (DEBUG_MEMORY_LEAKAGE) || \
		defined (DEBUG_MEMORY_LEAKAGE_ON_EXIT)
extern void relinquish_free_lease_states (void);
#endif
OMAPI_OBJECT_ALLOC_DECL (lease, struct lease, dhcp_type_lease)
OMAPI_OBJECT_ALLOC_DECL (class, struct class, dhcp_type_class)
OMAPI_OBJECT_ALLOC_DECL (pool, struct pool, dhcp_type_pool)
OMAPI_OBJECT_ALLOC_DECL (host, struct host_decl, dhcp_type_host)

/* alloc.c */
OMAPI_OBJECT_ALLOC_DECL (subnet, struct subnet, dhcp_type_subnet)
OMAPI_OBJECT_ALLOC_DECL (shared_network, struct shared_network,
			 dhcp_type_shared_network)
OMAPI_OBJECT_ALLOC_DECL (group_object, struct group_object, dhcp_type_group)
OMAPI_OBJECT_ALLOC_DECL (dhcp_control,
			 dhcp_control_object_t, dhcp_type_control)

#if defined (DEBUG_MEMORY_LEAKAGE) || \
		defined (DEBUG_MEMORY_LEAKAGE_ON_EXIT)
extern void relinquish_free_pairs (void);
extern void relinquish_free_expressions (void);
extern void relinquish_free_binding_values (void);
extern void relinquish_free_option_caches (void);
extern void relinquish_free_packets (void);
#endif

extern int option_chain_head_allocate (struct option_chain_head **,
				const char *, int);
extern int option_chain_head_reference (struct option_chain_head **,
				 struct option_chain_head *,
				 const char *, int);
extern int option_chain_head_dereference (struct option_chain_head **,
				   const char *, int);
extern int group_allocate (struct group **, const char *, int);
extern int group_reference (struct group **, struct group *, const char *, int);
extern int group_dereference (struct group **, const char *, int);
extern struct dhcp_packet *new_dhcp_packet PROTO ((const char *, int));
extern struct protocol *new_protocol PROTO ((const char *, int));
extern struct lease_state *new_lease_state PROTO ((const char *, int));
extern struct domain_search_list *new_domain_search_list PROTO ((const char *, int));
extern struct name_server *new_name_server PROTO ((const char *, int));
extern void free_name_server PROTO ((struct name_server *, const char *, int));
extern struct option *new_option PROTO ((const char *, int));
extern int group_allocate (struct group **, const char *, int);
extern int group_reference (struct group **, struct group *, const char *, int);
extern int group_dereference (struct group **, const char *, int);
extern void free_option PROTO ((struct option *, const char *, int));
extern struct universe *new_universe PROTO ((const char *, int));
extern void free_universe PROTO ((struct universe *, const char *, int));
extern void free_domain_search_list PROTO ((struct domain_search_list *,
				     const char *, int));
extern void free_lease_state PROTO ((struct lease_state *, const char *, int));
extern void free_protocol PROTO ((struct protocol *, const char *, int));
extern void free_dhcp_packet PROTO ((struct dhcp_packet *, const char *, int));
extern struct client_lease *new_client_lease PROTO ((const char *, int));
extern void free_client_lease PROTO ((struct client_lease *, const char *, int));
extern struct permit *new_permit PROTO ((const char *, int));
extern void free_permit PROTO ((struct permit *, const char *, int));
extern pair new_pair PROTO ((const char *, int));
extern void free_pair PROTO ((pair, const char *, int));
extern int expression_allocate PROTO ((struct expression **, const char *, int));
extern int expression_reference PROTO ((struct expression **,
				 struct expression *, const char *, int));
extern void free_expression PROTO ((struct expression *, const char *, int));
extern int binding_value_allocate PROTO ((struct binding_value **,
				   const char *, int));
extern int binding_value_reference PROTO ((struct binding_value **,
				    struct binding_value *,
				    const char *, int));
extern void free_binding_value PROTO ((struct binding_value *, const char *, int));
extern int fundef_allocate PROTO ((struct fundef **, const char *, int));
extern int fundef_reference PROTO ((struct fundef **,
			     struct fundef *, const char *, int));
extern int option_cache_allocate PROTO ((struct option_cache **, const char *, int));
extern int option_cache_reference PROTO ((struct option_cache **,
				   struct option_cache *, const char *, int));
extern int buffer_allocate PROTO ((struct buffer **, unsigned, const char *, int));
extern int buffer_reference PROTO ((struct buffer **, struct buffer *,
			     const char *, int));
extern int buffer_dereference PROTO ((struct buffer **, const char *, int));
extern int dns_host_entry_allocate PROTO ((struct dns_host_entry **,
				    const char *, const char *, int));
extern int dns_host_entry_reference PROTO ((struct dns_host_entry **,
				     struct dns_host_entry *,
				     const char *, int));
extern int dns_host_entry_dereference PROTO ((struct dns_host_entry **,
				       const char *, int));
extern int option_state_allocate PROTO ((struct option_state **, const char *, int));
extern int option_state_reference PROTO ((struct option_state **,
				   struct option_state *, const char *, int));
extern int option_state_dereference PROTO ((struct option_state **,
				     const char *, int));
extern void data_string_copy PROTO ((struct data_string *,
			      struct data_string *, const char *, int));
extern void data_string_forget PROTO ((struct data_string *, const char *, int));
extern void data_string_truncate PROTO ((struct data_string *, int));
extern int executable_statement_allocate PROTO ((struct executable_statement **,
					  const char *, int));
extern int executable_statement_reference PROTO ((struct executable_statement **,
					   struct executable_statement *,
					   const char *, int));
extern int packet_allocate PROTO ((struct packet **, const char *, int));
extern int packet_reference PROTO ((struct packet **,
			     struct packet *, const char *, int));
extern int packet_dereference PROTO ((struct packet **, const char *, int));
extern int binding_scope_allocate PROTO ((struct binding_scope **,
				   const char *, int));
extern int binding_scope_reference PROTO ((struct binding_scope **,
				    struct binding_scope *,
				    const char *, int));
extern int dns_zone_allocate PROTO ((struct dns_zone **, const char *, int));
extern int dns_zone_reference PROTO ((struct dns_zone **,
			       struct dns_zone *, const char *, int));

/* print.c */
extern char *quotify_string (const char *, const char *, int);
extern char *quotify_buf (const unsigned char *, unsigned, const char *, int);
extern char *print_base64 (const unsigned char *, unsigned, const char *, int);
extern char *print_hw_addr PROTO ((int, int, unsigned char *));
extern void print_lease PROTO ((struct lease *));
extern void dump_raw PROTO ((const unsigned char *, unsigned));
extern void dump_packet_option (struct option_cache *, struct packet *,
			 struct lease *, struct client_state *,
			 struct option_state *, struct option_state *,
			 struct binding_scope **, struct universe *, void *);
extern void dump_packet PROTO ((struct packet *));
extern void hash_dump PROTO ((struct hash_table *));
extern char *print_hex_1 PROTO ((unsigned, const u_int8_t *, unsigned));
extern char *print_hex_2 PROTO ((unsigned, const u_int8_t *, unsigned));
extern char *print_hex_3 PROTO ((unsigned, const u_int8_t *, unsigned));
extern char *print_dotted_quads PROTO ((unsigned, const u_int8_t *));
extern char *print_dec_1 PROTO ((unsigned long));
extern char *print_dec_2 PROTO ((unsigned long));
extern void print_expression PROTO ((const char *, struct expression *));
extern int token_print_indent_concat (FILE *, int, int,
			       const char *, const char *, ...);
extern int token_indent_data_string (FILE *, int, int, const char *, const char *,
			      struct data_string *);
extern int token_print_indent (FILE *, int, int,
			const char *, const char *, const char *);
extern void indent_spaces (FILE *, int);
#if defined (NSUPDATE)
extern void print_dns_status (int, ns_updque *);
#endif

/* socket.c */
#if defined (USE_SOCKET_SEND) || defined (USE_SOCKET_RECEIVE) \
	|| defined (USE_SOCKET_FALLBACK)
extern int if_register_socket PROTO ((struct interface_info *));
#endif

#if defined (USE_SOCKET_FALLBACK) && !defined (USE_SOCKET_SEND)
extern void if_reinitialize_fallback PROTO ((struct interface_info *));
extern void if_register_fallback PROTO ((struct interface_info *));
extern ssize_t send_fallback PROTO ((struct interface_info *,
			      struct packet *, struct dhcp_packet *, size_t, 
			      struct in_addr,
			      struct sockaddr_in *, struct hardware *));
#endif

#ifdef USE_SOCKET_SEND
extern void if_reinitialize_send PROTO ((struct interface_info *));
extern void if_register_send PROTO ((struct interface_info *));
extern void if_deregister_send PROTO ((struct interface_info *));
extern ssize_t send_packet PROTO ((struct interface_info *,
			    struct packet *, struct dhcp_packet *, size_t, 
			    struct in_addr,
			    struct sockaddr_in *, struct hardware *));
#endif
#ifdef USE_SOCKET_RECEIVE
extern void if_reinitialize_receive PROTO ((struct interface_info *));
extern void if_register_receive PROTO ((struct interface_info *));
extern void if_deregister_receive PROTO ((struct interface_info *));
extern ssize_t receive_packet PROTO ((struct interface_info *,
			       unsigned char *, size_t,
			       struct sockaddr_in *, struct hardware *));
#endif

#if defined (USE_SOCKET_FALLBACK)
extern isc_result_t fallback_discard PROTO ((omapi_object_t *));
#endif

#if defined (USE_SOCKET_SEND)
extern int can_unicast_without_arp PROTO ((struct interface_info *));
extern int can_receive_unicast_unconfigured PROTO ((struct interface_info *));
extern int supports_multiple_interfaces (struct interface_info *);
extern void maybe_setup_fallback PROTO ((void));
#endif

/* bpf.c */
#if defined (USE_BPF_SEND) || defined (USE_BPF_RECEIVE)
extern int if_register_bpf PROTO ( (struct interface_info *));
#endif
#ifdef USE_BPF_SEND
extern void if_reinitialize_send PROTO ((struct interface_info *));
extern void if_register_send PROTO ((struct interface_info *));
extern void if_deregister_send PROTO ((struct interface_info *));
extern ssize_t send_packet PROTO ((struct interface_info *,
			    struct packet *, struct dhcp_packet *, size_t,
			    struct in_addr,
			    struct sockaddr_in *, struct hardware *));
#endif
#ifdef USE_BPF_RECEIVE
extern void if_reinitialize_receive PROTO ((struct interface_info *));
extern void if_register_receive PROTO ((struct interface_info *));
extern void if_deregister_receive PROTO ((struct interface_info *));
extern ssize_t receive_packet PROTO ((struct interface_info *,
			       unsigned char *, size_t,
			       struct sockaddr_in *, struct hardware *));
#endif
#if defined (USE_BPF_SEND)
extern int can_unicast_without_arp PROTO ((struct interface_info *));
extern int can_receive_unicast_unconfigured PROTO ((struct interface_info *));
extern int supports_multiple_interfaces (struct interface_info *);
extern void maybe_setup_fallback PROTO ((void));
#endif

/* lpf.c */
#if defined (USE_LPF_SEND) || defined (USE_LPF_RECEIVE)
extern int if_register_lpf PROTO ( (struct interface_info *));
#endif
#ifdef USE_LPF_SEND
extern void if_reinitialize_send PROTO ((struct interface_info *));
extern void if_register_send PROTO ((struct interface_info *));
extern void if_deregister_send PROTO ((struct interface_info *));
extern ssize_t send_packet PROTO ((struct interface_info *,
			    struct packet *, struct dhcp_packet *, size_t,
			    struct in_addr,
			    struct sockaddr_in *, struct hardware *));
#endif
#ifdef USE_LPF_RECEIVE
extern void if_reinitialize_receive PROTO ((struct interface_info *));
extern void if_register_receive PROTO ((struct interface_info *));
extern void if_deregister_receive PROTO ((struct interface_info *));
extern ssize_t receive_packet PROTO ((struct interface_info *,
			       unsigned char *, size_t,
			       struct sockaddr_in *, struct hardware *));
#endif
#if defined (USE_LPF_SEND)
extern int can_unicast_without_arp PROTO ((struct interface_info *));
extern int can_receive_unicast_unconfigured PROTO ((struct interface_info *));
extern int supports_multiple_interfaces (struct interface_info *);
extern void maybe_setup_fallback PROTO ((void));
#endif

/* nit.c */
#if defined (USE_NIT_SEND) || defined (USE_NIT_RECEIVE)
extern int if_register_nit PROTO ( (struct interface_info *));
#endif

#ifdef USE_NIT_SEND
extern void if_reinitialize_send PROTO ((struct interface_info *));
extern void if_register_send PROTO ((struct interface_info *));
extern void if_deregister_send PROTO ((struct interface_info *));
extern ssize_t send_packet PROTO ((struct interface_info *,
			    struct packet *, struct dhcp_packet *, size_t,
			    struct in_addr,
			    struct sockaddr_in *, struct hardware *));
#endif
#ifdef USE_NIT_RECEIVE
extern void if_reinitialize_receive PROTO ((struct interface_info *));
extern void if_register_receive PROTO ((struct interface_info *));
extern void if_deregister_receive PROTO ((struct interface_info *));
extern ssize_t receive_packet PROTO ((struct interface_info *,
			       unsigned char *, size_t,
			       struct sockaddr_in *, struct hardware *));
#endif
#if defined (USE_NIT_SEND)
extern int can_unicast_without_arp PROTO ((struct interface_info *));
extern int can_receive_unicast_unconfigured PROTO ((struct interface_info *));
extern int supports_multiple_interfaces (struct interface_info *);
extern void maybe_setup_fallback PROTO ((void));
#endif

/* dlpi.c */
#if defined (USE_DLPI_SEND) || defined (USE_DLPI_RECEIVE)
extern int if_register_dlpi PROTO ( (struct interface_info *));
#endif

#ifdef USE_DLPI_SEND
extern void if_reinitialize_send PROTO ((struct interface_info *));
extern void if_register_send PROTO ((struct interface_info *));
extern void if_deregister_send PROTO ((struct interface_info *));
extern ssize_t send_packet PROTO ((struct interface_info *,
			    struct packet *, struct dhcp_packet *, size_t,
			    struct in_addr,
			    struct sockaddr_in *, struct hardware *));
#endif
#ifdef USE_DLPI_RECEIVE
extern void if_reinitialize_receive PROTO ((struct interface_info *));
extern void if_register_receive PROTO ((struct interface_info *));
extern void if_deregister_receive PROTO ((struct interface_info *));
extern ssize_t receive_packet PROTO ((struct interface_info *,
			       unsigned char *, size_t,
			       struct sockaddr_in *, struct hardware *));
#endif


/* raw.c */
#ifdef USE_RAW_SEND
extern void if_reinitialize_send PROTO ((struct interface_info *));
extern void if_register_send PROTO ((struct interface_info *));
extern void if_deregister_send PROTO ((struct interface_info *));
extern ssize_t send_packet PROTO ((struct interface_info *,
			    struct packet *, struct dhcp_packet *, size_t,
			    struct in_addr,
			    struct sockaddr_in *, struct hardware *));
extern int can_unicast_without_arp PROTO ((struct interface_info *));
extern int can_receive_unicast_unconfigured PROTO ((struct interface_info *));
extern int supports_multiple_interfaces (struct interface_info *);
extern void maybe_setup_fallback PROTO ((void));
#endif

/* discover.c */
extern struct interface_info *interfaces,
	*dummy_interfaces, *fallback_interface;
extern struct protocol *protocols;
extern int quiet_interface_discovery;
extern isc_result_t interface_setup (void);
extern void interface_trace_setup (void);

extern struct in_addr limited_broadcast;
extern struct in_addr local_address;

extern u_int16_t local_port;
extern u_int16_t remote_port;
extern int (*dhcp_interface_setup_hook) (struct interface_info *,
					 struct iaddr *);
extern int (*dhcp_interface_discovery_hook) (struct interface_info *);
extern isc_result_t (*dhcp_interface_startup_hook) (struct interface_info *);

extern void (*bootp_packet_handler) PROTO ((struct interface_info *,
					    struct dhcp_packet *, unsigned,
					    unsigned int,
					    struct iaddr, struct hardware *));
#if 0 /* this can be private to common/dispatch.c */
extern struct timeout *timeouts;
#endif
extern omapi_object_type_t *dhcp_type_interface;
#if defined (TRACING)
extern trace_type_t *interface_trace;
extern trace_type_t *inpacket_trace;
extern trace_type_t *outpacket_trace;
#endif
extern struct interface_info **interface_vector;
extern int interface_count;
extern int interface_max;
extern isc_result_t interface_initialize (omapi_object_t *, const char *, int);
extern void discover_interfaces PROTO ((int));
extern int setup_fallback (struct interface_info **, const char *, int);
extern int if_readsocket PROTO ((omapi_object_t *));
extern void reinitialize_interfaces PROTO ((void));

/* dispatch.c */
extern void set_time (u_int32_t);
extern struct timeval *process_outstanding_timeouts (struct timeval *);
extern void dispatch PROTO ((void));
extern isc_result_t got_one PROTO ((omapi_object_t *));
extern isc_result_t interface_set_value (omapi_object_t *, omapi_object_t *,
				  omapi_data_string_t *, omapi_typed_data_t *);
extern isc_result_t interface_get_value (omapi_object_t *, omapi_object_t *,
				  omapi_data_string_t *, omapi_value_t **); 
extern isc_result_t interface_destroy (omapi_object_t *, const char *, int);
extern isc_result_t interface_signal_handler (omapi_object_t *,
				       const char *, va_list);
extern isc_result_t interface_stuff_values (omapi_object_t *,
				     omapi_object_t *,
				     omapi_object_t *);

extern void add_timeout PROTO ((TIME, void (*) PROTO ((void *)), void *,
			 tvref_t, tvunref_t));
extern void cancel_timeout PROTO ((void (*) PROTO ((void *)), void *));
extern void cancel_all_timeouts (void);
extern void relinquish_timeouts (void);
#if 0
extern struct protocol *add_protocol PROTO ((const char *, int,
				      void (*) PROTO ((struct protocol *)),
				      void *));

extern void remove_protocol PROTO ((struct protocol *));
#endif
OMAPI_OBJECT_ALLOC_DECL (interface,
			 struct interface_info, dhcp_type_interface)

/* tables.c */
extern struct universe dhcp_universe;
extern struct option dhcp_options [256];
extern struct universe nwip_universe;
extern struct option nwip_options [256];
extern struct universe fqdn_universe;
extern struct option fqdn_options [256];
extern int dhcp_option_default_priority_list [];
extern int dhcp_option_default_priority_list_count;
extern const char *hardware_types [256];

extern universe_hash_t *universe_hash;
extern void initialize_common_option_spaces PROTO ((void));
extern int universe_count, universe_max;
extern struct universe **universes;/* stables.c */
extern struct universe *config_universe;
#if defined (FAILOVER_PROTOCOL)
extern failover_option_t null_failover_option;
extern failover_option_t skip_failover_option;
extern struct failover_option_info ft_options [];
extern u_int32_t fto_allowed [];
extern int ft_sizes [];
extern const char *dhcp_flink_state_names [];
#endif
extern const char *binding_state_names [];

extern struct universe agent_universe;
extern struct option agent_options [256];
extern struct universe server_universe;
extern struct option server_options [256];

extern struct enumeration ddns_styles;
extern struct enumeration syslog_enum;
extern void initialize_server_option_spaces PROTO ((void));

/* inet.c */
extern struct iaddr subnet_number PROTO ((struct iaddr, struct iaddr));
extern struct iaddr ip_addr PROTO ((struct iaddr, struct iaddr, u_int32_t));
extern struct iaddr broadcast_addr PROTO ((struct iaddr, struct iaddr));
extern u_int32_t host_addr PROTO ((struct iaddr, struct iaddr));
extern int addr_eq PROTO ((struct iaddr, struct iaddr));
extern char *piaddr PROTO ((struct iaddr));
extern char *piaddrmask (struct iaddr, struct iaddr, const char *, int);

/* dhclient.c */
extern const char *path_dhclient_conf;
extern const char *path_dhclient_db;
extern const char *path_dhclient_pid;
extern char *path_dhclient_script;
extern int interfaces_requested;

extern struct client_config top_level_config;

extern void dhcpoffer PROTO ((struct packet *));
extern void dhcpack PROTO ((struct packet *));
extern void dhcpnak PROTO ((struct packet *));

extern void send_discover PROTO ((void *));
extern void send_request PROTO ((void *));
extern void send_release PROTO ((void *));
extern void send_decline PROTO ((void *));

extern void state_reboot PROTO ((void *));
extern void state_init PROTO ((void *));
extern void state_selecting PROTO ((void *));
extern void state_requesting PROTO ((void *));
extern void state_bound PROTO ((void *));
extern void state_stop PROTO ((void *));
extern void state_panic PROTO ((void *));

extern void bind_lease PROTO ((struct client_state *));

extern void make_client_options PROTO ((struct client_state *,
				 struct client_lease *, u_int8_t *,
				 struct option_cache *, struct iaddr *,
				 u_int32_t *, struct option_state **));
extern void make_discover PROTO ((struct client_state *, struct client_lease *));
extern void make_request PROTO ((struct client_state *, struct client_lease *));
extern void make_decline PROTO ((struct client_state *, struct client_lease *));
extern void make_release PROTO ((struct client_state *, struct client_lease *));

extern void destroy_client_lease PROTO ((struct client_lease *));
extern void rewrite_client_leases PROTO ((void));
extern void write_lease_option (struct option_cache *, struct packet *,
			 struct lease *, struct client_state *,
			 struct option_state *, struct option_state *,
			 struct binding_scope **, struct universe *, void *);
extern int write_client_lease PROTO ((struct client_state *,
			       struct client_lease *, int, int));
extern int dhcp_option_ev_name (char *, size_t, struct option *);

extern void script_init PROTO ((struct client_state *, const char *,
			 struct string_list *));
extern void client_option_envadd (struct option_cache *, struct packet *,
			   struct lease *, struct client_state *,
			   struct option_state *, struct option_state *,
			   struct binding_scope **, struct universe *, void *);
extern void script_write_params PROTO ((struct client_state *,
				 const char *, struct client_lease *));
extern int script_go PROTO ((struct client_state *));
extern void client_envadd (struct client_state *,
		    const char *, const char *, const char *, ...)
	__attribute__((__format__(__printf__,4,5)));

extern struct client_lease *packet_to_lease (struct packet *, struct client_state *);
extern void go_daemon PROTO ((void));
extern void write_client_pid_file PROTO ((void));
extern void client_location_changed PROTO ((void));
extern void do_release PROTO ((struct client_state *));
extern int dhclient_interface_shutdown_hook (struct interface_info *);
extern int dhclient_interface_discovery_hook (struct interface_info *);
extern isc_result_t dhclient_interface_startup_hook (struct interface_info *);
extern void client_dns_update_timeout (void *cp);
extern isc_result_t client_dns_update (struct client_state *client, int, int);

/* db.c */
extern int write_lease PROTO ((struct lease *));
extern int write_host PROTO ((struct host_decl *));
#if defined (FAILOVER_PROTOCOL)
extern int write_failover_state (dhcp_failover_state_t *);
#endif
extern int db_printable PROTO ((const char *));
extern int db_printable_len PROTO ((const unsigned char *, unsigned));
extern void write_named_billing_class (const char *, unsigned, struct class *);
extern void write_billing_classes (void);
extern int write_billing_class PROTO ((struct class *));
extern void commit_leases_timeout PROTO ((void *));
extern int commit_leases PROTO ((void));
extern void db_startup PROTO ((int));
extern int new_lease_file PROTO ((void));
extern int group_writer (struct group_object *);

/* packet.c */
extern u_int32_t checksum PROTO ((unsigned char *, unsigned, u_int32_t));
extern u_int32_t wrapsum PROTO ((u_int32_t));
extern void assemble_hw_header PROTO ((struct interface_info *, unsigned char *,
				unsigned *, struct hardware *));
extern void assemble_udp_ip_header PROTO ((struct interface_info *, unsigned char *,
				    unsigned *, u_int32_t, u_int32_t,
				    u_int32_t, unsigned char *, unsigned));
extern ssize_t decode_hw_header PROTO ((struct interface_info *, unsigned char *,
				 unsigned, struct hardware *));
extern ssize_t decode_udp_ip_header PROTO ((struct interface_info *, unsigned char *,
				     unsigned, struct sockaddr_in *,
				     unsigned));

/* ethernet.c */
extern void assemble_ethernet_header PROTO ((struct interface_info *, unsigned char *,
				      unsigned *, struct hardware *));
extern ssize_t decode_ethernet_header PROTO ((struct interface_info *,
				       unsigned char *,
				       unsigned, struct hardware *));

/* tr.c */
extern void assemble_tr_header PROTO ((struct interface_info *, unsigned char *,
				unsigned *, struct hardware *));
extern ssize_t decode_tr_header PROTO ((struct interface_info *,
				 unsigned char *,
				 unsigned, struct hardware *));

/* dhxpxlt.c */
extern void convert_statement PROTO ((struct parse *));
extern void convert_host_statement PROTO ((struct parse *, jrefproto));
extern void convert_host_name PROTO ((struct parse *, jrefproto));
extern void convert_class_statement PROTO ((struct parse *, jrefproto, int));
extern void convert_class_decl PROTO ((struct parse *, jrefproto));
extern void convert_lease_time PROTO ((struct parse *, jrefproto, char *));
extern void convert_shared_net_statement PROTO ((struct parse *, jrefproto));
extern void convert_subnet_statement PROTO ((struct parse *, jrefproto));
extern void convert_subnet_decl PROTO ((struct parse *, jrefproto));
extern void convert_host_decl PROTO ((struct parse *, jrefproto));
extern void convert_hardware_decl PROTO ((struct parse *, jrefproto));
extern void convert_hardware_addr PROTO ((struct parse *, jrefproto));
extern void convert_filename_decl PROTO ((struct parse *, jrefproto));
extern void convert_servername_decl PROTO ((struct parse *, jrefproto));
extern void convert_ip_addr_or_hostname PROTO ((struct parse *, jrefproto, int));
extern void convert_fixed_addr_decl PROTO ((struct parse *, jrefproto));
extern void convert_option_decl PROTO ((struct parse *, jrefproto));
extern void convert_timestamp PROTO ((struct parse *, jrefproto));
extern void convert_lease_statement PROTO ((struct parse *, jrefproto));
extern void convert_address_range PROTO ((struct parse *, jrefproto));
extern void convert_date PROTO ((struct parse *, jrefproto, char *));
extern void convert_numeric_aggregate PROTO ((struct parse *, jrefproto, int, int, int, int));
extern void indent PROTO ((int));

/* route.c */
extern void add_route_direct PROTO ((struct interface_info *, struct in_addr));
extern void add_route_net PROTO ((struct interface_info *, struct in_addr,
			   struct in_addr));
extern void add_route_default_gateway PROTO ((struct interface_info *, 
				       struct in_addr));
extern void remove_routes PROTO ((struct in_addr));
extern void remove_if_route PROTO ((struct interface_info *, struct in_addr));
extern void remove_all_if_routes PROTO ((struct interface_info *));
extern void set_netmask PROTO ((struct interface_info *, struct in_addr));
extern void set_broadcast_addr PROTO ((struct interface_info *, struct in_addr));
extern void set_ip_address PROTO ((struct interface_info *, struct in_addr));

/* clparse.c */
extern isc_result_t read_client_conf PROTO ((void));
extern int read_client_conf_file (const char *,
			   struct interface_info *, struct client_config *);
extern void read_client_leases PROTO ((void));
extern void parse_client_statement PROTO ((struct parse *, struct interface_info *,
				    struct client_config *));
extern int parse_X PROTO ((struct parse *, u_int8_t *, unsigned));
extern void parse_option_list PROTO ((struct parse *, u_int32_t **));
extern void parse_interface_declaration PROTO ((struct parse *,
					 struct client_config *, char *));
extern int interface_or_dummy PROTO ((struct interface_info **, const char *));
extern void make_client_state PROTO ((struct client_state **));
extern void make_client_config PROTO ((struct client_state *,
				struct client_config *));
extern void parse_client_lease_statement PROTO ((struct parse *, int));
extern void parse_client_lease_declaration PROTO ((struct parse *,
					    struct client_lease *,
					    struct interface_info **,
					    struct client_state **));
extern int parse_option_decl PROTO ((struct option_cache **, struct parse *));
extern void parse_string_list PROTO ((struct parse *, struct string_list **, int));
extern int parse_ip_addr PROTO ((struct parse *, struct iaddr *));
extern void parse_reject_statement PROTO ((struct parse *, struct client_config *));

/* dhcrelay.c */
extern void relay PROTO ((struct interface_info *, struct dhcp_packet *, unsigned,
		   unsigned int, struct iaddr, struct hardware *));
extern int strip_relay_agent_options PROTO ((struct interface_info *,
				      struct interface_info **,
				      struct dhcp_packet *, unsigned));
extern int find_interface_by_agent_option PROTO ((struct dhcp_packet *,
					   struct interface_info **,
					   u_int8_t *, int));
extern int add_relay_agent_options PROTO ((struct interface_info *,
				    struct dhcp_packet *,
				    unsigned, struct in_addr));

/* icmp.c */
OMAPI_OBJECT_ALLOC_DECL (icmp_state, struct icmp_state, dhcp_type_icmp)
extern struct icmp_state *icmp_state;
extern void icmp_startup PROTO ((int, void (*) PROTO ((struct iaddr,
						u_int8_t *, int))));
extern int icmp_readsocket PROTO ((omapi_object_t *));
extern int icmp_echorequest PROTO ((struct iaddr *));
extern isc_result_t icmp_echoreply PROTO ((omapi_object_t *));

/* dns.c */
#if defined (NSUPDATE)
extern isc_result_t find_tsig_key (ns_tsig_key **, const char *, struct dns_zone *);
extern void tkey_free (ns_tsig_key **);
#endif
extern isc_result_t enter_dns_zone (struct dns_zone *);
extern isc_result_t dns_zone_lookup (struct dns_zone **, const char *);
extern int dns_zone_dereference PROTO ((struct dns_zone **, const char *, int));
#if defined (NSUPDATE)
extern isc_result_t find_cached_zone (const char *, ns_class, char *,
			       size_t, struct in_addr *, int, int *,
			       struct dns_zone **);
extern void forget_zone (struct dns_zone **);
extern void repudiate_zone (struct dns_zone **);
extern void cache_found_zone (ns_class, char *, struct in_addr *, int);
extern int get_dhcid (struct data_string *, int, const u_int8_t *, unsigned);
extern isc_result_t ddns_update_a (struct data_string *, struct iaddr,
			    struct data_string *, unsigned long, int);
extern isc_result_t ddns_remove_a (struct data_string *,
			    struct iaddr, struct data_string *);
#endif /* NSUPDATE */

/* resolv.c */
extern char path_resolv_conf [];
extern struct name_server *name_servers;
extern struct domain_search_list *domains;

extern void read_resolv_conf PROTO ((TIME));
extern struct name_server *first_name_server PROTO ((void));

/* inet_addr.c */
#ifdef NEED_INET_ATON
extern int inet_aton PROTO ((const char *, struct in_addr *));
#endif

/* class.c */
extern int have_billing_classes;
extern struct class unknown_class;
extern struct class known_class;
extern struct collection default_collection;
extern struct collection *collections;
extern struct executable_statement *default_classification_rules;

extern void classification_setup PROTO ((void));
extern void classify_client PROTO ((struct packet *));
extern int check_collection PROTO ((struct packet *, struct lease *,
			     struct collection *));
extern void classify PROTO ((struct packet *, struct class *));
extern isc_result_t find_class PROTO ((struct class **, const char *,
				const char *, int));
extern int unbill_class PROTO ((struct lease *, struct class *));
extern int bill_class PROTO ((struct lease *, struct class *));

/* execute.c */
extern int execute_statements PROTO ((struct binding_value **result,
			       struct packet *, struct lease *,
			       struct client_state *,
			       struct option_state *, struct option_state *,
			       struct binding_scope **,
			       struct executable_statement *));
extern void execute_statements_in_scope PROTO ((struct binding_value **result,
					 struct packet *, struct lease *,
					 struct client_state *,
					 struct option_state *,
					 struct option_state *,
					 struct binding_scope **,
					 struct group *, struct group *));
extern int executable_statement_dereference PROTO ((struct executable_statement **,
					     const char *, int));
extern void write_statements (FILE *, struct executable_statement *, int);
extern int find_matching_case (struct executable_statement **,
			struct packet *, struct lease *, struct client_state *,
			struct option_state *, struct option_state *,
			struct binding_scope **,
			struct expression *, struct executable_statement *);
extern int executable_statement_foreach (struct executable_statement *,
				  int (*) (struct executable_statement *,
					   void *, int), void *, int);

/* comapi.c */
extern omapi_object_type_t *dhcp_type_interface;
extern omapi_object_type_t *dhcp_type_group;
extern omapi_object_type_t *dhcp_type_shared_network;
extern omapi_object_type_t *dhcp_type_subnet;
extern omapi_object_type_t *dhcp_type_control;
extern dhcp_control_object_t *dhcp_control_object;

extern void dhcp_common_objects_setup (void);

extern isc_result_t dhcp_group_set_value  (omapi_object_t *, omapi_object_t *,
				    omapi_data_string_t *,
				    omapi_typed_data_t *);
extern isc_result_t dhcp_group_get_value (omapi_object_t *, omapi_object_t *,
				   omapi_data_string_t *,
				   omapi_value_t **); 
extern isc_result_t dhcp_group_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_group_signal_handler (omapi_object_t *,
					const char *, va_list);
extern isc_result_t dhcp_group_stuff_values (omapi_object_t *,
				      omapi_object_t *,
				      omapi_object_t *);
extern isc_result_t dhcp_group_lookup (omapi_object_t **,
				omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_group_create (omapi_object_t **,
				omapi_object_t *);
extern isc_result_t dhcp_group_remove (omapi_object_t *,
				omapi_object_t *);

extern isc_result_t dhcp_control_set_value  (omapi_object_t *, omapi_object_t *,
				      omapi_data_string_t *,
				      omapi_typed_data_t *);
extern isc_result_t dhcp_control_get_value (omapi_object_t *, omapi_object_t *,
				     omapi_data_string_t *,
				     omapi_value_t **); 
extern isc_result_t dhcp_control_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_control_signal_handler (omapi_object_t *,
					  const char *, va_list);
extern isc_result_t dhcp_control_stuff_values (omapi_object_t *,
					omapi_object_t *,
					omapi_object_t *);
extern isc_result_t dhcp_control_lookup (omapi_object_t **,
				  omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_control_create (omapi_object_t **,
				  omapi_object_t *);
extern isc_result_t dhcp_control_remove (omapi_object_t *,
				  omapi_object_t *);

extern isc_result_t dhcp_subnet_set_value  (omapi_object_t *, omapi_object_t *,
				     omapi_data_string_t *,
				     omapi_typed_data_t *);
extern isc_result_t dhcp_subnet_get_value (omapi_object_t *, omapi_object_t *,
				    omapi_data_string_t *,
				    omapi_value_t **); 
extern isc_result_t dhcp_subnet_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_subnet_signal_handler (omapi_object_t *,
					 const char *, va_list);
extern isc_result_t dhcp_subnet_stuff_values (omapi_object_t *,
				       omapi_object_t *,
				       omapi_object_t *);
extern isc_result_t dhcp_subnet_lookup (omapi_object_t **,
				 omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_subnet_create (omapi_object_t **,
				 omapi_object_t *);
extern isc_result_t dhcp_subnet_remove (omapi_object_t *,
				 omapi_object_t *);

extern isc_result_t dhcp_shared_network_set_value  (omapi_object_t *,
					     omapi_object_t *,
					     omapi_data_string_t *,
					     omapi_typed_data_t *);
extern isc_result_t dhcp_shared_network_get_value (omapi_object_t *,
					    omapi_object_t *,
					    omapi_data_string_t *,
					    omapi_value_t **); 
extern isc_result_t dhcp_shared_network_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_shared_network_signal_handler (omapi_object_t *,
						 const char *, va_list);
extern isc_result_t dhcp_shared_network_stuff_values (omapi_object_t *,
					       omapi_object_t *,
					       omapi_object_t *);
extern isc_result_t dhcp_shared_network_lookup (omapi_object_t **,
					 omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_shared_network_create (omapi_object_t **,
					 omapi_object_t *);
extern isc_result_t dhcp_shared_network_remove (omapi_object_t *,
					 omapi_object_t *);

/* omapi.c */
extern int (*dhcp_interface_shutdown_hook) (struct interface_info *);

extern omapi_object_type_t *dhcp_type_lease;
extern omapi_object_type_t *dhcp_type_pool;
extern omapi_object_type_t *dhcp_type_class;

#if defined (FAILOVER_PROTOCOL)
extern omapi_object_type_t *dhcp_type_failover_state;
extern omapi_object_type_t *dhcp_type_failover_link;
extern omapi_object_type_t *dhcp_type_failover_listener;
#endif

extern void dhcp_db_objects_setup (void);

extern isc_result_t dhcp_lease_set_value  (omapi_object_t *, omapi_object_t *,
				    omapi_data_string_t *,
				    omapi_typed_data_t *);
extern isc_result_t dhcp_lease_get_value (omapi_object_t *, omapi_object_t *,
				   omapi_data_string_t *,
				   omapi_value_t **); 
extern isc_result_t dhcp_lease_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_lease_signal_handler (omapi_object_t *,
					const char *, va_list);
extern isc_result_t dhcp_lease_stuff_values (omapi_object_t *,
				      omapi_object_t *,
				      omapi_object_t *);
extern isc_result_t dhcp_lease_lookup (omapi_object_t **,
				omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_lease_create (omapi_object_t **,
				omapi_object_t *);
extern isc_result_t dhcp_lease_remove (omapi_object_t *,
				omapi_object_t *);
extern isc_result_t dhcp_group_set_value  (omapi_object_t *, omapi_object_t *,
				    omapi_data_string_t *,
				    omapi_typed_data_t *);
extern isc_result_t dhcp_group_get_value (omapi_object_t *, omapi_object_t *,
				   omapi_data_string_t *,
				   omapi_value_t **); 
extern isc_result_t dhcp_group_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_group_signal_handler (omapi_object_t *,
					const char *, va_list);
extern isc_result_t dhcp_group_stuff_values (omapi_object_t *,
				      omapi_object_t *,
				      omapi_object_t *);
extern isc_result_t dhcp_group_lookup (omapi_object_t **,
				omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_group_create (omapi_object_t **,
				omapi_object_t *);
extern isc_result_t dhcp_group_remove (omapi_object_t *,
				omapi_object_t *);
extern isc_result_t dhcp_host_set_value  (omapi_object_t *, omapi_object_t *,
				   omapi_data_string_t *,
				   omapi_typed_data_t *);
extern isc_result_t dhcp_host_get_value (omapi_object_t *, omapi_object_t *,
				  omapi_data_string_t *,
				  omapi_value_t **); 
extern isc_result_t dhcp_host_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_host_signal_handler (omapi_object_t *,
				       const char *, va_list);
extern isc_result_t dhcp_host_stuff_values (omapi_object_t *,
				     omapi_object_t *,
				     omapi_object_t *);
extern isc_result_t dhcp_host_lookup (omapi_object_t **,
			       omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_host_create (omapi_object_t **,
			       omapi_object_t *);
extern isc_result_t dhcp_host_remove (omapi_object_t *,
			       omapi_object_t *);
extern isc_result_t dhcp_pool_set_value  (omapi_object_t *, omapi_object_t *,
				   omapi_data_string_t *,
				   omapi_typed_data_t *);
extern isc_result_t dhcp_pool_get_value (omapi_object_t *, omapi_object_t *,
				  omapi_data_string_t *,
				  omapi_value_t **); 
extern isc_result_t dhcp_pool_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_pool_signal_handler (omapi_object_t *,
				       const char *, va_list);
extern isc_result_t dhcp_pool_stuff_values (omapi_object_t *,
				     omapi_object_t *,
				     omapi_object_t *);
extern isc_result_t dhcp_pool_lookup (omapi_object_t **,
			       omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_pool_create (omapi_object_t **,
			       omapi_object_t *);
extern isc_result_t dhcp_pool_remove (omapi_object_t *,
			       omapi_object_t *);
extern isc_result_t dhcp_class_set_value  (omapi_object_t *, omapi_object_t *,
				    omapi_data_string_t *,
				    omapi_typed_data_t *);
extern isc_result_t dhcp_class_get_value (omapi_object_t *, omapi_object_t *,
				   omapi_data_string_t *,
				   omapi_value_t **); 
extern isc_result_t dhcp_class_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_class_signal_handler (omapi_object_t *,
					const char *, va_list);
extern isc_result_t dhcp_class_stuff_values (omapi_object_t *,
				      omapi_object_t *,
				      omapi_object_t *);
extern isc_result_t dhcp_class_lookup (omapi_object_t **,
				omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_class_create (omapi_object_t **,
				omapi_object_t *);
extern isc_result_t dhcp_class_remove (omapi_object_t *,
				omapi_object_t *);
extern isc_result_t dhcp_subclass_set_value  (omapi_object_t *, omapi_object_t *,
				       omapi_data_string_t *,
				       omapi_typed_data_t *);
extern isc_result_t dhcp_subclass_get_value (omapi_object_t *, omapi_object_t *,
				      omapi_data_string_t *,
				      omapi_value_t **); 
extern isc_result_t dhcp_subclass_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_subclass_signal_handler (omapi_object_t *,
					   const char *, va_list);
extern isc_result_t dhcp_subclass_stuff_values (omapi_object_t *,
					 omapi_object_t *,
					 omapi_object_t *);
extern isc_result_t dhcp_subclass_lookup (omapi_object_t **,
				   omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_subclass_create (omapi_object_t **,
				   omapi_object_t *);
extern isc_result_t dhcp_subclass_remove (omapi_object_t *,
				   omapi_object_t *);
extern isc_result_t dhcp_shared_network_set_value  (omapi_object_t *,
					     omapi_object_t *,
					     omapi_data_string_t *,
					     omapi_typed_data_t *);
extern isc_result_t dhcp_shared_network_get_value (omapi_object_t *, omapi_object_t *,
					    omapi_data_string_t *,
					    omapi_value_t **); 
extern isc_result_t dhcp_shared_network_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_shared_network_signal_handler (omapi_object_t *,
						 const char *, va_list);
extern isc_result_t dhcp_shared_network_stuff_values (omapi_object_t *,
					       omapi_object_t *,
					       omapi_object_t *);
extern isc_result_t dhcp_shared_network_lookup (omapi_object_t **,
					 omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_shared_network_create (omapi_object_t **,
					 omapi_object_t *);
extern isc_result_t dhcp_subnet_set_value  (omapi_object_t *, omapi_object_t *,
				     omapi_data_string_t *,
				     omapi_typed_data_t *);
extern isc_result_t dhcp_subnet_get_value (omapi_object_t *, omapi_object_t *,
				    omapi_data_string_t *,
				    omapi_value_t **); 
extern isc_result_t dhcp_subnet_destroy (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_subnet_signal_handler (omapi_object_t *,
					 const char *, va_list);
extern isc_result_t dhcp_subnet_stuff_values (omapi_object_t *,
				       omapi_object_t *,
				       omapi_object_t *);
extern isc_result_t dhcp_subnet_lookup (omapi_object_t **,
				 omapi_object_t *, omapi_object_t *);
extern isc_result_t dhcp_subnet_create (omapi_object_t **,
				 omapi_object_t *);
extern isc_result_t dhcp_interface_set_value (omapi_object_t *,
				       omapi_object_t *,
				       omapi_data_string_t *,
				       omapi_typed_data_t *);
extern isc_result_t dhcp_interface_get_value (omapi_object_t *,
				       omapi_object_t *,
				       omapi_data_string_t *,
				       omapi_value_t **);
extern isc_result_t dhcp_interface_destroy (omapi_object_t *,
				     const char *, int);
extern isc_result_t dhcp_interface_signal_handler (omapi_object_t *,
					    const char *,
					    va_list ap);
extern isc_result_t dhcp_interface_stuff_values (omapi_object_t *,
					  omapi_object_t *,
					  omapi_object_t *);
extern isc_result_t dhcp_interface_lookup (omapi_object_t **,
				    omapi_object_t *,
				    omapi_object_t *);
extern isc_result_t dhcp_interface_create (omapi_object_t **,
				    omapi_object_t *);
extern isc_result_t dhcp_interface_remove (omapi_object_t *,
				    omapi_object_t *);
extern void interface_stash (struct interface_info *);
extern void interface_snorf (struct interface_info *, int);

extern isc_result_t binding_scope_set_value (struct binding_scope *, int,
				      omapi_data_string_t *,
				      omapi_typed_data_t *);
extern isc_result_t binding_scope_get_value (omapi_value_t **,
				      struct binding_scope *,
				      omapi_data_string_t *);
extern isc_result_t binding_scope_stuff_values (omapi_object_t *,
					 struct binding_scope *);

/* mdb.c */

extern struct subnet *subnets;
extern struct shared_network *shared_networks;
extern host_hash_t *host_hw_addr_hash;
extern host_hash_t *host_uid_hash;
extern host_hash_t *host_name_hash;
extern lease_hash_t *lease_uid_hash;
extern lease_hash_t *lease_ip_addr_hash;
extern lease_hash_t *lease_hw_addr_hash;

extern omapi_object_type_t *dhcp_type_host;

extern isc_result_t enter_host PROTO ((struct host_decl *, int, int));
extern isc_result_t delete_host PROTO ((struct host_decl *, int));
extern int find_hosts_by_haddr PROTO ((struct host_decl **, int,
				const unsigned char *, unsigned,
				const char *, int));
extern int find_hosts_by_uid PROTO ((struct host_decl **, const unsigned char *,
			      unsigned, const char *, int));
extern int find_host_for_network PROTO ((struct subnet **, struct host_decl **,
				  struct iaddr *, struct shared_network *));
extern void new_address_range PROTO ((struct parse *, struct iaddr, struct iaddr,
			       struct subnet *, struct pool *,
			       struct lease **));
extern isc_result_t dhcp_lease_free (omapi_object_t *, const char *, int);
extern isc_result_t dhcp_lease_get (omapi_object_t **, const char *, int);
extern int find_grouped_subnet PROTO ((struct subnet **, struct shared_network *,
				struct iaddr, const char *, int));
extern int find_subnet (struct subnet **, struct iaddr, const char *, int);
extern void enter_shared_network PROTO ((struct shared_network *));
extern void new_shared_network_interface PROTO ((struct parse *,
					  struct shared_network *,
					  const char *));
extern int subnet_inner_than PROTO ((struct subnet *, struct subnet *, int));
extern void enter_subnet PROTO ((struct subnet *));
extern void enter_lease PROTO ((struct lease *));
extern int supersede_lease PROTO ((struct lease *, struct lease *, int, int, int));
extern void make_binding_state_transition (struct lease *);
extern int lease_copy PROTO ((struct lease **, struct lease *, const char *, int));
extern void release_lease PROTO ((struct lease *, struct packet *));
extern void abandon_lease PROTO ((struct lease *, const char *));
extern void dissociate_lease PROTO ((struct lease *));
extern void pool_timer PROTO ((void *));
extern int find_lease_by_uid PROTO ((struct lease **, const unsigned char *,
			      unsigned, const char *, int));
extern int find_lease_by_hw_addr PROTO ((struct lease **, const unsigned char *,
				  unsigned, const char *, int));
extern int find_lease_by_ip_addr PROTO ((struct lease **, struct iaddr,
				  const char *, int));
extern void uid_hash_add PROTO ((struct lease *));
extern void uid_hash_delete PROTO ((struct lease *));
extern void hw_hash_add PROTO ((struct lease *));
extern void hw_hash_delete PROTO ((struct lease *));
extern int write_leases PROTO ((void));
extern int lease_enqueue (struct lease *);
extern void lease_instantiate (const unsigned char *, unsigned, struct lease *);
extern void expire_all_pools PROTO ((void));
extern void dump_subnets PROTO ((void));
#if defined (DEBUG_MEMORY_LEAKAGE) || \
		defined (DEBUG_MEMORY_LEAKAGE_ON_EXIT)
extern void free_everything (void);
#endif

/* nsupdate.c */
extern char *ddns_rev_name (struct lease *, struct lease_state *, struct packet *);
extern char *ddns_fwd_name (struct lease *, struct lease_state *, struct packet *);
extern int nsupdateA (const char *, const unsigned char *, u_int32_t, int);
extern int nsupdatePTR (const char *, const unsigned char *, u_int32_t, int);
extern void nsupdate (struct lease *, struct lease_state *, struct packet *, int);
extern int updateA (const struct data_string *, const struct data_string *,
	     unsigned int, struct lease *);
extern int updatePTR (const struct data_string *, const struct data_string *,
	       unsigned int, struct lease *);
extern int deleteA (const struct data_string *, const struct data_string *,
	     struct lease *);
extern int deletePTR (const struct data_string *, const struct data_string *,
	       struct lease *);

/* failover.c */
#if defined (FAILOVER_PROTOCOL)
extern dhcp_failover_state_t *failover_states;
extern void dhcp_failover_startup PROTO ((void));
extern int dhcp_failover_write_all_states (void);
extern isc_result_t enter_failover_peer PROTO ((dhcp_failover_state_t *));
extern isc_result_t find_failover_peer PROTO ((dhcp_failover_state_t **,
					const char *, const char *, int));
extern isc_result_t dhcp_failover_link_initiate PROTO ((omapi_object_t *));
extern isc_result_t dhcp_failover_link_signal PROTO ((omapi_object_t *,
					       const char *, va_list));
extern isc_result_t dhcp_failover_link_set_value PROTO ((omapi_object_t *,
						  omapi_object_t *,
						  omapi_data_string_t *,
						  omapi_typed_data_t *));
extern isc_result_t dhcp_failover_link_get_value PROTO ((omapi_object_t *,
						  omapi_object_t *,
						  omapi_data_string_t *,
						  omapi_value_t **));
extern isc_result_t dhcp_failover_link_destroy PROTO ((omapi_object_t *,
						const char *, int));
extern isc_result_t dhcp_failover_link_stuff_values PROTO ((omapi_object_t *,
						     omapi_object_t *,
						     omapi_object_t *));
extern isc_result_t dhcp_failover_listen PROTO ((omapi_object_t *));

extern isc_result_t dhcp_failover_listener_signal PROTO ((omapi_object_t *,
						   const char *,
						   va_list));
extern isc_result_t dhcp_failover_listener_set_value PROTO ((omapi_object_t *,
						      omapi_object_t *,
						      omapi_data_string_t *,
						      omapi_typed_data_t *));
extern isc_result_t dhcp_failover_listener_get_value PROTO ((omapi_object_t *,
						      omapi_object_t *,
						      omapi_data_string_t *,
						      omapi_value_t **));
extern isc_result_t dhcp_failover_listener_destroy PROTO ((omapi_object_t *,
						    const char *, int));
extern isc_result_t dhcp_failover_listener_stuff PROTO ((omapi_object_t *,
						  omapi_object_t *,
						  omapi_object_t *));
extern isc_result_t dhcp_failover_register PROTO ((omapi_object_t *));
extern isc_result_t dhcp_failover_state_signal PROTO ((omapi_object_t *,
						const char *, va_list));
extern isc_result_t dhcp_failover_state_transition (dhcp_failover_state_t *,
					     const char *);
extern isc_result_t dhcp_failover_set_service_state (dhcp_failover_state_t *state);
extern isc_result_t dhcp_failover_set_state (dhcp_failover_state_t *,
				      enum failover_state);
extern isc_result_t dhcp_failover_peer_state_changed (dhcp_failover_state_t *,
					       failover_message_t *);
extern int dhcp_failover_pool_rebalance (dhcp_failover_state_t *);
extern int dhcp_failover_pool_check (struct pool *);
extern int dhcp_failover_state_pool_check (dhcp_failover_state_t *);
extern void dhcp_failover_timeout (void *);
extern void dhcp_failover_send_contact (void *);
extern isc_result_t dhcp_failover_send_state (dhcp_failover_state_t *);
extern isc_result_t dhcp_failover_send_updates (dhcp_failover_state_t *);
extern int dhcp_failover_queue_update (struct lease *, int);
extern int dhcp_failover_send_acks (dhcp_failover_state_t *);
extern void dhcp_failover_toack_queue_timeout (void *);
extern int dhcp_failover_queue_ack (dhcp_failover_state_t *, failover_message_t *msg);
extern void dhcp_failover_ack_queue_remove (dhcp_failover_state_t *, struct lease *);
extern isc_result_t dhcp_failover_state_set_value PROTO ((omapi_object_t *,
						   omapi_object_t *,
						   omapi_data_string_t *,
						   omapi_typed_data_t *));
extern void dhcp_failover_keepalive (void *);
extern void dhcp_failover_reconnect (void *);
extern void dhcp_failover_startup_timeout (void *);
extern void dhcp_failover_link_startup_timeout (void *);
extern void dhcp_failover_listener_restart (void *);
extern isc_result_t dhcp_failover_state_get_value PROTO ((omapi_object_t *,
						   omapi_object_t *,
						   omapi_data_string_t *,
						   omapi_value_t **));
extern isc_result_t dhcp_failover_state_destroy PROTO ((omapi_object_t *,
						 const char *, int));
extern isc_result_t dhcp_failover_state_stuff PROTO ((omapi_object_t *,
					       omapi_object_t *,
					       omapi_object_t *));
extern isc_result_t dhcp_failover_state_lookup PROTO ((omapi_object_t **,
						omapi_object_t *,
						omapi_object_t *));
extern isc_result_t dhcp_failover_state_create PROTO ((omapi_object_t **,
						omapi_object_t *));
extern isc_result_t dhcp_failover_state_remove PROTO ((omapi_object_t *,
					       omapi_object_t *));
extern int dhcp_failover_state_match (dhcp_failover_state_t *, u_int8_t *, unsigned);
extern const char *dhcp_failover_reject_reason_print (int);
extern const char *dhcp_failover_state_name_print (enum failover_state);
extern const char *dhcp_failover_message_name (unsigned);
extern const char *dhcp_failover_option_name (unsigned);
extern failover_option_t *dhcp_failover_option_printf (unsigned, char *,
						unsigned *,
						unsigned, 
						const char *, ...)
	__attribute__((__format__(__printf__,5,6)));
extern failover_option_t *dhcp_failover_make_option (unsigned, char *,
					      unsigned *, unsigned, ...);
extern isc_result_t dhcp_failover_put_message (dhcp_failover_link_t *,
					omapi_object_t *, int, ...);
extern isc_result_t dhcp_failover_send_connect PROTO ((omapi_object_t *));
extern isc_result_t dhcp_failover_send_connectack PROTO ((omapi_object_t *,
						   dhcp_failover_state_t *,
						   int, const char *));
extern isc_result_t dhcp_failover_send_disconnect PROTO ((omapi_object_t *,
						   int, const char *));
extern isc_result_t dhcp_failover_send_bind_update (dhcp_failover_state_t *,
					     struct lease *);
extern isc_result_t dhcp_failover_send_bind_ack (dhcp_failover_state_t *,
					  failover_message_t *,
					  int, const char *);
extern isc_result_t dhcp_failover_send_poolreq (dhcp_failover_state_t *);
extern isc_result_t dhcp_failover_send_poolresp (dhcp_failover_state_t *, int);
extern isc_result_t dhcp_failover_send_update_request (dhcp_failover_state_t *);
extern isc_result_t dhcp_failover_send_update_request_all (dhcp_failover_state_t *);
extern isc_result_t dhcp_failover_send_update_done (dhcp_failover_state_t *);
extern isc_result_t dhcp_failover_process_bind_update (dhcp_failover_state_t *,
						failover_message_t *);
extern isc_result_t dhcp_failover_process_bind_ack (dhcp_failover_state_t *,
					     failover_message_t *);
extern isc_result_t dhcp_failover_generate_update_queue (dhcp_failover_state_t *,
						  int);
extern isc_result_t dhcp_failover_process_update_request (dhcp_failover_state_t *,
						   failover_message_t *);
extern isc_result_t dhcp_failover_process_update_request_all (dhcp_failover_state_t *,
						       failover_message_t *);
extern isc_result_t dhcp_failover_process_update_done (dhcp_failover_state_t *,
						failover_message_t *);
extern void dhcp_failover_recover_done (void *);
extern void failover_print PROTO ((char *, unsigned *, unsigned, const char *));
extern void update_partner PROTO ((struct lease *));
extern int load_balance_mine (struct packet *, dhcp_failover_state_t *);
extern binding_state_t normal_binding_state_transition_check (struct lease *,
						       dhcp_failover_state_t *,
						       binding_state_t,
						       u_int32_t);
extern binding_state_t
conflict_binding_state_transition_check (struct lease *,
					 dhcp_failover_state_t *,
					 binding_state_t, u_int32_t);
extern int lease_mine_to_reallocate (struct lease *);

OMAPI_OBJECT_ALLOC_DECL (dhcp_failover_state, dhcp_failover_state_t,
			 dhcp_type_failover_state)
OMAPI_OBJECT_ALLOC_DECL (dhcp_failover_listener, dhcp_failover_listener_t,
			 dhcp_type_failover_listener)
OMAPI_OBJECT_ALLOC_DECL (dhcp_failover_link, dhcp_failover_link_t,
			 dhcp_type_failover_link)
#endif /* FAILOVER_PROTOCOL */

extern const char *binding_state_print (enum failover_state);
