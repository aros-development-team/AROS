#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <exec/types.h>

#define BLOCK_SIZE (64 * 4096)
#define BLOCK_MASK ~(BLOCK_SIZE - 1)

int main() 
{
    void * address = NULL;

    posix_memalign(&address, BLOCK_SIZE, BLOCK_SIZE);

    TEST(((IPTR)address == ((IPTR)address & (IPTR)BLOCK_MASK)));

    free(address);

    return OK;
}

void cleanup()
{
}
