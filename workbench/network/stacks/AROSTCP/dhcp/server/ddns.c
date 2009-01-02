/* ddns.c

   Dynamic DNS updates. */

/*
 * Copyright (c) 2004-2005 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 2000-2003 by Internet Software Consortium
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
 * This software has been donated to Internet Systems Consortium
 * by Damien Neil of Nominum, Inc.
 *
 * To learn more about Internet Systems Consortium, see
 * ``http://www.isc.org/''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#ifndef lint
static char copyright[] =
"$Id$ Copyright (c) 2004-2005 Internet Systems Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"
#include "dst/md5.h"
#include "minires/minires.h"

#ifdef NSUPDATE

/* DN: No way of checking that there is enough space in a data_string's
   buffer.  Be certain to allocate enough!
   TL: This is why the expression evaluation code allocates a *new*
   data_string.   :') */
static void data_string_append (struct data_string *ds1,
				struct data_string *ds2)
{
	memcpy (ds1 -> buffer -> data + ds1 -> len,
		ds2 -> data,
		ds2 -> len);
	ds1 -> len += ds2 -> len;
}

static isc_result_t ddns_update_ptr (struct data_string *ddns_fwd_name,
				     struct data_string *ddns_rev_name,
				     unsigned long ttl)
{
	ns_updque updqueue;
	ns_updrec *updrec;
	isc_result_t result = ISC_R_UNEXPECTED;

	/*
	 * The DHCP server submits a DNS query which deletes all of the PTR RRs
	 * associated with the lease IP address, and adds a PTR RR whose data
	 * is the client's (possibly disambiguated) host name. The server also
	 * adds a DHCID RR specified in Section 4.3.
	 *   -- "Interaction between DHCP and DNS"
	 */

	ISC_LIST_INIT (updqueue);

	/*
	 * Delete all PTR RRs.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_rev_name -> data,
				   C_IN, T_PTR, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)0;
	updrec -> r_size = 0;
	updrec -> r_opcode = DELETE;

	ISC_LIST_APPEND (updqueue, updrec, r_link);

	/*
	 * Add PTR RR.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_rev_name -> data,
				   C_IN, T_PTR, ttl);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = ddns_fwd_name -> data;
	updrec -> r_size = ddns_fwd_name -> len;
	updrec -> r_opcode = ADD;

	ISC_LIST_APPEND (updqueue, updrec, r_link);

	/*
	 * Attempt to perform the update.
	 */
	result = minires_nupdate (&resolver_state, ISC_LIST_HEAD (updqueue));
#if defined (DEBUG)
	print_dns_status ((int)result, &updqueue);
#endif
	if (result == ISC_R_SUCCESS) {
		log_info ("added reverse map from %.*s to %.*s",
			  (int)ddns_rev_name -> len,
			  (const char *)ddns_rev_name -> data,
			  (int)ddns_fwd_name -> len,
			  (const char *)ddns_fwd_name -> data);
	} else {
		log_error ("unable to add reverse map from %.*s to %.*s: %s",
			   (int)ddns_rev_name -> len,
			   (const char *)ddns_rev_name -> data,
			   (int)ddns_fwd_name -> len,
			   (const char *)ddns_fwd_name -> data,
			   isc_result_totext (result));
	}

	/* Fall through. */
      error:

	while (!ISC_LIST_EMPTY (updqueue)) {
		updrec = ISC_LIST_HEAD (updqueue);
		ISC_LIST_UNLINK (updqueue, updrec, r_link);
		minires_freeupdrec (updrec);
	}

	return result;
}


