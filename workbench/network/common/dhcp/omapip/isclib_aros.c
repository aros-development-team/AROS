/* isclib_aros.c - AROS stub for ISC library context
 *
 * Provides dhcp_gbl_ctx and no-op isclib_cleanup/dhcp_context_create
 * for builds without the ISC BIND library.
 */

#include "dhcpd.h"

/* Minimal context struct with only the fields AROS code references.
 * On non-AROS platforms this is defined in omapip/isclib.h with full ISC types. */
#if defined(__AROS__)

/* dhcp_gbl_ctx is referenced by omapip/dispatch.c even on AROS for the
 * socket manager fdwatch path (which we skip), but the linker needs the
 * symbol.  Define it as a zero-initialised opaque blob. */
dhcp_context_t dhcp_gbl_ctx;

/* shutdown_signal: used by dhcp_set_control_state() in dhclient.c.
 * Defined in isclib.c on non-AROS; we provide it here. */
int shutdown_signal = 0;

void
isclib_cleanup(void)
{
	/* No ISC library to clean up on AROS */
}

isc_result_t
dhcp_context_create(int flags,
		    struct in_addr *local4,
		    struct in6_addr *local6)
{
	/* No ISC library to initialise on AROS */
	return ISC_R_SUCCESS;
}

/* isc_result_totext: convert an ISC result code to a human-readable string */
const char *
isc_result_totext(isc_result_t result)
{
	switch (result) {
	case ISC_R_SUCCESS:     return "success";
	case ISC_R_NOMEMORY:    return "out of memory";
	case ISC_R_TIMEDOUT:    return "timed out";
	case ISC_R_NOPERM:      return "permission denied";
	case ISC_R_NOCONN:      return "not connected";
	case ISC_R_NETUNREACH:  return "network unreachable";
	case ISC_R_HOSTUNREACH: return "host unreachable";
	case ISC_R_CONNREFUSED: return "connection refused";
	case ISC_R_NORESOURCES: return "not enough free resources";
	case ISC_R_EOF:         return "end of file";
	case ISC_R_UNEXPECTED:  return "unexpected end of input";
	case ISC_R_INVALIDARG:  return "invalid argument";
	case ISC_R_EXISTS:      return "already exists";
	case ISC_R_NOTFOUND:    return "not found";
	case ISC_R_FAILURE:     return "failure";
	case ISC_R_NOTIMPLEMENTED: return "not implemented";
	case ISC_R_INPROGRESS:  return "operation in progress";
	default:                return "unknown error";
	}
}

/* isclib_make_dst_key: create a DST key from components.
 * Not supported on AROS (no BIND9 crypto library). */
isc_result_t
isclib_make_dst_key(char *inname, char *algorithm,
		    unsigned char *secret, int secret_len,
		    dst_key_t **dstkey)
{
	(void)inname; (void)algorithm; (void)secret;
	(void)secret_len; (void)dstkey;
	return ISC_R_NOTIMPLEMENTED;
}

#endif /* __AROS__ */
