/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(PATH_MAX)
#define PATH_MAX 4096
#endif

struct linuxcpu_func {
    void *(*fopen) ( void *, void * );
    int (*fclose) ( void * );
    char * (*fgets) ( char * , int , void * );
    int (*strncmp) ( const char *, const char *,  size_t );
    int (*atoi)( const char * );
    double (*strtod) ( const char *, char ** );
    char * (*strchr) ( const char * , int );
};

extern struct linuxcpu_func linuxcpu_func;
#define CCALL(func,...) (linuxcpu_func.func(__VA_ARGS__))

int64_t linuxcpu_getproccpufreq(int cpu)
{
    int thiscpu = -1;
    int64_t freq = 0;

    void *f = CCALL(fopen, "/proc/cpuinfo", "r");
    if (f)
    {
        char pcpuiline[PATH_MAX];

        while (CCALL(fgets, pcpuiline, PATH_MAX, f) != NULL)
        {
            if (!CCALL(strncmp, "processor", pcpuiline, 9))
                thiscpu = CCALL(atoi, CCALL(strchr, pcpuiline, ':') + 2);
            if (!CCALL(strncmp, "cpu MHz", pcpuiline, 7)) {
                int64_t cpufreq = (int64_t)(CCALL(strtod, CCALL(strchr, pcpuiline, ':') + 2, NULL) * 1000000.0);
                if ((thiscpu == -1) || (thiscpu = cpu))
                    freq = cpufreq;
            }
        }
        CCALL(fclose, f);
    }
    return freq;
}