static isc_result_t ddns_remove_ptr (struct data_string *ddns_rev_name)
{
	ns_updque updqueue;
	ns_updrec *updrec;
	isc_result_t result;

	/*
	 * When a lease expires or a DHCP client issues a DHCPRELEASE request,
	 * the DHCP server SHOULD delete the PTR RR that matches the DHCP
	 * binding, if one was successfully added. The server's update query
	 * SHOULD assert that the name in the PTR record matches the name of
	 * the client whose lease has expired or been released.
	 *   -- "Interaction between DHCP and DNS"
	 */

	ISC_LIST_INIT (updqueue);

	/*
	 * Delete the PTR RRset for the leased address.
	 */
	updrec = minires_mkupdrec (S_UPDATE,
				   (const char *)ddns_rev_name -> data,
				   C_IN, T_PTR, 0);
	if (!updrec) {
		result = ISC_R_NOMEMORY;
		goto error;
	}

	updrec -> r_data = (unsigned char *)0;
	updrec -> r_size = 0;
	updrec -> r_opcode = DELETE;

	ISC_LIST_APPEND (updqueue, updrec, r_link);

	/*
	 * Attempt to perform the update.
	 */
	result = minires_nupdate (&resolver_state, ISC_LIST_HEAD (updqueue));
#if defined (DEBUG)
	print_dns_status ((int)result, &updqueue);
#endif
	if (result == ISC_R_SUCCESS) {
		log_info ("removed reverse map on %.*s",
			  (int)ddns_rev_name -> len,
			  (const char *)ddns_rev_name -> data);
	} else {
		if (result != ISC_R_NXRRSET && result != ISC_R_NXDOMAIN)
			log_error ("can't remove reverse map on %.*s: %s",
				   (int)ddns_rev_name -> len,
				   (const char *)ddns_rev_name -> data,
				   isc_result_totext (result));
	}

	/* Not there is success. */
	if (result == ISC_R_NXRRSET || result == ISC_R_NXDOMAIN)
		result = ISC_R_SUCCESS;

	/* Fall through. */
      error:

	while (!ISC_LIST_EMPTY (updqueue)) {
		updrec = ISC_LIST_HEAD (updqueue);
		ISC_LIST_UNLINK (updqueue, updrec, r_link);
		minires_freeupdrec (updrec);
	}

	return result;
}


int ddns_updates (struct packet *packet,
		  struct lease *lease, struct lease *old,
		  struct lease_state *state)
{
	unsigned long ddns_ttl = DEFAULT_DDNS_TTL;
	struct data_string ddns_hostname;
	struct data_string ddns_domainname;
	struct data_string old_ddns_fwd_name;
	struct data_string ddns_fwd_name;
	struct data_string ddns_rev_name;
	struct data_string ddns_dhcid;
	unsigned len;
	struct data_string d1;
	struct option_cache *oc;
	int s1, s2;
	int result = 0;
	isc_result_t rcode1 = ISC_R_SUCCESS, rcode2 = ISC_R_SUCCESS;
	int server_updates_a = 1;
	struct buffer *bp = (struct buffer *)0;
	int ignorep = 0;

	if (ddns_update_style != 2)
		return 0;

	/* Can only cope with IPv4 addrs at the moment. */
	if (lease -> ip_addr . len != 4)
		return 0;

	memset (&ddns_hostname, 0, sizeof (ddns_hostname));
	memset (&ddns_domainname, 0, sizeof (ddns_domainname));
	memset (&old_ddns_fwd_name, 0, sizeof (ddns_fwd_name));
	memset (&ddns_fwd_name, 0, sizeof (ddns_fwd_name));
	memset (&ddns_rev_name, 0, sizeof (ddns_rev_name));
	memset (&ddns_dhcid, 0, sizeof (ddns_dhcid));

	/* If we are allowed to accept the client's update of its own A
	   record, see if the client wants to update its own A record. */
	if (!(oc = lookup_option (&server_universe, state -> options,
				  SV_CLIENT_UPDATES)) ||
	    evaluate_boolean_option_cache (&ignorep, packet, lease,
					   (struct client_state *)0,
					   packet -> options,
					   state -> options,
					   &lease -> scope, oc, MDL)) {
		/* If there's no fqdn.no-client-update or if it's
		   nonzero, don't try to use the client-supplied
		   XXX */
		if (!(oc = lookup_option (&fqdn_universe, packet -> options,
					  FQDN_SERVER_UPDATE)) ||
		    evaluate_boolean_option_cache (&ignorep, packet, lease,
						   (struct client_state *)0,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL))
			goto noclient;
		/* Win98 and Win2k will happily claim to be willing to
		   update an unqualified domain name. */
		if (!(oc = lookup_option (&fqdn_universe, packet -> options,
					  FQDN_DOMAINNAME)))
			goto noclient;
		if (!(oc = lookup_option (&fqdn_universe, packet -> options,
					  FQDN_FQDN)) ||
		    !evaluate_option_cache (&ddns_fwd_name, packet, lease,
					    (struct client_state *)0,
					    packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL))
			goto noclient;
		server_updates_a = 0;
		goto client_updates;
	}
      noclient:
	/* If do-forward-updates is disabled, this basically means don't
	   do an update unless the client is participating, so if we get
	   here and do-forward-updates is disabled, we can stop. */
	if ((oc = lookup_option (&server_universe, state -> options,
				 SV_DO_FORWARD_UPDATES)) &&
	    !evaluate_boolean_option_cache (&ignorep, packet, lease,
					    (struct client_state *)0,
					    packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL)) {
		return 0;
	}

