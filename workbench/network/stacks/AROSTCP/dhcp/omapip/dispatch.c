/* dispatch.c

   I/O dispatcher. */

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
 *
 * This software has been written for Internet Systems Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about Internet Systems Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#include <omapip/omapip_p.h>

static omapi_io_object_t omapi_io_states;
TIME cur_time;

OMAPI_OBJECT_ALLOC (omapi_io,
		    omapi_io_object_t, omapi_type_io_object)
OMAPI_OBJECT_ALLOC (omapi_waiter,
		    omapi_waiter_object_t, omapi_type_waiter)

/* Register an I/O handle so that we can do asynchronous I/O on it. */

isc_result_t omapi_register_io_object (omapi_object_t *h,
				       int (*readfd) (omapi_object_t *),
				       int (*writefd) (omapi_object_t *),
				       isc_result_t (*reader)
						(omapi_object_t *),
				       isc_result_t (*writer)
						(omapi_object_t *),
				       isc_result_t (*reaper)
						(omapi_object_t *))
{
	isc_result_t status;
	omapi_io_object_t *obj, *p;

	/* omapi_io_states is a static object.   If its reference count
	   is zero, this is the first I/O handle to be registered, so
	   we need to initialize it.   Because there is no inner or outer
	   pointer on this object, and we're setting its refcnt to 1, it
	   will never be freed. */
	if (!omapi_io_states.refcnt) {
		omapi_io_states.refcnt = 1;
		omapi_io_states.type = omapi_type_io_object;
	}
		
	obj = (omapi_io_object_t *)0;
	status = omapi_io_allocate (&obj, MDL);
	if (status != ISC_R_SUCCESS)
		return status;

	status = omapi_object_reference (&obj -> inner, h, MDL);
	if (status != ISC_R_SUCCESS) {
		omapi_io_dereference (&obj, MDL);
		return status;
	}

	status = omapi_object_reference (&h -> outer,
					 (omapi_object_t *)obj, MDL);
	if (status != ISC_R_SUCCESS) {
		omapi_io_dereference (&obj, MDL);
		return status;
	}

	/* Find the last I/O state, if there are any. */
	for (p = omapi_io_states.next;
	     p && p -> next; p = p -> next)
		;
	if (p)
		omapi_io_reference (&p -> next, obj, MDL);
	else
		omapi_io_reference (&omapi_io_states.next, obj, MDL);

	obj -> readfd = readfd;
	obj -> writefd = writefd;
	obj -> reader = reader;
	obj -> writer = writer;
	obj -> reaper = reaper;
	return ISC_R_SUCCESS;
}

isc_result_t omapi_unregister_io_object (omapi_object_t *h)
{
	omapi_io_object_t *p, *obj, *last, *ph;

	if (!h -> outer || h -> outer -> type != omapi_type_io_object)
		return ISC_R_INVALIDARG;
	obj = (omapi_io_object_t *)h -> outer;
	ph = (omapi_io_object_t *)0;
	omapi_io_reference (&ph, obj, MDL);

	/* remove from the list of I/O states */
        last = &omapi_io_states;
	for (p = omapi_io_states.next; p; p = p -> next) {
		if (p == obj) {
			omapi_io_dereference (&last -> next, MDL);
			omapi_io_reference (&last -> next, p -> next, MDL);
			break;
		}
		last = p;
	}
	if (obj -> next)
		omapi_io_dereference (&obj -> next, MDL);

	if (obj -> outer) {
		if (obj -> outer -> inner == (omapi_object_t *)obj)
			omapi_object_dereference (&obj -> outer -> inner,
						  MDL);
		omapi_object_dereference (&obj -> outer, MDL);
	}
	omapi_object_dereference (&obj -> inner, MDL);
	omapi_object_dereference (&h -> outer, MDL);
	omapi_io_dereference (&ph, MDL);
	return ISC_R_SUCCESS;
}

isc_result_t omapi_dispatch (struct timeval *t)
{
	return omapi_wait_for_completion ((omapi_object_t *)&omapi_io_states,
					  t);
}

isc_result_t omapi_wait_for_completion (omapi_object_t *object,
					struct timeval *t)
{
	isc_result_t status;
	omapi_waiter_object_t *waiter;
	omapi_object_t *inner;

	if (object) {
		waiter = (omapi_waiter_object_t *)0;
		status = omapi_waiter_allocate (&waiter, MDL);
		if (status != ISC_R_SUCCESS)
			return status;

		/* Paste the waiter object onto the inner object we're
		   waiting on. */
		for (inner = object; inner -> inner; inner = inner -> inner)
			;

		status = omapi_object_reference (&waiter -> outer, inner, MDL);
		if (status != ISC_R_SUCCESS) {
			omapi_waiter_dereference (&waiter, MDL);
			return status;
		}
		
		status = omapi_object_reference (&inner -> inner,
						 (omapi_object_t *)waiter,
						 MDL);
		if (status != ISC_R_SUCCESS) {
			omapi_waiter_dereference (&waiter, MDL);
			return status;
		}
	} else
		waiter = (omapi_waiter_object_t *)0;

