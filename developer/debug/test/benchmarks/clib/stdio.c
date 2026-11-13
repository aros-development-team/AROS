/*
    Copyright (C) 2008, The AROS Development Team. All rights reserved.
*/

#include "benchmark.h"
#include <stdio.h>

#define BUFSIZE 1000000

int main() {
    FILE *file;
    char *buffer;
    buffer = malloc(BUFSIZE);

    printf("Please be patient while benchmarks run...\n");

    if(buffer && (file = fopen("T:__test__", "w")))
    {
        printf("Benchmarking 1MB Reads/Writes ...\n");

        #define BENCHMARK(z, n, c) fwrite(buffer, BUFSIZE, sizeof(char), file);
        BENCHMARK_BUFFER(fwrite,100, BUFSIZE);
        #undef BENCHMARK
        
        fseek(file, 0, SEEK_SET);
        
        #define BENCHMARK(z, n, c) fread(buffer, BUFSIZE, sizeof(char), file);
        BENCHMARK_BUFFER(fread,100, BUFSIZE);
        #undef BENCHMARK
        
        fclose(file);
        remove("T:__test__");
    }

    if(buffer && (file = fopen("T:__test__", "w")))
    {
        printf("Benchmarking 1Byte Reads/Writes ...\n");

        #define BENCHMARK(z, n, c) fwrite(buffer, 1, sizeof(char), file);
        BENCHMARK_OPERATION(fwrite,10000000);
        #undef BENCHMARK

        fseek(file, 0, SEEK_SET);

        #define BENCHMARK(z, n, c) fread(buffer, 1, sizeof(char), file);
        BENCHMARK_OPERATION(fread,10000000);
        #undef BENCHMARK

        fseek(file, 0, SEEK_SET);

        #define BENCHMARK(z, n, c) fseek(file, 1, SEEK_CUR);
        BENCHMARK_OPERATION(fseek_cur,10000);
        #undef BENCHMARK

        #define BENCHMARK(z, n, c) fseek(file, c * 100 + n, SEEK_SET);
        BENCHMARK_OPERATION(fseek_set,10000);
        #undef BENCHMARK

        #define BENCHMARK(z, n, c) fseek(file, c * 100 + n, SEEK_END);
        BENCHMARK_OPERATION(fseek_end,10000);
        #undef BENCHMARK
        
        fclose(file);
        remove("T:__test__");
    }

    if (buffer)
        free(buffer);

    printf("Benchmarks complete...\n");
    
    return 0;
}
