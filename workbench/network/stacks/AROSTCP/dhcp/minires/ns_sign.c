/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1999-2003 by Internet Software Consortium
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
 */

#ifndef lint
static const char rcsid[] = "$Id$";
#endif

#if defined (TRACING)
#define time(x)		trace_mr_time (x)
time_t trace_mr_time (time_t *);
#endif

/* Import. */

#include <sys/types.h>
#include <sys/param.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "minires/minires.h"
#include "arpa/nameser.h"

#include <isc-dhcp/dst.h>

#define BOUNDS_CHECK(ptr, count) \
	do { \
		if ((ptr) + (count) > eob) { \
			return ISC_R_NOSPACE; \
		} \
	} while (0)

/* ns_sign
 * Parameters:
 *	msg		message to be sent
 *	msglen		input - length of message
 *			output - length of signed message
 *	msgsize		length of buffer containing message
 *	error		value to put in the error field
 *	key		tsig key used for signing
 *	querysig	(response), the signature in the query
 *	querysiglen	(response), the length of the signature in the query
 *	sig		a buffer to hold the generated signature
 *	siglen		input - length of signature buffer
 *			output - length of signature
 *
 * Errors:
 *	- bad input data (-1)
 *	- bad key / sign failed (-BADKEY)
 *	- not enough space (NS_TSIG_ERROR_NO_SPACE)
 */
