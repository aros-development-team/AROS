/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include <proto/exec.h>
#include <stdlib.h>
#include <string.h>
#include "support.h"

#define MIN(a,b) ((a)<=(b)?(a):(b))
#define BASE_PTR(ptr) ((APTR)((size_t *)ptr - 1))
#define ALLOC_SIZE(ptr) (*((size_t *)ptr - 1))

static struct SignalSemaphore *LibPoolSemaphore;
static APTR LibPool;

int malloc_init(void) {
	LibPoolSemaphore = CreateSemaphore();
	if (LibPoolSemaphore) {
		LibPool = CreatePool(MEMF_ANY, 4096, 1024);
		if (LibPool) {
			return TRUE;
		}
		DeleteSemaphore(LibPoolSemaphore);
	}
	return FALSE;
}

void malloc_exit(void) {
	DeletePool(LibPool);
	DeleteSemaphore(LibPoolSemaphore);
}

APTR malloc(size_t size) {
	size_t *ptr;
	ObtainSemaphore(LibPoolSemaphore);
	ptr = AllocPooled(LibPool, sizeof(size_t) + size);
	ReleaseSemaphore(LibPoolSemaphore);
	if (ptr) {
		*ptr++ = size;
	}
	return ptr;
}

APTR realloc(APTR old, size_t size) {
	if (old) {
		size_t old_size = ALLOC_SIZE(old);
		APTR new;
		if (size == old_size) {
			return old;
		}
		new = malloc(size);
		if (new) {
			memcpy(new, old, MIN(size, old_size));
			free(old);
			return new;
		}
		free(old);
		return NULL;
	} else {
		return malloc(size);
	}
}

void free(APTR ptr) {
	if (ptr) {
		ObtainSemaphore(LibPoolSemaphore);
		FreePooled(LibPool, BASE_PTR(ptr), sizeof(size_t) + ALLOC_SIZE(ptr));
		ReleaseSemaphore(LibPoolSemaphore);
	}
}
