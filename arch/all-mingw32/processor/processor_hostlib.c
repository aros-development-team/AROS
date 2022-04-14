/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#define __HOSTLIB_NOLIBBASE__
#include <proto/hostlib.h>

#include <resources/processor.h>

#include <string.h>

#include "processor_intern.h"

static const char *wincpu_func_names[] = {
    "win_getcpucount",
    "win_getcpufreq"
};
#define LCPUFUNCCOUNT	2
struct wincpu_func {
    int (*win_getcpucount)(void);
    UQUAD (*win_getcpufreq) ( int, void * );
};

static void *HostLibBase;
static void *wincpu_handle = NULL;
static void *wincpu_pppi = NULL;
struct wincpu_func wincpu_func;

#define CCALL(func,...) (wincpu_func.func(__VA_ARGS__))

UQUAD GetHostProcessorFrequency(int cpuno)
{
	if (wincpu_handle)
		return CCALL(win_getcpufreq, cpuno, wincpu_pppi);
	return 0;
}

void *wincpu_hostlib_load_so(const char *sofile, const char **names, int nfuncs, void **funcptr)
{
    void *handle;
    char *err;
    int i;

    D(
		bug("[processor.win] %s('%s')\n", __func__, sofile);
		bug("[processor.win] %s: attempting to load %d functions\n", __func__, nfuncs);
	)

    if ((handle = HostLib_Open(sofile, &err)) == NULL) {
        D(bug("[processor.win] %s: failed to open '%s': %s\n", __func__, sofile, err);)
        return NULL;
    }

    for (i = 0; i < nfuncs; i++) {
        funcptr[i] = HostLib_GetPointer(handle, names[i], &err);
        D(bug("[processor.win] %s: 0x%p = '%s'\n", __func__, funcptr[i], names[i]);)
        if (err != NULL) {
            D(bug("[processor.win] %s: failed to get symbol '%s' (%s)\n", __func__, names[i], err);)
            HostLib_Close(handle, NULL);
            return NULL;
        }
    }

    return handle;
}

static int wincpu_hostlib_init(void *unused)
{
    D(bug("[processor.win] %s()\n", __func__);)

    if ((HostLibBase = OpenResource("hostlib.resource")) != NULL) {
		wincpu_handle = wincpu_hostlib_load_so("Libs\\Host\\wincpu.dll", wincpu_func_names, LCPUFUNCCOUNT, (void **)&wincpu_func);
	} else {
        D(bug("[processor.win] %s: failed to open hostlib.resource!\n", __func__););
    }
    if (wincpu_handle)
    {
        int cpucount = CCALL(win_getcpucount);
        wincpu_pppi = AllocMem(cpucount * (6 * sizeof(ULONG)), MEMF_ANY);
    }
	return TRUE;
}

static int wincpu_hostlib_expunge(void *unused)
{
    D(bug("[processor.win] %s()\n", __func__);)

    if (wincpu_handle != NULL)
        HostLib_Close(wincpu_handle, NULL);

    return TRUE;
}

ADD2INITLIB(wincpu_hostlib_init, 0)
ADD2EXPUNGELIB(wincpu_hostlib_expunge, 0)