	do {
		status = omapi_one_dispatch ((omapi_object_t *)waiter, t);
		if (status != ISC_R_SUCCESS)
			return status;
	} while (!waiter || !waiter -> ready);

	if (waiter -> outer) {
		if (waiter -> outer -> inner) {
			omapi_object_dereference (&waiter -> outer -> inner,
						  MDL);
			if (waiter -> inner)
				omapi_object_reference
					(&waiter -> outer -> inner,
					 waiter -> inner, MDL);
		}
		omapi_object_dereference (&waiter -> outer, MDL);
	}
	if (waiter -> inner)
		omapi_object_dereference (&waiter -> inner, MDL);
	
	status = waiter -> waitstatus;
	omapi_waiter_dereference (&waiter, MDL);
	return status;
}

isc_result_t omapi_one_dispatch (omapi_object_t *wo,
				 struct timeval *t)
{
	fd_set r, w, x;
	int max = 0;
	int count;
	int desc;
	struct timeval now, to;
	omapi_io_object_t *io, *prev;
	omapi_waiter_object_t *waiter;
	omapi_object_t *tmp = (omapi_object_t *)0;

	if (!wo || wo -> type != omapi_type_waiter)
		waiter = (omapi_waiter_object_t *)0;
	else
		waiter = (omapi_waiter_object_t *)wo;

	FD_ZERO (&x);

	/* First, see if the timeout has expired, and if so return. */
	if (t) {
		gettimeofday (&now, (struct timezone *)0);
		cur_time = now.tv_sec;
		if (now.tv_sec > t -> tv_sec ||
		    (now.tv_sec == t -> tv_sec && now.tv_usec >= t -> tv_usec))
			return ISC_R_TIMEDOUT;
			
		/* We didn't time out, so figure out how long until
		   we do. */
		to.tv_sec = t -> tv_sec - now.tv_sec;
		to.tv_usec = t -> tv_usec - now.tv_usec;
		if (to.tv_usec < 0) {
			to.tv_usec += 1000000;
			to.tv_sec--;
		}

		/* It is possible for the timeout to get set larger than
		   the largest time select() is willing to accept.
		   Restricting the timeout to a maximum of one day should
		   work around this.  -DPN.  (Ref: Bug #416) */
		if (to.tv_sec > (60 * 60 * 24))
			to.tv_sec = 60 * 60 * 24;
	}
	
	/* If the object we're waiting on has reached completion,
	   return now. */
	if (waiter && waiter -> ready)
		return ISC_R_SUCCESS;
	
      again:
	/* If we have no I/O state, we can't proceed. */
	if (!(io = omapi_io_states.next))
		return ISC_R_NOMORE;

	/* Set up the read and write masks. */
	FD_ZERO (&r);
	FD_ZERO (&w);

	for (; io; io = io -> next) {
		/* Check for a read socket.   If we shouldn't be
		   trying to read for this I/O object, either there
		   won't be a readfd function, or it'll return -1. */
		if (io -> readfd && io -> inner &&
		    (desc = (*(io -> readfd)) (io -> inner)) >= 0) {
			FD_SET (desc, &r);
			if (desc > max)
				max = desc;
		}
		
		/* Same deal for write fdets. */
		if (io -> writefd && io -> inner &&
		    (desc = (*(io -> writefd)) (io -> inner)) >= 0) {
			FD_SET (desc, &w);
			if (desc > max)
				max = desc;
		}
	}

	/* Wait for a packet or a timeout... XXX */
#if 0
#if defined (__linux__)
#define fds_bits __fds_bits
#endif
	log_error ("dispatch: %d %lx %lx", max,
		   (unsigned long)r.fds_bits [0],
		   (unsigned long)w.fds_bits [0]);
#endif
	count = select (max + 1, &r, &w, &x, t ? &to : (struct timeval *)0);
#ifdef HAVE_CHKABORT
	__chkabort();
#else
	if (Errno() == EINTR)
		exit(0);
#endif

	/* Get the current time... */
	gettimeofday (&now, (struct timezone *)0);
	cur_time = now.tv_sec;

