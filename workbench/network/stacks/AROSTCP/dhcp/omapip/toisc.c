/* toisc.c

   Convert non-ISC result codes to ISC result codes. */

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 2001-2003 by Internet Software Consortium
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

#include <omapip/omapip_p.h>
#include "arpa/nameser.h"
#include "minires/minires.h"

isc_result_t ns_rcode_to_isc (int nsr)
{
	switch (nsr) {
	      case ns_r_noerror:
		return ISC_R_SUCCESS;

	      case ns_r_formerr:
		return ISC_R_FORMERR;

	      case ns_r_servfail:
		return ISC_R_SERVFAIL;

	      case ns_r_nxdomain:
		return ISC_R_NXDOMAIN;

	      case ns_r_notimpl:
		return ISC_R_NOTIMPL;

	      case ns_r_refused:
		return ISC_R_REFUSED;

	      case ns_r_yxdomain:
		return ISC_R_YXDOMAIN;

	      case ns_r_yxrrset:
		return ISC_R_YXRRSET;

	      case ns_r_nxrrset:
		return ISC_R_NXRRSET;

	      case ns_r_notauth:
		return ISC_R_NOTAUTH;

	      case ns_r_notzone:
		return ISC_R_NOTZONE;

	      case ns_r_badsig:
		return ISC_R_BADSIG;

	      case ns_r_badkey:
		return ISC_R_BADKEY;

	      case ns_r_badtime:
		return ISC_R_BADTIME;

	      default:
		;
	}
	return ISC_R_UNEXPECTED;
}

isc_result_t uerr2isc (int err)
{
	switch (err) {
	      case EPERM:
		return ISC_R_NOPERM;

	      case ENOENT:
		return ISC_R_NOTFOUND;

	      case ESRCH:
		return ISC_R_NOTFOUND;

	      case EIO:
		return ISC_R_IOERROR;

	      case ENXIO:
		return ISC_R_NOTFOUND;

	      case E2BIG:
		return ISC_R_NOSPACE;

	      case ENOEXEC:
		return ISC_R_FORMERR;

	      case ECHILD:
		return ISC_R_NOTFOUND;

	      case ENOMEM:
		return ISC_R_NOMEMORY;

	      case EACCES:
		return ISC_R_NOPERM;

	      case EFAULT:
		return ISC_R_INVALIDARG;

	      case EEXIST:
		return ISC_R_EXISTS;

	      case EINVAL:
		return ISC_R_INVALIDARG;

	      case ENOTTY:
		return ISC_R_INVALIDARG;

	      case EFBIG:
		return ISC_R_NOSPACE;

	      case ENOSPC:
		return ISC_R_NOSPACE;

	      case EROFS:
		return ISC_R_NOPERM;

	      case EMLINK:
		return ISC_R_NOSPACE;

	      case EPIPE:
		return ISC_R_NOTCONNECTED;

	      case EINPROGRESS:
		return ISC_R_ALREADYRUNNING;

	      case EALREADY:
		return ISC_R_ALREADYRUNNING;

	      case ENOTSOCK:
		return ISC_R_INVALIDFILE;

	      case EDESTADDRREQ:
		return ISC_R_DESTADDRREQ;

	      case EMSGSIZE:
		return ISC_R_NOSPACE;

	      case EPROTOTYPE:
		return ISC_R_INVALIDARG;

	      case ENOPROTOOPT:
		return ISC_R_NOTIMPLEMENTED;

	      case EPROTONOSUPPORT:
		return ISC_R_NOTIMPLEMENTED;

	      case ESOCKTNOSUPPORT:
		return ISC_R_NOTIMPLEMENTED;

	      case EOPNOTSUPP:
		return ISC_R_NOTIMPLEMENTED;

	      case EPFNOSUPPORT:
		return ISC_R_NOTIMPLEMENTED;

	      case EAFNOSUPPORT:
		return ISC_R_NOTIMPLEMENTED;

	      case EADDRINUSE:
		return ISC_R_ADDRINUSE;

	      case EADDRNOTAVAIL:
		return ISC_R_ADDRNOTAVAIL;

	      case ENETDOWN:
		return ISC_R_NETDOWN;

	      case ENETUNREACH:
		return ISC_R_NETUNREACH;

	      case ECONNABORTED:
		return ISC_R_TIMEDOUT;

	      case ECONNRESET:
		return ISC_R_CONNRESET;

	      case ENOBUFS:
		return ISC_R_NOSPACE;

	      case EISCONN:
		return ISC_R_ALREADYRUNNING;

	      case ENOTCONN:
		return ISC_R_NOTCONNECTED;

	      case ESHUTDOWN:
		return ISC_R_SHUTTINGDOWN;

	      case ETIMEDOUT:
		return ISC_R_TIMEDOUT;

	      case ECONNREFUSED:
		return ISC_R_CONNREFUSED;

	      case EHOSTDOWN:
		return ISC_R_HOSTDOWN;

	      case EHOSTUNREACH:
		return ISC_R_HOSTUNREACH;

#ifdef EDQUOT
	      case EDQUOT:
		return ISC_R_QUOTA;
#endif

#ifdef EBADRPC
	      case EBADRPC:
		return ISC_R_NOTIMPLEMENTED;
#endif

#ifdef ERPCMISMATCH
	      case ERPCMISMATCH:
		return ISC_R_VERSIONMISMATCH;
#endif

#ifdef EPROGMISMATCH
	      case EPROGMISMATCH:
		return ISC_R_VERSIONMISMATCH;
#endif

#ifdef EAUTH
	      case EAUTH:
		return ISC_R_NOTAUTH;
#endif

#ifdef ENEEDAUTH
	      case ENEEDAUTH:
		return ISC_R_NOTAUTH;
#endif

#ifdef EOVERFLOW
	      case EOVERFLOW:
		return ISC_R_NOSPACE;
#endif
	}
	return ISC_R_UNEXPECTED;
}

ns_rcode isc_rcode_to_ns (isc_result_t isc)
{
	switch (isc) {
	      case ISC_R_SUCCESS:
		return ns_r_noerror;

	      case ISC_R_FORMERR:
		return ns_r_formerr;

	      case ISC_R_SERVFAIL:
		return ns_r_servfail;

	      case ISC_R_NXDOMAIN:
		return ns_r_nxdomain;

	      case ISC_R_NOTIMPL:
		return ns_r_notimpl;

	      case ISC_R_REFUSED:
		return ns_r_refused;

	      case ISC_R_YXDOMAIN:
		return ns_r_yxdomain;

	      case ISC_R_YXRRSET:
		return ns_r_yxrrset;

	      case ISC_R_NXRRSET:
		return ns_r_nxrrset;

	      case ISC_R_NOTAUTH:
		return ns_r_notauth;

	      case ISC_R_NOTZONE:
		return ns_r_notzone;

	      case ISC_R_BADSIG:
		return ns_r_badsig;

	      case ISC_R_BADKEY:
		return ns_r_badkey;

	      case ISC_R_BADTIME:
		return ns_r_badtime;

	      default:
		;
	}
	return ns_r_servfail;
}
