/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/atomic.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <hidd/pci.h>

#include <devices/timer.h>

#include "ahci.h"
#include "timer.h"

void callout_init_mp(struct callout *co)
{
    memset(co, 0, sizeof(*co));
}

void callout_init(struct callout *co)
{
    callout_init_mp(co);
}

void callout_stop(struct callout *co)
{
    Forbid();
    if (co->co_Task) {
        Signal(co->co_Task, SIGF_ABORT);
        co->co_Task = NULL;
    }
    Permit();
}

void callout_stop_sync(struct callout *co)
{
    callout_stop(co);
}

static void callout_handler(struct callout *co, unsigned ticks, timeout_t *func, void *arg)
{
    struct IORequest *io;
    ULONG signals = 0;
    ULONG ms = ticks / hz;

    if ((io = ahci_OpenTimer())) {
        signals = ahci_WaitTO(io, ms, 0, SIGF_ABORT);
        ahci_CloseTimer(io);
    }

    if (!(signals & SIGF_ABORT)) {
        co->co_Task = NULL;
        func(arg);
    }
}

int callout_reset(struct callout *co, unsigned ticks, timeout_t *func, void *arg)
{
    struct Task *t;

    callout_stop(co);

    t = NewCreateTask(TASKTAG_NAME, "AHCI Callout",
                      TASKTAG_PC, callout_handler,
                      TASKTAG_PRI, 21,
                      TASKTAG_ARG1, co,
                      TASKTAG_ARG2, ticks,
                      TASKTAG_ARG3, func,
                      TASKTAG_ARG4, arg,
                      TAG_END);
    co->co_Task = t;

    return (t == NULL) ? ENOMEM : 0;
}

void	ahci_os_sleep(int ms)
{
    struct IORequest *io = ahci_OpenTimer();
    if (io != NULL) {
        ahci_WaitTO(io, ms / 1000, (ms % 1000) * 1000, 0);
        ahci_CloseTimer(io);
    }
}

void	ahci_os_hardsleep(int us)
{
    ahci_WaitNano((ULONG)us * 1000);
}

/*
 * Sleep for a minimum interval and return the number of milliseconds
 * that was.  The minimum value returned is 1
 *
 * UNIT_MICROHZ is only guaranteed to work down to 2 microseconds.
 */
int	ahci_os_softsleep(void)
{
    struct IORequest *io = ahci_OpenTimer();
    if (io != NULL) {
        ahci_WaitTO(io, 0, 100 * 1000, 0);
        ahci_CloseTimer(io);
    }
    return 100;
}

/*
 * Per-port thread helper.  This helper thread is responsible for
 * atomically retrieving and clearing the signal mask and calling
 * the machine-independant driver core.
 *
 * MPSAFE
 */
static void ahci_port_thread(void *arg)
{
	struct ahci_port *ap = arg;
	int mask;

	/*
	 * The helper thread is responsible for the initial port init,
	 * so all the ports can be inited in parallel.
	 *
	 * We also run the state machine which should do all probes.
	 * Since CAM is not attached yet we will not get out-of-order
	 * SCSI attachments.
	 */
	ahci_os_lock_port(ap);
	ahci_port_init(ap);
	atomic_clear_int(&ap->ap_signal, AP_SIGF_THREAD_SYNC);
	ahci_port_state_machine(ap, 1);
	ahci_os_unlock_port(ap);
	atomic_clear_int(&ap->ap_signal, AP_SIGF_INIT);

	/*
	 * Then loop on the helper core.
	 */
	mask = ap->ap_signal;
	while ((mask & AP_SIGF_STOP) == 0) {
		ahci_port_thread_core(ap, mask);
		// lockmgr(&ap->ap_sig_lock, LK_EXCLUSIVE);
		if (ap->ap_signal == 0)
			Wait(SIGF_DOS);
		mask = ap->ap_signal;
		atomic_clear_int(&ap->ap_signal, mask);
		// lockmgr(&ap->ap_sig_lock, LK_RELEASE);
	}
	ap->ap_thread = NULL;
}

void	ahci_os_start_port(struct ahci_port *ap)
{
	char name[16];

	atomic_set_int(&ap->ap_signal, AP_SIGF_INIT | AP_SIGF_THREAD_SYNC);
	lockinit(&ap->ap_lock, "ahcipo", 0, LK_CANRECURSE);
	lockinit(&ap->ap_sim_lock, "ahcicam", 0, LK_CANRECURSE);
	ksnprintf(name, sizeof(name), "%d", ap->ap_num);

	kthread_create(ahci_port_thread, ap, &ap->ap_thread,
		       "%s", PORTNAME(ap));
}

/*
 * Stop the OS-specific port helper thread and kill the per-port lock.
 */
void ahci_os_stop_port(struct ahci_port *ap)
{
	if (ap->ap_thread) {
		ahci_os_signal_port_thread(ap, AP_SIGF_STOP);
		ahci_os_sleep(10);
		if (ap->ap_thread) {
			kprintf("%s: Waiting for thread to terminate\n",
				PORTNAME(ap));
			while (ap->ap_thread)
				ahci_os_sleep(100);
			kprintf("%s: thread terminated\n",
				PORTNAME(ap));
		}
	}
	lockuninit(&ap->ap_lock);
}

/*
 * Add (mask) to the set of bits being sent to the per-port thread helper
 * and wake the helper up if necessary.
 *
 * We use SIGF_DOS, since these are Threads, not Processes, and
 * won't already be using SIGF_DOS for messages.
 */
void ahci_os_signal_port_thread(struct ahci_port *ap, int mask)
{
	atomic_set_int(&ap->ap_signal, mask);
	Signal(ap->ap_thread, SIGF_DOS);
}

/*
 * Unconditionally lock the port structure for access.
 */
void ahci_os_lock_port(struct ahci_port *ap)
{
	lockmgr(&ap->ap_lock, LK_EXCLUSIVE);
}

/*
 * Conditionally lock the port structure for access.
 *
 * Returns 0 on success, non-zero on failure.
 */
int ahci_os_lock_port_nb(struct ahci_port *ap)
{
	return 1;
}

/*
 * Unlock a previously locked port.
 */
void ahci_os_unlock_port(struct ahci_port *ap)
{
	lockmgr(&ap->ap_lock, LK_RELEASE);
}


