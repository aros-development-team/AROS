/*
 * Copyright (C) 2012-2023, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
 
#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <aros/atomic.h>
#include <hidd/pci.h>
#include <interface/Hidd_PCIDevice.h>
#include <devices/timer.h>

#include <string.h>

#include "ahci.h"
#include "timer.h"

/* Callout Support Functions */
void callout_init_mp(struct callout *co)
{
#if (0)
    D(bug("[AHCI] %s()\n", __func__));
#endif
    memset(co, 0, sizeof(*co));
}

void callout_init(struct callout *co)
{
#if (0)
    D(bug("[AHCI] %s()\n", __func__));
#endif
    callout_init_mp(co);
}

void callout_stop(struct callout *co)
{
#if (0)
    D(bug("[AHCI] %s()\n", __func__));
#endif
    Forbid();
    if (co->co_Task) {
        Signal(co->co_Task, SIGF_ABORT);
        co->co_Task = NULL;
    }
    Permit();
}

void callout_cancel(struct callout *co)
{
#if (0)
    D(bug("[AHCI] %s()\n", __func__));
#endif
    callout_stop(co);
}

static void callout_handler(struct callout *co, unsigned ticks, timeout_t *func, void *arg)
{
    struct IORequest *io;
    ULONG signals = 0;
    ULONG ms = ticks;

    if ((io = ahci_OpenTimer())) {
        signals = ahci_WaitTO(io, ms / 1000, 1000 * (ms % 1000), SIGF_ABORT);
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
#if (0)
    D(bug("[AHCI] %s()\n", __func__));
#endif

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

/* IRQ Support Functions */
int pci_alloc_1intr(device_t dev, int msi_enable,
            int *rid0, u_int *irq_flags)
{
    if ((msi_enable) && (dev->dev_Object))
    {
        struct AHCIBase *AHCIBase = dev->dev_Base;
        OOP_MethodID HiddPCIDeviceBase = AHCIBase->ahci_HiddPCIDeviceMethodBase;

        struct TagItem vectreqs[] =
        {
            { tHidd_PCIVector_Min,      1                       },
            { tHidd_PCIVector_Max,      1                       },
            { TAG_DONE,                 0                       }
        };
        if (HIDD_PCIDevice_ObtainVectors(dev->dev_Object, vectreqs))
        {
            struct TagItem vecAttribs[] =
            {
                            {   tHidd_PCIVector_Int,    (IPTR)-1        },
                            {   TAG_DONE,               0               }
            };

            HIDD_PCIDevice_GetVectorAttribs(dev->dev_Object, 0, vecAttribs);
            *rid0 = vecAttribs[0].ti_Data;
            *irq_flags = RF_ACTIVE;

            return 1;
        }
    }
    *rid0 = AHCI_IRQ_RID;
    *irq_flags = RF_SHAREABLE | RF_ACTIVE;

    return 0;
}

/* AHCI Support Functions */
void    ahci_os_sleep(int ms)
{
    struct IORequest *io = ahci_OpenTimer();
    if (io != NULL) {
        ahci_WaitTO(io, ms / 1000, (ms % 1000) * 1000, 0);
        ahci_CloseTimer(io);
    }
}

void    ahci_os_hardsleep(int us)
{
    ahci_WaitNano((ULONG)us * 1000);
}

/*
 * Sleep for a minimum interval and return the number of milliseconds
 * that was.  The minimum value returned is 1
 *
 * UNIT_MICROHZ is only guaranteed to work down to 2 microseconds.
 */
int     ahci_os_softsleep(void)
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
    struct AHCIBase *AHCIBase = ap->ap_sc->sc_dev->dev_Base;

    int mask;

    ahciDebug("[AHCI] %s()\n", __func__);

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

void    ahci_os_start_port(struct ahci_port *ap)
{
    struct AHCIBase *AHCIBase = ap->ap_sc->sc_dev->dev_Base;

    char name[16];

    ahciDebug("[AHCI] %s()\n", __func__);

    atomic_set_int(&ap->ap_signal, AP_SIGF_INIT | AP_SIGF_THREAD_SYNC);
    lockinit(&ap->ap_lock, "ahcipo", 0, LK_CANRECURSE);
    lockinit(&ap->ap_sim_lock, "ahcicam", 0, LK_CANRECURSE);
    lockinit(&ap->ap_sig_lock, "ahport", 0, 0);
    ksnprintf(name, sizeof(name), "%d", ap->ap_num);

    kthread_create(ahci_port_thread, ap, &ap->ap_thread,
                   "%s", PORTNAME(ap));
}

/*
 * Stop the OS-specific port helper thread and kill the per-port lock.
 */
void ahci_os_stop_port(struct ahci_port *ap)
{
    struct AHCIBase *AHCIBase = ap->ap_sc->sc_dev->dev_Base;

    ahciDebug("[AHCI] %s()\n", __func__);
    if (ap->ap_thread) {
            ahci_os_signal_port_thread(ap, AP_SIGF_STOP);
            ahci_os_sleep(10);
            if (ap->ap_thread) {
                    ahciDebug("%s: Waiting for thread to terminate\n",
                            PORTNAME(ap));
                    while (ap->ap_thread)
                            ahci_os_sleep(100);
                    ahciDebug("%s: thread terminated\n",
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
    struct AHCIBase *AHCIBase = ap->ap_sc->sc_dev->dev_Base;

    ahciDebug("[AHCI] %s()\n", __func__);
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


