/* isclib_aros.h - Minimal ISC type stubs for AROS builds
 *
 * Replaces omapip/isclib.h (which pulls in the full BIND9 ISC library)
 * for platforms that do not have the ISC library available.
 */

#ifndef ISCLIB_AROS_H
#define ISCLIB_AROS_H 1

/* isc_boolean_t comes from our bundled isc-dhcp/boolean.h */
#include <isc-dhcp/boolean.h>

/* isc_result_t comes from our bundled isc-dhcp/result.h */
#include <isc-dhcp/result.h>

#include <netinet/in.h>
#include <sys/socket.h>

/* ISC POST macro - suppress unused result warnings */
#ifndef POST
# define POST(x) ((void)(x))
#endif

/* ISC IGNORE_RET macro - suppress warn_unused_result */
#ifndef IGNORE_RET
# define IGNORE_RET(x) ((void)(x))
#endif

/* ISC IGNORE_UNUSED macro - suppress unused variable warnings */
#ifndef IGNORE_UNUSED
# define IGNORE_UNUSED(x) ((void)(x))
#endif

/* isc_file_basename: return the filename component of a path */
#include <string.h>
static inline const char *isc_file_basename(const char *filename) {
	const char *s;
	if (filename == NULL) return "";
	s = strrchr(filename, '/');
	return s ? s + 1 : filename;
}

/* DHCP_DNS_CLIENT_LAZY_INIT: lazy DNS client init flag (not used on AROS) */
#define DHCP_DNS_CLIENT_LAZY_INIT  0x04

/* shutdown_signal: used by dhcp_set_control_state() */
#ifndef SHUTDOWN_SIGNAL_DEFINED
extern int shutdown_signal;
# define SHUTDOWN_SIGNAL_DEFINED 1
#endif

/* ISC socket watch/cancel flags (not used on AROS but referenced by
 * the fd_flags variable in omapi_register_io_object) */
#define ISC_SOCKFDWATCH_READ    0x01
#define ISC_SOCKFDWATCH_WRITE   0x02
#define ISC_SOCKCANCEL_ALL      0xFF

/* Minimal isc_sockaddr_t - wraps a union of sockaddr types */
typedef struct isc_sockaddr {
	union {
		struct sockaddr		sa;
		struct sockaddr_in	sin;
		struct sockaddr_in6	sin6;
	} type;
	unsigned int length;
} isc_sockaddr_t;

/* Minimal linked-list head for isc_sockaddrlist_t */
typedef struct isc_sockaddrlist {
	struct isc_sockaddr *head;
} isc_sockaddrlist_t;

/* Forward declarations for ISC types used only as pointers (never dereferenced
 * on AROS since the ISC code paths are ifdef'd out). */
typedef struct isc_mem     isc_mem_t;
typedef struct isc_appctx  isc_appctx_t;
typedef struct isc_taskmgr isc_taskmgr_t;
typedef struct isc_task    isc_task_t;
typedef struct isc_socketmgr isc_socketmgr_t;
typedef struct isc_socket  isc_socket_t;
typedef struct isc_timermgr isc_timermgr_t;
typedef struct isc_timer   isc_timer_t;
typedef struct isc_event   isc_event_t;
typedef struct isc_heap    isc_heap_t;

/* DNS/DST forward declarations used only in dns.c (excluded/guarded on AROS) */
typedef struct dns_client  dns_client_t;
typedef struct dst_key     dst_key_t;
typedef struct dns_tsec    dns_tsec_t;

/* DNS rdata types used in dhcpd.h struct fields */
typedef unsigned int dns_rdataclass_t;
typedef unsigned int dns_rdatatype_t;

/* Dummy isc_result values not already in isc-dhcp/result.h */
#ifndef ISC_R_RELOAD
#define ISC_R_RELOAD		((isc_result_t)11)
#endif

/* Maximum NS entries per zone (normally from isclib.h) */
#define DHCP_MAXNS         3
#define DHCP_MAXDNS_WIRE   256

/* dhcp_context_create flags (normally from isclib.h) */
#define DHCP_CONTEXT_PRE_DB   1
#define DHCP_CONTEXT_POST_DB  2

/* Minimal context struct - fields are void* so the compiler is happy;
 * on AROS the ISC code paths are never executed so they are never accessed. */
typedef struct dhcp_context {
	isc_mem_t       *mctx;
	isc_appctx_t    *actx;
	int              actx_started;
	int              actx_running;
	isc_taskmgr_t   *taskmgr;
	isc_task_t      *task;
	isc_socketmgr_t *socketmgr;
	isc_timermgr_t  *timermgr;
	/* NSUPDATE fields omitted - NSUPDATE is not enabled on AROS */
} dhcp_context_t;

extern dhcp_context_t dhcp_gbl_ctx;

isc_result_t dhcp_context_create(int flags,
				 struct in_addr *local4,
				 struct in6_addr *local6);
void isclib_cleanup(void);

/* isclib_make_dst_key is used in omapip/auth.c; stub declaration. */
struct data_string;
isc_result_t isclib_make_dst_key(char *name, char *algorithm,
				 unsigned char *secret, int secret_len,
				 dst_key_t **dstkey);

/* isc_result_totext() is provided by omapip/result.c */
const char *isc_result_totext(isc_result_t result);

#endif /* ISCLIB_AROS_H */