isc_result_t
ns_sign(u_char *msg, unsigned *msglen, unsigned msgsize, int error, void *k,
	const u_char *querysig, unsigned querysiglen, u_char *sig,
	unsigned *siglen, time_t in_timesigned)
{
	HEADER *hp = (HEADER *)msg;
	DST_KEY *key = (DST_KEY *)k;
	u_char *cp = msg + *msglen, *eob = msg + msgsize;
	u_char *lenp;
	u_char *name, *alg;
	unsigned n;
	time_t timesigned;

	dst_init();
	if (msg == NULL || msglen == NULL || sig == NULL || siglen == NULL)
		return ISC_R_INVALIDARG;

	/* Name. */
	if (key != NULL && error != ns_r_badsig && error != ns_r_badkey)
		n = dn_comp(key->dk_key_name,
			    cp, (unsigned)(eob - cp), NULL, NULL);
	else
		n = dn_comp("", cp, (unsigned)(eob - cp), NULL, NULL);
	if (n < 0)
		return ISC_R_NOSPACE;
	name = cp;
	cp += n;

	/* Type, class, ttl, length (not filled in yet). */
	BOUNDS_CHECK(cp, INT16SZ + INT16SZ + INT32SZ + INT16SZ);
	PUTSHORT(ns_t_tsig, cp);
	PUTSHORT(ns_c_any, cp);
	PUTLONG(0, cp);		/* TTL */
	lenp = cp;
	cp += 2;

	/* Alg. */
	if (key != NULL && error != ns_r_badsig && error != ns_r_badkey) {
		if (key->dk_alg != KEY_HMAC_MD5)
			return ISC_R_BADKEY;
		n = dn_comp(NS_TSIG_ALG_HMAC_MD5,
			    cp, (unsigned)(eob - cp), NULL, NULL);
	}
	else
		n = dn_comp("", cp, (unsigned)(eob - cp), NULL, NULL);
	if (n < 0)
		return ISC_R_NOSPACE;
	alg = cp;
	cp += n;
	
	/* Time. */
	BOUNDS_CHECK(cp, INT16SZ + INT32SZ + INT16SZ);
	PUTSHORT(0, cp);
	timesigned = time(NULL);
	if (error != ns_r_badtime)
		PUTLONG(timesigned, cp);
	else
		PUTLONG(in_timesigned, cp);
	PUTSHORT(NS_TSIG_FUDGE, cp);

	/* Compute the signature. */
	if (key != NULL && error != ns_r_badsig && error != ns_r_badkey) {
		void *ctx;
		u_char buf[MAXDNAME], *cp2;
		unsigned n;

		dst_sign_data(SIG_MODE_INIT, key, &ctx, NULL, 0, NULL, 0);

		/* Digest the query signature, if this is a response. */
		if (querysiglen > 0 && querysig != NULL) {
			u_int16_t len_n = htons(querysiglen);
			dst_sign_data(SIG_MODE_UPDATE, key, &ctx,
				      (u_char *)&len_n, INT16SZ, NULL, 0);
			dst_sign_data(SIG_MODE_UPDATE, key, &ctx,
				      querysig, querysiglen, NULL, 0);
		}

		/* Digest the message. */
		dst_sign_data(SIG_MODE_UPDATE, key, &ctx, msg, *msglen,
			      NULL, 0);

		/* Digest the key name. */
		n = ns_name_ntol(name, buf, sizeof(buf));
		dst_sign_data(SIG_MODE_UPDATE, key, &ctx, buf, n, NULL, 0);

		/* Digest the class and TTL. */
		cp2 = buf;
		PUTSHORT(ns_c_any, cp2);
		PUTLONG(0, cp2);
		dst_sign_data(SIG_MODE_UPDATE, key, &ctx,
			      buf, (unsigned)(cp2-buf), NULL, 0);

		/* Digest the algorithm. */
		n = ns_name_ntol(alg, buf, sizeof(buf));
		dst_sign_data(SIG_MODE_UPDATE, key, &ctx, buf, n, NULL, 0);

		/* Digest the time signed, fudge, error, and other data */
		cp2 = buf;
		PUTSHORT(0, cp2);	/* Top 16 bits of time */
		if (error != ns_r_badtime)
			PUTLONG(timesigned, cp2);
		else
			PUTLONG(in_timesigned, cp2);
		PUTSHORT(NS_TSIG_FUDGE, cp2);
		PUTSHORT(error, cp2);	/* Error */
		if (error != ns_r_badtime)
			PUTSHORT(0, cp2);	/* Other data length */
		else {
			PUTSHORT(INT16SZ+INT32SZ, cp2);	/* Other data length */
			PUTSHORT(0, cp2);	/* Top 16 bits of time */
			PUTLONG(timesigned, cp2);
		}
		dst_sign_data(SIG_MODE_UPDATE, key, &ctx,
			      buf, (unsigned)(cp2-buf), NULL, 0);

		n = dst_sign_data(SIG_MODE_FINAL, key, &ctx, NULL, 0,
				  sig, *siglen);
		if (n < 0)
			return ISC_R_BADKEY;
		*siglen = n;
	} else
		*siglen = 0;

	/* Add the signature. */
	BOUNDS_CHECK(cp, INT16SZ + (*siglen));
	PUTSHORT(*siglen, cp);
	memcpy(cp, sig, *siglen);
	cp += (*siglen);

	/* The original message ID & error. */
	BOUNDS_CHECK(cp, INT16SZ + INT16SZ);
	PUTSHORT(ntohs(hp->id), cp);	/* already in network order */
	PUTSHORT(error, cp);

	/* Other data. */
	BOUNDS_CHECK(cp, INT16SZ);
	if (error != ns_r_badtime)
		PUTSHORT(0, cp);	/* Other data length */
	else {
		PUTSHORT(INT16SZ+INT32SZ, cp);	/* Other data length */
		BOUNDS_CHECK(cp, INT32SZ+INT16SZ);
		PUTSHORT(0, cp);	/* Top 16 bits of time */
		PUTLONG(timesigned, cp);
	}

	/* Go back and fill in the length. */
	PUTSHORT(cp - lenp - INT16SZ, lenp);

	hp->arcount = htons(ntohs(hp->arcount) + 1);
	*msglen = (cp - msg);
	return ISC_R_SUCCESS;
}

#if 0
isc_result_t
ns_sign_tcp_init(void *k, const u_char *querysig, unsigned querysiglen,
		 ns_tcp_tsig_state *state)
{
	dst_init();
	if (state == NULL || k == NULL || querysig == NULL || querysiglen < 0)
		return ISC_R_INVALIDARG;
	state->counter = -1;
	state->key = k;
	if (state->key->dk_alg != KEY_HMAC_MD5)
		return ISC_R_BADKEY;
	if (querysiglen > sizeof(state->sig))
		return ISC_R_NOSPACE;
	memcpy(state->sig, querysig, querysiglen);
	state->siglen = querysiglen;
	return ISC_R_SUCCESS;
}

