/*
 * Event loop for AmigaOS/MorphOS/AROS
 * Copyright (c) 2002-2005, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2010-2011, Neil Cafferkey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include "includes.h"

#include "common.h"
#include "eloop.h"

#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <devices/timer.h>

#include <proto/exec.h>
#include <proto/timer.h>


struct eloop_sock {
	int sock;
	void *eloop_data;
	void *user_data;
	void (*handler)(int sock, void *eloop_ctx, void *sock_ctx);
};

struct eloop_timeout {
	struct os_time time;
	void *eloop_data;
	void *user_data;
	void (*handler)(void *eloop_ctx, void *sock_ctx);
	struct eloop_timeout *next;
};

struct eloop_signal {
	int sig;
	void *user_data;
	void (*handler)(int sig, void *signal_ctx);
	int signaled;
};

struct eloop_data {
	void *user_data;

	int max_sock, reader_count;
	struct eloop_sock *readers;

	struct eloop_timeout *timeout;

	int signal_count;
	struct eloop_signal *signals;
	int signaled;
	int pending_terminate;

	int terminate;
	int reader_table_changed;

	struct MsgPort *timer_port;
	struct timerequest *timer_request;
};

static struct eloop_data eloop;


int eloop_init(void)
{
	int err = 0;

	memset(&eloop, 0, sizeof(eloop));

	/* Open timer device */

	if(err == 0)
	{
		eloop.timer_port = CreateMsgPort();
		eloop.timer_request = (APTR)CreateIORequest(eloop.timer_port,
			sizeof(struct timerequest));
		if(eloop.timer_request == NULL)
			err = 1;
	}

	if(err == 0)
	{
		if(OpenDevice((CONST_STRPTR)"timer.device", UNIT_VBLANK,
			(APTR)eloop.timer_request, 0) != 0) {
			err = 1;
			DeleteIORequest((APTR)eloop.timer_request);
			eloop.timer_request = NULL;
		} else
			eloop.timer_request->tr_node.io_Message.mn_Node.ln_Type
				= NT_REPLYMSG;
	}

	/* Use request at least once to make shutdown easier */

	if(err == 0)
	{
		eloop.timer_request->tr_node.io_Command =
			TR_ADDREQUEST;
		eloop.timer_request->tr_time.tv_secs = 0;
		eloop.timer_request->tr_time.tv_micro = 0;
		DoIO((APTR) eloop.timer_request);
	}

	return err;
}


int eloop_register_read_sock(int sock,
			     void (*handler)(int sock, void *eloop_ctx,
					     void *sock_ctx),
			     void *eloop_data, void *user_data)
{
	struct eloop_sock *tmp;

	tmp = (struct eloop_sock *)
		realloc(eloop.readers,
			(eloop.reader_count + 1) * sizeof(struct eloop_sock));
	if (tmp == NULL)
		return -1;

	tmp[eloop.reader_count].sock = sock;
	tmp[eloop.reader_count].eloop_data = eloop_data;
	tmp[eloop.reader_count].user_data = user_data;
	tmp[eloop.reader_count].handler = handler;
	eloop.reader_count++;
	eloop.readers = tmp;
	if (sock > eloop.max_sock)
		eloop.max_sock = sock;
	eloop.reader_table_changed = 1;

	return 0;
}


void eloop_unregister_read_sock(int sock)
{
	int i;

	if (eloop.readers == NULL || eloop.reader_count == 0)
		return;

	for (i = 0; i < eloop.reader_count; i++) {
		if (eloop.readers[i].sock == sock)
			break;
	}
	if (i == eloop.reader_count)
		return;
	if (i != eloop.reader_count - 1) {
		memmove(&eloop.readers[i], &eloop.readers[i + 1],
			(eloop.reader_count - i - 1) *
			sizeof(struct eloop_sock));
	}
	eloop.reader_count--;
	eloop.reader_table_changed = 1;
}


int eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   void (*handler)(void *eloop_ctx, void *timeout_ctx),
			   void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *tmp, *prev;

	timeout = (struct eloop_timeout *) malloc(sizeof(*timeout));
	if (timeout == NULL)
		return -1;
	os_get_time(&timeout->time);
	timeout->time.sec += secs;
	timeout->time.usec += usecs;
	while (timeout->time.usec >= 1000000) {
		timeout->time.sec++;
		timeout->time.usec -= 1000000;
	}
	timeout->eloop_data = eloop_data;
	timeout->user_data = user_data;
	timeout->handler = handler;
	timeout->next = NULL;

	if (eloop.timeout == NULL) {
		eloop.timeout = timeout;
		return 0;
	}

	prev = NULL;
	tmp = eloop.timeout;
	while (tmp != NULL) {
		if (os_time_before(&timeout->time, &tmp->time))
			break;
		prev = tmp;
		tmp = tmp->next;
	}

	if (prev == NULL) {
		timeout->next = eloop.timeout;
		eloop.timeout = timeout;
	} else {
		timeout->next = prev->next;
		prev->next = timeout;
	}

	return 0;
}


