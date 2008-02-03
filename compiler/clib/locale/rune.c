/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)rune.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/lib/libc/locale/rune.c,v 1.15 2007/01/09 00:28:00 imp Exp $");

#include "namespace.h"
#ifdef __AROS__
/* until we have arpa/inet.h in libc */
#include <exec/types.h>
#include <aros/macros.h>
#define ntohl(x) AROS_BE2LONG(x)
#else
#include <arpa/inet.h>
#endif
#include <errno.h>
#include <runetype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "un-namespace.h"

#include "runefile.h"

_RuneLocale *_Read_RuneMagi(FILE *);

_RuneLocale *
_Read_RuneMagi(FILE *fp)
{
	char *fdata, *data;
	void *lastp;
	_FileRuneLocale *frl;
	_RuneLocale *rl;
	_FileRuneEntry *frr;
	_RuneEntry *rr;
	struct stat sb;
	int x, saverr;
	void *variable;
	_FileRuneEntry *runetype_ext_ranges;
	_FileRuneEntry *maplower_ext_ranges;
	_FileRuneEntry *mapupper_ext_ranges;
	int runetype_ext_len = 0;

	if (_fstat(fileno(fp), &sb) < 0)
		return (NULL);

	if ((size_t)sb.st_size < sizeof(_FileRuneLocale)) {
		errno = EFTYPE;
		return (NULL);
	}

	if ((fdata = malloc(sb.st_size)) == NULL)
		return (NULL);

	errno = 0;
	rewind(fp); /* Someone might have read the magic number once already */
	if (errno) {
		saverr = errno;
		free(fdata);
		errno = saverr;
		return (NULL);
	}

	if (fread(fdata, sb.st_size, 1, fp) != 1) {
		saverr = errno;
		free(fdata);
		errno = saverr;
		return (NULL);
	}

	frl = (_FileRuneLocale *)fdata;
	lastp = fdata + sb.st_size;

	variable = frl + 1;

	if (memcmp(frl->magic, _FILE_RUNE_MAGIC_1, sizeof(frl->magic))) {
		free(fdata);
		errno = EFTYPE;
		return (NULL);
	}

	frl->variable_len = ntohl(frl->variable_len);
	frl->runetype_ext_nranges = ntohl(frl->runetype_ext_nranges);
	frl->maplower_ext_nranges = ntohl(frl->maplower_ext_nranges);
	frl->mapupper_ext_nranges = ntohl(frl->mapupper_ext_nranges);

	for (x = 0; x < _CACHED_RUNES; ++x) {
		frl->runetype[x] = ntohl(frl->runetype[x]);
		frl->maplower[x] = ntohl(frl->maplower[x]);
		frl->mapupper[x] = ntohl(frl->mapupper[x]);
	}

	runetype_ext_ranges = (_FileRuneEntry *)variable;
	variable = runetype_ext_ranges + frl->runetype_ext_nranges;
	if (variable > lastp) {
		free(fdata);
		errno = EFTYPE;
		return (NULL);
	}

	maplower_ext_ranges = (_FileRuneEntry *)variable;
	variable = maplower_ext_ranges + frl->maplower_ext_nranges;
	if (variable > lastp) {
		free(fdata);
		errno = EFTYPE;
		return (NULL);
	}

	mapupper_ext_ranges = (_FileRuneEntry *)variable;
	variable = mapupper_ext_ranges + frl->mapupper_ext_nranges;
	if (variable > lastp) {
		free(fdata);
		errno = EFTYPE;
		return (NULL);
	}

	frr = runetype_ext_ranges;
	for (x = 0; x < frl->runetype_ext_nranges; ++x) {
		uint32_t *types;

		frr[x].min = ntohl(frr[x].min);
		frr[x].max = ntohl(frr[x].max);
		frr[x].map = ntohl(frr[x].map);
		if (frr[x].map == 0) {
			int len = frr[x].max - frr[x].min + 1;
			types = variable;
			variable = types + len;
			runetype_ext_len += len;
			if (variable > lastp) {
				free(fdata);
				errno = EFTYPE;
				return (NULL);
			}
			while (len-- > 0)
				types[len] = ntohl(types[len]);
		}
	}

	frr = maplower_ext_ranges;
	for (x = 0; x < frl->maplower_ext_nranges; ++x) {
		frr[x].min = ntohl(frr[x].min);
		frr[x].max = ntohl(frr[x].max);
		frr[x].map = ntohl(frr[x].map);
	}

	frr = mapupper_ext_ranges;
	for (x = 0; x < frl->mapupper_ext_nranges; ++x) {
		frr[x].min = ntohl(frr[x].min);
		frr[x].max = ntohl(frr[x].max);
		frr[x].map = ntohl(frr[x].map);
	}
	if ((char *)variable + frl->variable_len > (char *)lastp) {
		free(fdata);
		errno = EFTYPE;
		return (NULL);
	}

	/*
	 * Convert from disk format to host format.
	 */
	data = malloc(sizeof(_RuneLocale) +
	    (frl->runetype_ext_nranges + frl->maplower_ext_nranges +
	    frl->mapupper_ext_nranges) * sizeof(_RuneEntry) +
	    runetype_ext_len * sizeof(*rr->__types) +
	    frl->variable_len);
	if (data == NULL) {
		saverr = errno;
		free(fdata);
		errno = saverr;
		return (NULL);
	}

	rl = (_RuneLocale *)data;
	rl->__variable = rl + 1;

	memcpy(rl->__magic, _RUNE_MAGIC_1, sizeof(rl->__magic));
	memcpy(rl->__encoding, frl->encoding, sizeof(rl->__encoding));
	rl->__invalid_rune = 0;

	rl->__variable_len = frl->variable_len;
	rl->__runetype_ext.__nranges = frl->runetype_ext_nranges;
	rl->__maplower_ext.__nranges = frl->maplower_ext_nranges;
	rl->__mapupper_ext.__nranges = frl->mapupper_ext_nranges;

	for (x = 0; x < _CACHED_RUNES; ++x) {
		rl->__runetype[x] = frl->runetype[x];
		rl->__maplower[x] = frl->maplower[x];
		rl->__mapupper[x] = frl->mapupper[x];
	}

	rl->__runetype_ext.__ranges = (_RuneEntry *)rl->__variable;
	rl->__variable = rl->__runetype_ext.__ranges +
	    rl->__runetype_ext.__nranges;

	rl->__maplower_ext.__ranges = (_RuneEntry *)rl->__variable;
	rl->__variable = rl->__maplower_ext.__ranges +
	    rl->__maplower_ext.__nranges;

	rl->__mapupper_ext.__ranges = (_RuneEntry *)rl->__variable;
	rl->__variable = rl->__mapupper_ext.__ranges +
	    rl->__mapupper_ext.__nranges;

	variable = mapupper_ext_ranges + frl->mapupper_ext_nranges;
	frr = runetype_ext_ranges;
	rr = rl->__runetype_ext.__ranges;
	for (x = 0; x < rl->__runetype_ext.__nranges; ++x) {
		uint32_t *types;

		rr[x].__min = frr[x].min;
		rr[x].__max = frr[x].max;
		rr[x].__map = frr[x].map;
		if (rr[x].__map == 0) {
			int len = rr[x].__max - rr[x].__min + 1;
			types = variable;
			variable = types + len;
			rr[x].__types = rl->__variable;
			rl->__variable = rr[x].__types + len;
			while (len-- > 0)
				rr[x].__types[len] = types[len];
		} else
			rr[x].__types = NULL;
	}

	frr = maplower_ext_ranges;
	rr = rl->__maplower_ext.__ranges;
	for (x = 0; x < rl->__maplower_ext.__nranges; ++x) {
		rr[x].__min = frr[x].min;
		rr[x].__max = frr[x].max;
		rr[x].__map = frr[x].map;
	}

	frr = mapupper_ext_ranges;
	rr = rl->__mapupper_ext.__ranges;
	for (x = 0; x < rl->__mapupper_ext.__nranges; ++x) {
		rr[x].__min = frr[x].min;
		rr[x].__max = frr[x].max;
		rr[x].__map = frr[x].map;
	}

	memcpy(rl->__variable, variable, rl->__variable_len);
	free(fdata);

	/*
	 * Go out and zero pointers that should be zero.
	 */
	if (!rl->__variable_len)
		rl->__variable = NULL;

	if (!rl->__runetype_ext.__nranges)
		rl->__runetype_ext.__ranges = NULL;

	if (!rl->__maplower_ext.__nranges)
		rl->__maplower_ext.__ranges = NULL;

	if (!rl->__mapupper_ext.__nranges)
		rl->__mapupper_ext.__ranges = NULL;

	return (rl);
}
