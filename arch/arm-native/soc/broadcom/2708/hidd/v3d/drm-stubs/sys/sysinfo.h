/*
 * AROS stub for sys/sysinfo.h. Mesa's v3d_screen.c uses exactly one field,
 * totalram, to answer PIPE_CAP video memory queries. Reporting zero there
 * is what the driver falls back to whenever the call does not succeed, so
 * nothing needs the real figure.
 */
#ifndef SYS_SYSINFO_H_AROS
#define SYS_SYSINFO_H_AROS

struct sysinfo
{
    unsigned long totalram;
    unsigned long freeram;
    unsigned int  mem_unit;
};

static inline int sysinfo(struct sysinfo *info)
{
    info->totalram = 0;
    info->freeram = 0;
    info->mem_unit = 1;
    return 0;
}

#endif
