/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <boost/preprocessor/repetition/repeat.hpp>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#define TIMER(name) \
    struct timeval name ## _start; \
    struct timeval name ## _stop

#define START(name) gettimeofday(& name ## _start, NULL);

#define STOP(name) gettimeofday(& name ## _stop, NULL);

#define ELAPSED(name) ((double)(((name ## _stop.tv_sec * 1000000) + name ## _stop.tv_usec) - ((name ## _start.tv_sec * 1000000) + name ## _start.tv_usec))/1000000.0)

#define BENCHMARK_UNIVERSAL(name, count, bufsize) \
    TIMER(name); \
    START(name); \
    long name ## _i; \
    for(name ## _i = 0; name ## _i < count/100; name ## _i++) { \
        BOOST_PP_REPEAT(100, BENCHMARK,name ## _i); \
    } \
    STOP(name);

#define BENCHMARK_OPERATION(name, count) \
    BENCHMARK_UNIVERSAL(name, count, 1); \
    printf(#name " %.2lf operations/s\n", 1.0 * count / ELAPSED(name));
    

#define BENCHMARK_BUFFER(name, count, bufsize) \
    BENCHMARK_UNIVERSAL(name, count, bufsize) \
    printf(#name " %.2lf bytes/s\n", 1.0 * bufsize * count / ELAPSED(name));