	/* We probably have a bad file descriptor.   Figure out which one.
	   When we find it, call the reaper function on it, which will
	   maybe make it go away, and then try again. */
	if (count < 0) {
		struct timeval t0;
		omapi_io_object_t *prev = (omapi_io_object_t *)0;
		io = (omapi_io_object_t *)0;
		if (omapi_io_states.next)
			omapi_io_reference (&io, omapi_io_states.next, MDL);

		while (io) {
			omapi_object_t *obj;
			FD_ZERO (&r);
			FD_ZERO (&w);
			t0.tv_sec = t0.tv_usec = 0;

			if (io -> readfd && io -> inner &&
			    (desc = (*(io -> readfd)) (io -> inner)) >= 0) {
			    FD_SET (desc, &r);
#if 0
			    log_error ("read check: %d %lx %lx", max,
				       (unsigned long)r.fds_bits [0],
				       (unsigned long)w.fds_bits [0]);
#endif
			    count = select (desc + 1, &r, &w, &x, &t0);
			   bogon:
			    if (count < 0) {
				log_error ("Bad descriptor %d.", desc);
				for (obj = (omapi_object_t *)io;
				     obj -> outer;
				     obj = obj -> outer)
					;
				for (; obj; obj = obj -> inner) {
				    omapi_value_t *ov;
				    int len;
				    const char *s;
				    ov = (omapi_value_t *)0;
				    omapi_get_value_str (obj,
							 (omapi_object_t *)0,
							 "name", &ov);
				    if (ov && ov -> value &&
					(ov -> value -> type ==
					 omapi_datatype_string)) {
					s = (char *)
						ov -> value -> u.buffer.value;
					len = ov -> value -> u.buffer.len;
				    } else {
					s = "";
					len = 0;
				    }
				    log_error ("Object %lx %s%s%.*s",
					       (unsigned long)obj,
					       obj -> type -> name,
					       len ? " " : "",
					       len, s);
				    if (len)
					omapi_value_dereference (&ov, MDL);
				}
				(*(io -> reaper)) (io -> inner);
				if (prev) {
				    omapi_io_dereference (&prev -> next, MDL);
				    if (io -> next)
					omapi_io_reference (&prev -> next,
							    io -> next, MDL);
				} else {
				    omapi_io_dereference
					    (&omapi_io_states.next, MDL);
				    if (io -> next)
					omapi_io_reference
						(&omapi_io_states.next,
						 io -> next, MDL);
				}
				omapi_io_dereference (&io, MDL);
				goto again;
			    }
			}
			
			FD_ZERO (&r);
			FD_ZERO (&w);
			t0.tv_sec = t0.tv_usec = 0;

			/* Same deal for write fdets. */
			if (io -> writefd && io -> inner &&
			    (desc = (*(io -> writefd)) (io -> inner)) >= 0) {
				FD_SET (desc, &w);
				count = select (desc + 1, &r, &w, &x, &t0);
				if (count < 0)
					goto bogon;
			}
			if (prev)
				omapi_io_dereference (&prev, MDL);
			omapi_io_reference (&prev, io, MDL);
			omapi_io_dereference (&io, MDL);
			if (prev -> next)
			    omapi_io_reference (&io, prev -> next, MDL);
		}
		if (prev)
			omapi_io_dereference (&prev, MDL);
		
	}

	for (io = omapi_io_states.next; io; io = io -> next) {
		if (!io -> inner)
			continue;
		omapi_object_reference (&tmp, io -> inner, MDL);
		/* Check for a read descriptor, and if there is one,
		   see if we got input on that socket. */
		if (io -> readfd &&
		    (desc = (*(io -> readfd)) (tmp)) >= 0) {
			if (FD_ISSET (desc, &r))
				((*(io -> reader)) (tmp));
		}
		
		/* Same deal for write descriptors. */
		if (io -> writefd &&
		    (desc = (*(io -> writefd)) (tmp)) >= 0)
		{
			if (FD_ISSET (desc, &w))
				((*(io -> writer)) (tmp));
		}
		omapi_object_dereference (&tmp, MDL);
	}

	/* Now check for I/O handles that are no longer valid,
	   and remove them from the list. */
	prev = (omapi_io_object_t *)0;
	for (io = omapi_io_states.next; io; io = io -> next) {
		if (io -> reaper) {
			if (!io -> inner ||
			    ((*(io -> reaper)) (io -> inner) !=
							ISC_R_SUCCESS)) {
				omapi_io_object_t *tmp =
					(omapi_io_object_t *)0;
				/* Save a reference to the next
				   pointer, if there is one. */
				if (io -> next)
					omapi_io_reference (&tmp,
							    io -> next, MDL);
				if (prev) {
					omapi_io_dereference (&prev -> next,
							      MDL);
					if (tmp)
						omapi_io_reference
							(&prev -> next,
							 tmp, MDL);
				} else {
					omapi_io_dereference
						(&omapi_io_states.next, MDL);
					if (tmp)
						omapi_io_reference
						    (&omapi_io_states.next,
						     tmp, MDL);
					else
						omapi_signal_in
							((omapi_object_t *)
							 &omapi_io_states,
							 "ready");
				}
				if (tmp)
					omapi_io_dereference (&tmp, MDL);
			}
		}
		prev = io;
	}

