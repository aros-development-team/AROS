#include "test.h"
#include <stdio.h>
#include <unistd.h>

int main() 
{
    TEST((usleep(1000000) != -1));
    return OK;
}

void cleanup()
{
}
