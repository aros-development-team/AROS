/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    long seed;
    int i, steps;

    if (argc != 3) {
        printf("Usage:\n%s <seed> <steps>\n", argv[0]);
        return EXIT_FAILURE;
    }

    seed = strtol(argv[1], NULL, 0);
    steps = strtol(argv[2], NULL, 0);

    srandom(seed);
    printf("Seed: %ld\n", seed);
    for (i = 0; i < steps; i++) {
        sleep(1);
        printf("Step %d: %ld\n", i, random());
    }

    return 0;
}