	/* If it's a static lease, then don't do the DNS update unless we're
	   specifically configured to do so.   If the client asked to do its
	   own update and we allowed that, we don't do this test. */
	if (lease -> flags & STATIC_LEASE) {
		if (!(oc = lookup_option (&server_universe, state -> options,
					  SV_UPDATE_STATIC_LEASES)) ||
		    !evaluate_boolean_option_cache (&ignorep, packet, lease,
						    (struct client_state *)0,
						    packet -> options,
						    state -> options,
						    &lease -> scope, oc, MDL))
			return 0;
	}

	/*
	 * Compute the name for the A record.
	 */
	oc = lookup_option (&server_universe, state -> options,
			    SV_DDNS_HOST_NAME);
	if (oc)
		s1 = evaluate_option_cache (&ddns_hostname, packet, lease,
					    (struct client_state *)0,
					    packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL);
	else
		s1 = 0;

	oc = lookup_option (&server_universe, state -> options,
			    SV_DDNS_DOMAIN_NAME);
	if (oc)
		s2 = evaluate_option_cache (&ddns_domainname, packet, lease,
					    (struct client_state *)0,
					    packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL);
	else
		s2 = 0;

	if (s1 && s2) {
		if (ddns_hostname.len + ddns_domainname.len > 253) {
			log_error ("ddns_update: host.domain name too long");

			goto out;
		}

		buffer_allocate (&ddns_fwd_name.buffer,
				 ddns_hostname.len + ddns_domainname.len + 2,
				 MDL);
		if (ddns_fwd_name.buffer) {
			ddns_fwd_name.data = ddns_fwd_name.buffer -> data;
			data_string_append (&ddns_fwd_name, &ddns_hostname);
			ddns_fwd_name.buffer -> data [ddns_fwd_name.len] = '.';
			ddns_fwd_name.len++;
			data_string_append (&ddns_fwd_name, &ddns_domainname);
			ddns_fwd_name.buffer -> data [ddns_fwd_name.len] ='\0';
			ddns_fwd_name.terminated = 1;
		}
	}
      client_updates:

	/* See if there's a name already stored on the lease. */
	if (find_bound_string (&old_ddns_fwd_name,
			       lease -> scope, "ddns-fwd-name")) {
		/* If there is, see if it's different. */
		if (old_ddns_fwd_name.len != ddns_fwd_name.len ||
		    memcmp (old_ddns_fwd_name.data, ddns_fwd_name.data,
			    old_ddns_fwd_name.len)) {
			/* If the name is different, try to delete
			   the old A record. */
			if (!ddns_removals (lease))
				goto out;
			/* If the delete succeeded, go install the new
			   record. */
			goto in;
		}

		/* See if there's a DHCID on the lease. */
		if (!find_bound_string (&ddns_dhcid,
					lease -> scope, "ddns-txt")) {
			/* If there's no DHCID, the update was probably
			   done with the old-style ad-hoc DDNS updates.
			   So if the expiry and release events look like
			   they're the same, run them.   This should delete
			   the old DDNS data. */
			if (old -> on_expiry == old -> on_release) {
				execute_statements ((struct binding_value **)0,
						    (struct packet *)0, lease,
						    (struct client_state *)0,
						    (struct option_state *)0,
						    (struct option_state *)0,
						    &lease -> scope,
						    old -> on_expiry);
				if (old -> on_expiry)
					executable_statement_dereference
						(&old -> on_expiry, MDL);
				if (old -> on_release)
					executable_statement_dereference
						(&old -> on_release, MDL);
				/* Now, install the DDNS data the new way. */
				goto in;
			}
		}

		/* See if the administrator wants to do updates even
		   in cases where the update already appears to have been
		   done. */
		if (!(oc = lookup_option (&server_universe, state -> options,
					  SV_UPDATE_OPTIMIZATION)) ||
		    evaluate_boolean_option_cache (&ignorep, packet, lease,
						   (struct client_state *)0,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL)) {
			result = 1;
			goto noerror;
		}
	}

