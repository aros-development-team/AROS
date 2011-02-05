/*
 * support.c
 *
 *  Created on: Jun 29, 2010
 *      Author: misc
 */

#include "bootstrap.h"

#include <sys/types.h>

static unsigned char __tmpspace[BOOT_TMP_SIZE];
static unsigned char *first_free = &__tmpspace[0];
static unsigned long free_memory = BOOT_TMP_SIZE;

void *malloc(size_t size)
{
	void *ret = NULL;

	size = (size + 15) & ~15;

	if (size <= free_memory)
	{
		ret = first_free;

		first_free += size;
		free_memory -= size;
	}

	return ret;
}

size_t mem_avail()
{
	return free_memory;
}

size_t mem_used()
{
	return BOOT_TMP_SIZE - free_memory;
}
