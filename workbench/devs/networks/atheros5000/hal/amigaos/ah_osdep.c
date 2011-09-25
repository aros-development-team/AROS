/*
 * Copyright (c) 2002-2008 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2008 Atheros Communications, Inc.
 * Copyright (c) 2010      Neil Cafferkey
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id$
 */
#include "opt_ah.h"

#ifndef EXPORT_SYMTAB
#define	EXPORT_SYMTAB
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/timer.h>

#include "ah.h"

#ifdef AH_DEBUG
static	int ath_hal_debug = 0;
#endif

int	ath_hal_dma_beacon_response_time = 2;	/* in TU's */
int	ath_hal_sw_beacon_response_time = 10;	/* in TU's */
int	ath_hal_additional_swba_backoff = 0;	/* in TU's */

#if 0
struct ath_hal *
_ath_hal_attach(uint16_t devid, HAL_SOFTC sc,
		HAL_BUS_TAG t, HAL_BUS_HANDLE h, void* s)
{
	HAL_STATUS status;
	struct ath_hal *ah = ath_hal_attach(devid, sc, t, h, &status);

	*(HAL_STATUS *)s = status;
	return ah;
}

void
ath_hal_detach(struct ath_hal *ah)
{
	(*ah->ah_detach)(ah);
}
#endif

/*
 * Print/log message support.
 */

void __ahdecl
ath_hal_vprintf(struct ath_hal *ah, const char* fmt, va_list ap)
{
	char buf[1024];					/* XXX */
	vsnprintf(buf, sizeof(buf), fmt, ap);
//	printk("%s", buf);
}

void __ahdecl
ath_hal_printf(struct ath_hal *ah, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	ath_hal_vprintf(ah, fmt, ap);
	va_end(ap);
}
//EXPORT_SYMBOL(ath_hal_printf);

/*
 * Format an Ethernet MAC for printing.
 */