	/* If there's no ddns-fwd-name on the lease, see if there's
	   a ddns-client-fqdn, indicating a prior client FQDN update.
	   If there is, and if we're still doing the client update,
	   see if the name has changed.   If it hasn't, don't do the
	   PTR update. */
	if (find_bound_string (&old_ddns_fwd_name,
			       lease -> scope, "ddns-client-fqdn")) {
		/* If the name is not different, no need to update
		   the PTR record. */
		if (old_ddns_fwd_name.len == ddns_fwd_name.len &&
		    !memcmp (old_ddns_fwd_name.data, ddns_fwd_name.data,
			     old_ddns_fwd_name.len) &&
		    (!(oc = lookup_option (&server_universe,
					   state -> options,
					   SV_UPDATE_OPTIMIZATION)) ||
		     evaluate_boolean_option_cache (&ignorep, packet, lease,
						    (struct client_state *)0,
						    packet -> options,
						    state -> options,
						    &lease -> scope, oc,
						    MDL))) {
			goto noerror;
		}
	}
      in:
		
	/* If we don't have a name that the client has been assigned, we
	   can just skip all this. */
	if (!ddns_fwd_name.len)
		goto out;

	if (ddns_fwd_name.len > 255) {
		log_error ("client provided fqdn: too long");
		goto out;
	}

	/*
	 * Compute the RR TTL.
	 */
	ddns_ttl = DEFAULT_DDNS_TTL;
	memset (&d1, 0, sizeof d1);
	if ((oc = lookup_option (&server_universe, state -> options,
				 SV_DDNS_TTL))) {
		if (evaluate_option_cache (&d1, packet, lease,
					   (struct client_state *)0,
					   packet -> options,
					   state -> options,
					   &lease -> scope, oc, MDL)) {
			if (d1.len == sizeof (u_int32_t))
				ddns_ttl = getULong (d1.data);
			data_string_forget (&d1, MDL);
		}
	}


	/*
	 * Compute the reverse IP name.
	 */
	oc = lookup_option (&server_universe, state -> options,
			    SV_DDNS_REV_DOMAIN_NAME);
	if (oc)
		s1 = evaluate_option_cache (&d1, packet, lease,
					    (struct client_state *)0,
					    packet -> options,
					    state -> options,
					    &lease -> scope, oc, MDL);
	else
		s1 = 0;

	if (s1 && (d1.len > 238)) {
		log_error ("ddns_update: Calculated rev domain name too long.");
		s1 = 0;
		data_string_forget (&d1, MDL);
	}

	if (oc && s1) {
		/* Buffer length:
		   XXX.XXX.XXX.XXX.<ddns-rev-domain-name>\0 */
		buffer_allocate (&ddns_rev_name.buffer,
				 d1.len + 17, MDL);
		if (ddns_rev_name.buffer) {
			ddns_rev_name.data = ddns_rev_name.buffer -> data;

			/* %Audit% Cannot exceed 17 bytes. %2004.06.17,Safe% */
			sprintf ((char *)ddns_rev_name.buffer -> data,
				  "%u.%u.%u.%u.",
				  lease -> ip_addr . iabuf[3] & 0xff,
				  lease -> ip_addr . iabuf[2] & 0xff,
				  lease -> ip_addr . iabuf[1] & 0xff,
				  lease -> ip_addr . iabuf[0] & 0xff);

			ddns_rev_name.len =
				strlen ((const char *)ddns_rev_name.data);
			data_string_append (&ddns_rev_name, &d1);
			ddns_rev_name.buffer -> data [ddns_rev_name.len] ='\0';
			ddns_rev_name.terminated = 1;
		}
		
		data_string_forget (&d1, MDL);
	}