	return ISC_R_SUCCESS;
}

isc_result_t omapi_io_set_value (omapi_object_t *h,
				 omapi_object_t *id,
				 omapi_data_string_t *name,
				 omapi_typed_data_t *value)
{
	if (h -> type != omapi_type_io_object)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> set_value)
		return (*(h -> inner -> type -> set_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t omapi_io_get_value (omapi_object_t *h,
				 omapi_object_t *id,
				 omapi_data_string_t *name,
				 omapi_value_t **value)
{
	if (h -> type != omapi_type_io_object)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> get_value)
		return (*(h -> inner -> type -> get_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

/* omapi_io_destroy (object, MDL);
 *
 *	Find the requsted IO [object] and remove it from the list of io
 * states, causing the cleanup functions to destroy it.  Note that we must
 * hold a reference on the object while moving its ->next reference and
 * removing the reference in the chain to the target object...otherwise it
 * may be cleaned up from under us.
 */
isc_result_t omapi_io_destroy (omapi_object_t *h, const char *file, int line)
{
	omapi_io_object_t *obj = NULL, *p, *last = NULL, **holder;

	if (h -> type != omapi_type_io_object)
		return ISC_R_INVALIDARG;
	
	/* remove from the list of I/O states */
	for (p = omapi_io_states.next; p; p = p -> next) {
		if (p == (omapi_io_object_t *)h) {
			omapi_io_reference (&obj, p, MDL);

			if (last)
				holder = &last -> next;
			else
				holder = &omapi_io_states.next;

			omapi_io_dereference (holder, MDL);

			if (obj -> next) {
				omapi_io_reference (holder, obj -> next, MDL);
				omapi_io_dereference (&obj -> next, MDL);
			}

			return omapi_io_dereference (&obj, MDL);
		}
		last = p;
	}

	return ISC_R_NOTFOUND;
}

isc_result_t omapi_io_signal_handler (omapi_object_t *h,
				      const char *name, va_list ap)
{
	if (h -> type != omapi_type_io_object)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> signal_handler)
		return (*(h -> inner -> type -> signal_handler)) (h -> inner,
								  name, ap);
	return ISC_R_NOTFOUND;
}

isc_result_t omapi_io_stuff_values (omapi_object_t *c,
				    omapi_object_t *id,
				    omapi_object_t *i)
{
	if (i -> type != omapi_type_io_object)
		return ISC_R_INVALIDARG;

	if (i -> inner && i -> inner -> type -> stuff_values)
		return (*(i -> inner -> type -> stuff_values)) (c, id,
								i -> inner);
	return ISC_R_SUCCESS;
}

isc_result_t omapi_waiter_signal_handler (omapi_object_t *h,
					  const char *name, va_list ap)
{
	omapi_waiter_object_t *waiter;

	if (h -> type != omapi_type_waiter)
		return ISC_R_INVALIDARG;
	
	if (!strcmp (name, "ready")) {
		waiter = (omapi_waiter_object_t *)h;
		waiter -> ready = 1;
		waiter -> waitstatus = ISC_R_SUCCESS;
		return ISC_R_SUCCESS;
	}

	if (!strcmp (name, "status")) {
		waiter = (omapi_waiter_object_t *)h;
		waiter -> ready = 1;
		waiter -> waitstatus = va_arg (ap, isc_result_t);
		return ISC_R_SUCCESS;
	}

	if (!strcmp (name, "disconnect")) {
		waiter = (omapi_waiter_object_t *)h;
		waiter -> ready = 1;
		waiter -> waitstatus = ISC_R_CONNRESET;
		return ISC_R_SUCCESS;
	}

	if (h -> inner && h -> inner -> type -> signal_handler)
		return (*(h -> inner -> type -> signal_handler)) (h -> inner,
								  name, ap);
	return ISC_R_NOTFOUND;
}

isc_result_t omapi_io_state_foreach (isc_result_t (*func) (omapi_object_t *,
							   void *),
				     void *p)
{
	omapi_io_object_t *io;
	isc_result_t status;

	for (io = omapi_io_states.next; io; io = io -> next) {
		if (io -> inner) {
			status = (*func) (io -> inner, p);
			if (status != ISC_R_SUCCESS)
				return status;
		}
	}
	return ISC_R_SUCCESS;
}