isc_result_t
ns_sign_tcp(u_char *msg, unsigned *msglen, unsigned msgsize, int error,
	    ns_tcp_tsig_state *state, int done)
{
	u_char *cp, *eob, *lenp;
	u_char buf[MAXDNAME], *cp2;
	HEADER *hp = (HEADER *)msg;
	time_t timesigned;
	int n;

	if (msg == NULL || msglen == NULL || state == NULL)
		return ISC_R_INVALIDARG;

	state->counter++;
	if (state->counter == 0)
		return ns_sign(msg, msglen, msgsize, error, state->key,
			       state->sig, state->siglen,
			       state->sig, &state->siglen, 0);

	if (state->siglen > 0) {
		u_int16_t siglen_n = htons(state->siglen);
		dst_sign_data(SIG_MODE_INIT, state->key, &state->ctx,
			      NULL, 0, NULL, 0);
		dst_sign_data(SIG_MODE_UPDATE, state->key, &state->ctx,
			      (u_char *)&siglen_n, INT16SZ, NULL, 0);
		dst_sign_data(SIG_MODE_UPDATE, state->key, &state->ctx,
			      state->sig, state->siglen, NULL, 0);
		state->siglen = 0;
	}

	dst_sign_data(SIG_MODE_UPDATE, state->key, &state->ctx, msg, *msglen,
		      NULL, 0);

	if (done == 0 && (state->counter % 100 != 0))
		return ISC_R_SUCCESS;

	cp = msg + *msglen;
	eob = msg + msgsize;

	/* Name. */
	n = dn_comp(state->key->dk_key_name,
		    cp, (unsigned)(eob - cp), NULL, NULL);
	if (n < 0)
		return ISC_R_NOSPACE;
	cp += n;

	/* Type, class, ttl, length (not filled in yet). */
	BOUNDS_CHECK(cp, INT16SZ + INT16SZ + INT32SZ + INT16SZ);
	PUTSHORT(ns_t_tsig, cp);
	PUTSHORT(ns_c_any, cp);
	PUTLONG(0, cp);		/* TTL */
	lenp = cp;
	cp += 2;

	/* Alg. */
	n = dn_comp(NS_TSIG_ALG_HMAC_MD5,
		    cp, (unsigned)(eob - cp), NULL, NULL);
	if (n < 0)
		return ISC_R_NOSPACE;
	cp += n;
	
	/* Time. */
	BOUNDS_CHECK(cp, INT16SZ + INT32SZ + INT16SZ);
	PUTSHORT(0, cp);
	timesigned = time(NULL);
	PUTLONG(timesigned, cp);
	PUTSHORT(NS_TSIG_FUDGE, cp);

	/*
	 * Compute the signature.
	 */

	/* Digest the time signed and fudge. */
	cp2 = buf;
	PUTSHORT(0, cp2);	/* Top 16 bits of time */
	PUTLONG(timesigned, cp2);
	PUTSHORT(NS_TSIG_FUDGE, cp2);

	dst_sign_data(SIG_MODE_UPDATE, state->key, &state->ctx,
		      buf, (unsigned)(cp2 - buf), NULL, 0);

	n = dst_sign_data(SIG_MODE_FINAL, state->key, &state->ctx, NULL, 0,
			  state->sig, sizeof(state->sig));
	if (n < 0)
		return ISC_R_BADKEY;
	state->siglen = n;

	/* Add the signature. */
	BOUNDS_CHECK(cp, INT16SZ + state->siglen);
	PUTSHORT(state->siglen, cp);
	memcpy(cp, state->sig, state->siglen);
	cp += state->siglen;

	/* The original message ID & error. */
	BOUNDS_CHECK(cp, INT16SZ + INT16SZ);
	PUTSHORT(ntohs(hp->id), cp);	/* already in network order */
	PUTSHORT(error, cp);

	/* Other data. */
	BOUNDS_CHECK(cp, INT16SZ);
	PUTSHORT(0, cp);

	/* Go back and fill in the length. */
	PUTSHORT(cp - lenp - INT16SZ, lenp);

	hp->arcount = htons(ntohs(hp->arcount) + 1);
	*msglen = (cp - msg);
	return ISC_R_SUCCESS;
}
#endif
