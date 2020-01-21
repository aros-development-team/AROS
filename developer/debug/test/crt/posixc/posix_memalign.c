/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <exec/types.h>

#define BLOCK_SIZE (64 * 4096)
#define BLOCK_MASK ~(BLOCK_SIZE - 1)
#define BLOCK_COUNT 16

int main() 
{
    void *blocks[BLOCK_COUNT];
    int i;
    long block_size, block_mask;

    /* Allocate a constant alignment with sizes that are multiples of it */
    for (i = 0; i < BLOCK_COUNT; i++)
    {
        TEST(posix_memalign(&blocks[i], BLOCK_SIZE, BLOCK_SIZE * i) == 0);
        TEST((IPTR)blocks[i] == ((IPTR)blocks[i] & (IPTR)BLOCK_MASK));
    }

    for (i = 0; i < BLOCK_COUNT; i++)
    {
        free(blocks[i]);
    }

    /* Allocate varying alignment with a constant size */
    for (i = 0; i < BLOCK_COUNT; i++)
    {
        block_size = sizeof(void *) << i;
        block_mask = ~(block_size - 1);
        TEST(posix_memalign(&blocks[i], block_size, BLOCK_SIZE) == 0);
        TEST((IPTR)blocks[i] == ((IPTR)blocks[i] & (IPTR)block_mask));
    }

    /* Reallocate the memory with a different size */
    for (i = 0; i < BLOCK_COUNT; i++)
    {
        TEST((blocks[i] = realloc(blocks[i], BLOCK_SIZE * i)) != NULL);
    }

    for (i = 0; i < BLOCK_COUNT; i++)
    {
        free(blocks[i]);
    }

    /* Allocate a constant alignment with sizes that are smaller than it */
    for (i = 0; i < BLOCK_COUNT; i++)
    {
        TEST(posix_memalign(&blocks[i], BLOCK_SIZE, BLOCK_SIZE - i * 1000)
            == 0);
        TEST((IPTR)blocks[i] == ((IPTR)blocks[i] & (IPTR)BLOCK_MASK));
    }

    /* Reallocate the memory with a different size */
    for (i = 0; i < BLOCK_COUNT; i++)
    {
        TEST((blocks[i] = realloc(blocks[i], BLOCK_SIZE)) != NULL);
    }

    for (i = 0; i < BLOCK_COUNT; i++)
    {
        free(blocks[i]);
    }

    return OK;
}

void cleanup()
{
}