int eloop_cancel_timeout(void (*handler)(void *eloop_ctx, void *sock_ctx),
			 void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *prev, *next;
	int removed = 0;

	prev = NULL;
	timeout = eloop.timeout;
	while (timeout != NULL) {
		next = timeout->next;

		if (timeout->handler == handler &&
		    (timeout->eloop_data == eloop_data ||
		     eloop_data == ELOOP_ALL_CTX) &&
		    (timeout->user_data == user_data ||
		     user_data == ELOOP_ALL_CTX)) {
			if (prev == NULL)
				eloop.timeout = next;
			else
				prev->next = next;
			free(timeout);
			removed++;
		} else
			prev = timeout;

		timeout = next;
	}

	return removed;
}


int eloop_is_timeout_registered(void (*handler)(void *eloop_ctx,
						void *timeout_ctx),
				void *eloop_data, void *user_data)
{
	struct eloop_timeout *tmp;

	tmp = eloop.timeout;
	while (tmp != NULL) {
		if (tmp->handler == handler &&
		    tmp->eloop_data == eloop_data &&
		    tmp->user_data == user_data)
			return 1;

		tmp = tmp->next;
	}

	return 0;
}


static void eloop_handle_signals(u32 sigs)
{
	int i;

	eloop.signaled++;
	for (i = 0; i < eloop.signal_count; i++) {
		if (1 << eloop.signals[i].sig & sigs) {
			eloop.signals[i].signaled++;
		}
	}
}


static void eloop_process_pending_signals(void)
{
	int i;

	if (eloop.signaled == 0)
		return;
	eloop.signaled = 0;

	if (eloop.pending_terminate) {
		eloop.pending_terminate = 0;
	}

	for (i = 0; i < eloop.signal_count; i++) {
		if (eloop.signals[i].signaled) {
			eloop.signals[i].signaled = 0;
			eloop.signals[i].handler(eloop.signals[i].sig,
						 eloop.signals[i].user_data);
		}
	}
}


int eloop_register_signal(int sig,
			  void (*handler)(int sig, void *signal_ctx),
			  void *user_data)
{
	struct eloop_signal *tmp;

	tmp = (struct eloop_signal *)
		realloc(eloop.signals,
			(eloop.signal_count + 1) *
			sizeof(struct eloop_signal));
	if (tmp == NULL)
		return -1;

	tmp[eloop.signal_count].sig = sig;
	tmp[eloop.signal_count].user_data = user_data;
	tmp[eloop.signal_count].handler = handler;
	tmp[eloop.signal_count].signaled = 0;
	eloop.signal_count++;
	eloop.signals = tmp;

	return 0;
}


int eloop_register_signal_terminate(eloop_signal_handler handler,
				    void *user_data)
{
	return eloop_register_signal(SIGBREAKB_CTRL_C, handler, user_data);
}


int eloop_register_signal_reconfig(eloop_signal_handler handler,
				   void *user_data)
{
#if 0
	/* TODO: for example */
	return eloop_register_signal(SIGHUP, handler, user_data);
#endif
	return 0;
}


void eloop_run(void)
{
	int i;
	struct os_time tv, now;
	u32 sig_mask, sigs;
	struct timerequest *cur_timer_req = NULL;

	while (!eloop.terminate) {
		sig_mask = 0;

		if (eloop.timeout && cur_timer_req == NULL) {
			/* Send a timer request for the next timeout
			 * (even if it's already occurred) */
			os_get_time(&now);
			if (os_time_before(&now, &eloop.timeout->time))
				os_time_sub(&eloop.timeout->time, &now, &tv);
			else
				tv.sec = tv.usec = 0;
			eloop.timer_request->tr_node.io_Command =
				TR_ADDREQUEST;
			eloop.timer_request->tr_time.tv_secs =
				tv.sec;
			eloop.timer_request->tr_time.tv_micro =
				tv.usec;
			SendIO((APTR) eloop.timer_request);
			cur_timer_req = eloop.timer_request;
		}
		if (eloop.timeout) {
			sig_mask |= 1 << eloop.timer_port->mp_SigBit;
		}


		/* Add registered signals to signal mask */
		for (i = 0; i < eloop.signal_count; i++)
			sig_mask |= 1 << eloop.signals[i].sig;

		/* Wait for something to happen */
		sigs = Wait(sig_mask);
		eloop_handle_signals(sigs);
		eloop_process_pending_signals();

		/* Check if some registered timeouts have occurred */
		if (eloop.timeout) {
			struct eloop_timeout *tmp;

			os_get_time(&now);
			if (sigs & 1 << eloop.timer_port->mp_SigBit) {
				tmp = eloop.timeout;
				eloop.timeout = eloop.timeout->next;
				tmp->handler(tmp->eloop_data,
					     tmp->user_data);
				free(tmp);

				WaitIO((APTR) eloop.timer_request);
				cur_timer_req = NULL;
			}

		}
	}
}


void eloop_terminate(void)
{
	eloop.terminate = 1;
}


void eloop_destroy(void)
{
	struct eloop_timeout *timeout, *prev;

	timeout = eloop.timeout;
	while (timeout != NULL) {
		prev = timeout;
		timeout = timeout->next;
		free(prev);
	}
	free(eloop.signals);

	if(eloop.timer_request != NULL) {
		AbortIO((APTR) eloop.timer_request);
		WaitIO((APTR) eloop.timer_request);
		CloseDevice((APTR) eloop.timer_request);
	}
	DeleteIORequest((APTR) eloop.timer_request);
	DeleteMsgPort(eloop.timer_port);
}


int eloop_terminated(void)
{
	return eloop.terminate;
}