	/*
	 * If we are updating the A record, compute the DHCID value.
	 */
	if (server_updates_a) {
		memset (&ddns_dhcid, 0, sizeof ddns_dhcid);
		if (lease -> uid && lease -> uid_len)
			result = get_dhcid (&ddns_dhcid,
					    DHO_DHCP_CLIENT_IDENTIFIER,
					    lease -> uid, lease -> uid_len);
		else
			result = get_dhcid (&ddns_dhcid, 0,
					    lease -> hardware_addr.hbuf,
					    lease -> hardware_addr.hlen);
		if (!result)
			goto badfqdn;
	}

	/*
	 * Start the resolver, if necessary.
	 */
	if (!resolver_inited) {
		minires_ninit (&resolver_state);
		resolver_inited = 1;
		resolver_state.retrans = 1;
		resolver_state.retry = 1;
	}

	/*
	 * Perform updates.
	 */
	if (ddns_fwd_name.len && ddns_dhcid.len)
		rcode1 = ddns_update_a (&ddns_fwd_name, lease -> ip_addr,
					&ddns_dhcid, ddns_ttl, 0);
	
	if (rcode1 == ISC_R_SUCCESS) {
		if (ddns_fwd_name.len && ddns_rev_name.len)
			rcode2 = ddns_update_ptr (&ddns_fwd_name,
						  &ddns_rev_name, ddns_ttl);
	} else
		rcode2 = rcode1;

	if (rcode1 == ISC_R_SUCCESS &&
	    (server_updates_a || rcode2 == ISC_R_SUCCESS)) {
		bind_ds_value (&lease -> scope, 
			       (server_updates_a
				? "ddns-fwd-name" : "ddns-client-fqdn"),
			       &ddns_fwd_name);
		if (server_updates_a)
			bind_ds_value (&lease -> scope, "ddns-txt",
				       &ddns_dhcid);
	}

	if (rcode2 == ISC_R_SUCCESS) {
		bind_ds_value (&lease -> scope, "ddns-rev-name",
			       &ddns_rev_name);
	}

	/* Set up the outgoing FQDN option if there was an incoming
	   FQDN option.  If there's a valid FQDN option, there should
	   be an FQDN_ENCODED suboption, so we test the latter to
	   detect the presence of the former. */
      noerror:
	if ((oc = lookup_option (&fqdn_universe,
				 packet -> options, FQDN_ENCODED))
	    && buffer_allocate (&bp, ddns_fwd_name.len + 5, MDL)) {
		bp -> data [0] = server_updates_a;
		if (!save_option_buffer (&fqdn_universe, state -> options,
					 bp, &bp -> data [0], 1,
					 &fqdn_options [FQDN_SERVER_UPDATE],
					 0))
			goto badfqdn;
		bp -> data [1] = server_updates_a;
		if (!save_option_buffer (&fqdn_universe, state -> options,
					 bp, &bp -> data [1], 1,
					 &fqdn_options [FQDN_NO_CLIENT_UPDATE],
					 0))
			goto badfqdn;
		/* Do the same encoding the client did. */
		oc = lookup_option (&fqdn_universe, packet -> options,
				    FQDN_ENCODED);
		if (oc &&
		    evaluate_boolean_option_cache (&ignorep, packet, lease,
						   (struct client_state *)0,
						   packet -> options,
						   state -> options,
						   &lease -> scope, oc, MDL))
			bp -> data [2] = 1;
		else
			bp -> data [2] = 0;
		if (!save_option_buffer (&fqdn_universe, state -> options,
					 bp, &bp -> data [2], 1,
					 &fqdn_options [FQDN_ENCODED],
					 0))
			goto badfqdn;
		bp -> data [3] = isc_rcode_to_ns (rcode1);
		if (!save_option_buffer (&fqdn_universe, state -> options,
					 bp, &bp -> data [3], 1,
					 &fqdn_options [FQDN_RCODE1],
					 0))
			goto badfqdn;
		bp -> data [4] = isc_rcode_to_ns (rcode2);
		if (!save_option_buffer (&fqdn_universe, state -> options,
					 bp, &bp -> data [4], 1,
					 &fqdn_options [FQDN_RCODE2],
					 0))
			goto badfqdn;
		if (ddns_fwd_name.len) {
		    memcpy (&bp -> data [5],
			    ddns_fwd_name.data, ddns_fwd_name.len);
		    if (!save_option_buffer (&fqdn_universe, state -> options,
					     bp, &bp -> data [5],
					     ddns_fwd_name.len,
					     &fqdn_options [FQDN_FQDN],
					     0))
			goto badfqdn;
		}
	}