const char* __ahdecl
ath_hal_ether_sprintf(const uint8_t *mac)
{
	static char etherbuf[18];
	snprintf(etherbuf, sizeof(etherbuf), "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return etherbuf;
}

#ifdef AH_ASSERT
void __ahdecl
ath_hal_assert_failed(const char* filename, int lineno, const char *msg)
{
	printk("Atheros HAL assertion failure: %s: line %u: %s\n",
		filename, lineno, msg);
	panic("ath_hal_assert");
}
#endif /* AH_ASSERT */

#ifdef AH_DEBUG_ALQ
/*
 * ALQ register tracing support.
 *
 * Setting hw.ath.hal.alq=1 enables tracing of all register reads and
 * writes to the file /tmp/ath_hal.log.  The file format is a simple
 * fixed-size array of records.  When done logging set hw.ath.hal.alq=0
 * and then decode the file with the ardecode program (that is part of the
 * HAL).  If you start+stop tracing the data will be appended to an
 * existing file.
 *
 * NB: doesn't handle multiple devices properly; only one DEVICE record
 *     is emitted and the different devices are not identified.
 */
#include "alq/alq.h"
#include "ah_decode.h"

static	struct alq *ath_hal_alq;
static	int ath_hal_alq_emitdev;	/* need to emit DEVICE record */
static	u_int ath_hal_alq_lost;		/* count of lost records */
static	const char *ath_hal_logfile = "/tmp/ath_hal.log";
static	u_int ath_hal_alq_qsize = 8*1024;

static int
ath_hal_setlogging(int enable)
{
	int error;

	if (enable) {
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		error = alq_open(&ath_hal_alq, ath_hal_logfile,
				sizeof (struct athregrec), ath_hal_alq_qsize);
		ath_hal_alq_lost = 0;
		ath_hal_alq_emitdev = 1;
		printk("ath_hal: logging to %s %s\n", ath_hal_logfile,
			error == 0 ? "enabled" : "could not be setup");
	} else {
		if (ath_hal_alq)
			alq_close(ath_hal_alq);
		ath_hal_alq = NULL;
		printk("ath_hal: logging disabled\n");
		error = 0;
	}
	return error;
}

/*
 * Deal with the sysctl handler api changing.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,8)
#define	AH_SYSCTL_ARGS_DECL \
	ctl_table *ctl, int write, struct file *filp, void *buffer, \
		size_t *lenp
#define	AH_SYSCTL_ARGS		ctl, write, filp, buffer, lenp
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,8) */
#define	AH_SYSCTL_ARGS_DECL \
	ctl_table *ctl, int write, struct file *filp, void *buffer,\
		size_t *lenp, loff_t *ppos
#define	AH_SYSCTL_ARGS		ctl, write, filp, buffer, lenp, ppos
#endif

static int
sysctl_hw_ath_hal_log(AH_SYSCTL_ARGS_DECL)
{
	int error, enable;

	ctl->data = &enable;
	ctl->maxlen = sizeof(enable);
	enable = (ath_hal_alq != NULL);
        error = proc_dointvec(AH_SYSCTL_ARGS);
        if (error || !write)
                return error;
	else
		return ath_hal_setlogging(enable);
}

static struct ale *
ath_hal_alq_get(struct ath_hal *ah)
{
	struct ale *ale;

	if (ath_hal_alq_emitdev) {
		ale = alq_get(ath_hal_alq, ALQ_NOWAIT);
		if (ale) {
			struct athregrec *r =
				(struct athregrec *) ale->ae_data;
			r->op = OP_DEVICE;
			r->reg = 0;
			r->val = ah->ah_devid;
			alq_post(ath_hal_alq, ale);
			ath_hal_alq_emitdev = 0;
		} else
			ath_hal_alq_lost++;
	}
	ale = alq_get(ath_hal_alq, ALQ_NOWAIT);
	if (!ale)
		ath_hal_alq_lost++;
	return ale;
}

void __ahdecl
ath_hal_reg_write(struct ath_hal *ah, uint32_t reg, uint32_t val)
{
	if (ath_hal_alq) {
		unsigned long flags;
		struct ale *ale;

		local_irq_save(flags);
		ale = ath_hal_alq_get(ah);
		if (ale) {
			struct athregrec *r = (struct athregrec *) ale->ae_data;
			r->op = OP_WRITE;
			r->reg = reg;
			r->val = val;
			alq_post(ath_hal_alq, ale);
		}
		local_irq_restore(flags);
	}
	_OS_REG_WRITE(ah, reg, val);
}
EXPORT_SYMBOL(ath_hal_reg_write);

uint32_t __ahdecl
ath_hal_reg_read(struct ath_hal *ah, uint32_t reg)
{
	uint32_t val;

	val = _OS_REG_READ(ah, reg);
	if (ath_hal_alq) {
		unsigned long flags;
		struct ale *ale;

		local_irq_save(flags);
		ale = ath_hal_alq_get(ah);
		if (ale) {
			struct athregrec *r = (struct athregrec *) ale->ae_data;
			r->op = OP_READ;
			r->reg = reg;
			r->val = val;
			alq_post(ath_hal_alq, ale);
		}
		local_irq_restore(flags);
	}
	return val;
}
EXPORT_SYMBOL(ath_hal_reg_read);

void __ahdecl
OS_MARK(struct ath_hal *ah, u_int id, uint32_t v)
{
	if (ath_hal_alq) {
		unsigned long flags;
		struct ale *ale;

		local_irq_save(flags);
		ale = ath_hal_alq_get(ah);
		if (ale) {
			struct athregrec *r = (struct athregrec *) ale->ae_data;
			r->op = OP_MARK;
			r->reg = id;
			r->val = v;
			alq_post(ath_hal_alq, ale);
		}
		local_irq_restore(flags);
	}
}
EXPORT_SYMBOL(OS_MARK);
#elif defined(AH_DEBUG) || defined(AH_REGOPS_FUNC)
/*
 * Memory-mapped device register read/write.  These are here
 * as routines when debugging support is enabled and/or when
 * explicitly configured to use function calls.  The latter is
 * for architectures that might need to do something before
 * referencing memory (e.g. remap an i/o window).
 *
 * NB: see the comments in ah_osdep.h about byte-swapping register
 *     reads and writes to understand what's going on below.
 */
void __ahdecl
ath_hal_reg_write(struct ath_hal *ah, u_int reg, uint32_t val)
{
#ifdef AH_DEBUG
	if (ath_hal_debug > 1)
		ath_hal_printf(ah, "WRITE 0x%x <= 0x%x\n", reg, val);
#endif
	_OS_REG_WRITE(ah, reg, val);
}
EXPORT_SYMBOL(ath_hal_reg_write);

uint32_t __ahdecl
ath_hal_reg_read(struct ath_hal *ah, u_int reg)
{
 	uint32_t val;

	val = _OS_REG_READ(ah, reg);
#ifdef AH_DEBUG
	if (ath_hal_debug > 1)
		ath_hal_printf(ah, "READ 0x%x => 0x%x\n", reg, val);
#endif
	return val;
}
EXPORT_SYMBOL(ath_hal_reg_read);
#endif /* AH_DEBUG || AH_REGOPS_FUNC */

#ifdef AH_DEBUG
void __ahdecl
HALDEBUG(struct ath_hal *ah, const char* fmt, ...)
{
	if (ath_hal_debug) {
		__va_list ap;
		va_start(ap, fmt);
		ath_hal_vprintf(ah, fmt, ap);
		va_end(ap);
	}
}


void __ahdecl
HALDEBUGn(struct ath_hal *ah, u_int level, const char* fmt, ...)
{
	if (ath_hal_debug >= level) {
		__va_list ap;
		va_start(ap, fmt);
		ath_hal_vprintf(ah, fmt, ap);
		va_end(ap);
	}
}
#endif /* AH_DEBUG */

/*
 * Delay n microseconds.
 */
void __ahdecl
ath_hal_delay(int n)
{
	BusyMicroDelay(n);
}

uint32_t __ahdecl
ath_hal_getuptime(struct ath_hal *ah)
{
//	return ((jiffies / HZ) * 1000) + (jiffies % HZ) * (1000 / HZ);
return 0;
}
//EXPORT_SYMBOL(ath_hal_getuptime);

/*
 * Allocate/free memory.
 */

void * __ahdecl
ath_hal_malloc(size_t size)
{
	return AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);
}

void __ahdecl
ath_hal_free(void* p)
{
	FreeVec(p);
}

void __ahdecl
ath_hal_memzero(void *dst, size_t n)
{
	memset(dst, 0, n);
}
//EXPORT_SYMBOL(ath_hal_memzero);

void * __ahdecl
ath_hal_memcpy(void *dst, const void *src, size_t n)
{
	return memcpy(dst, src, n);
}
//EXPORT_SYMBOL(ath_hal_memcpy);

int __ahdecl
ath_hal_memcmp(const void *a, const void *b, size_t n)
{
	return memcmp(a, b, n);
}
//EXPORT_SYMBOL(ath_hal_memcmp);