      badfqdn:
      out:
	/*
	 * Final cleanup.
	 */
	data_string_forget (&ddns_hostname, MDL);
	data_string_forget (&ddns_domainname, MDL);
	data_string_forget (&old_ddns_fwd_name, MDL);
	data_string_forget (&ddns_fwd_name, MDL);
	data_string_forget (&ddns_rev_name, MDL);
	data_string_forget (&ddns_dhcid, MDL);
	if (bp)
		buffer_dereference (&bp, MDL);

	return result;
}

int ddns_removals (struct lease *lease)
{
	struct data_string ddns_fwd_name;
	struct data_string ddns_rev_name;
	struct data_string ddns_dhcid;
	isc_result_t rcode;
	struct binding *binding;
	int result = 0;
	int client_updated = 0;

	/* No scope implies that DDNS has not been performed for this lease. */
	if (!lease -> scope)
		return 0;

	if (ddns_update_style != 2)
		return 0;

	/*
	 * Look up stored names.
	 */
	memset (&ddns_fwd_name, 0, sizeof (ddns_fwd_name));
	memset (&ddns_rev_name, 0, sizeof (ddns_rev_name));
	memset (&ddns_dhcid, 0, sizeof (ddns_dhcid));

	/*
	 * Start the resolver, if necessary.
	 */
	if (!resolver_inited) {
		minires_ninit (&resolver_state);
		resolver_inited = 1;
		resolver_state.retrans = 1;
		resolver_state.retry = 1;
	}

	/* We need the fwd name whether we are deleting both records or just
	   the PTR record, so if it's not there, we can't proceed. */
	if (!find_bound_string (&ddns_fwd_name,
				lease -> scope, "ddns-fwd-name")) {
		/* If there's no ddns-fwd-name, look for the client fqdn,
		   in case the client did the update. */
		if (!find_bound_string (&ddns_fwd_name,
					lease -> scope, "ddns-client-fqdn"))
			goto try_rev;
		client_updated = 1;
		goto try_rev;
	}

	/* If the ddns-txt binding isn't there, this isn't an interim
	   or rfc3??? record, so we can't delete the A record using
	   this mechanism, but we can delete the PTR record. */
	if (!find_bound_string (&ddns_dhcid, lease -> scope, "ddns-txt")) {
		result = 1;
		goto try_rev;
	}

	/*
	 * Perform removals.
	 */
	if (ddns_fwd_name.len)
		rcode = ddns_remove_a (&ddns_fwd_name,
				       lease -> ip_addr, &ddns_dhcid);
	else
		rcode = ISC_R_SUCCESS;

	if (rcode == ISC_R_SUCCESS) {
		result = 1;
		unset (lease -> scope, "ddns-fwd-name");
		unset (lease -> scope, "ddns-txt");
	      try_rev:
		if (find_bound_string (&ddns_rev_name,
				       lease -> scope, "ddns-rev-name")) {
			if (ddns_remove_ptr(&ddns_rev_name) == NOERROR) {
				unset (lease -> scope, "ddns-rev-name");
				if (client_updated)
					unset (lease -> scope,
					       "ddns-client-fqdn");
				/* XXX this is to compensate for a bug in
				   XXX 3.0rc8, and should be removed before
				   XXX 3.0pl1. */
				else if (!ddns_fwd_name.len)
					unset (lease -> scope, "ddns-text");
			} else
				result = 0;
		}
	}

	data_string_forget (&ddns_fwd_name, MDL);
	data_string_forget (&ddns_rev_name, MDL);
	data_string_forget (&ddns_dhcid, MDL);

	return result;
}

#endif /* NSUPDATE */
