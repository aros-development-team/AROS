/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/hostlib.h>

#include <resources/processor.h>

#include <string.h>

#include "processor_intern.h"

static const char *linuxcpu_func_names[] = {
    "fopen",
	"fclose",
	"fgets",
	"strncmp",
	"atoi",
	"strtod",
	"strchr"
};
#define LCPUFUNCCOUNT	7

void *HostLibBase;
void *linuxcpu_handle = NULL;
void * linuxcpu_func[LCPUFUNCCOUNT];

extern UQUAD linuxcpu_getproccpufreq(int);

UQUAD GetHostProcessorFrequency(int cpuno)
{
	if (linuxcpu_handle)
		return linuxcpu_getproccpufreq(cpuno + 1);
	return 0;
}

void *linuxcpu_hostlib_load_so(const char *sofile, const char **names, int nfuncs, void **funcptr)
{
    void *handle;
    char *err;
    int i;

    D(
		bug("[processor.linux] %s('%s')\n", __func__, sofile);
		bug("[processor.linux] %s: attempting to load %d functions\n", __func__, nfuncs);
	)

    if ((handle = HostLib_Open(sofile, &err)) == NULL) {
        D(bug("[processor.linux] %s: failed to open '%s': %s\n", __func__, sofile, err);)
        return NULL;
    }

    for (i = 0; i < nfuncs; i++) {
        funcptr[i] = HostLib_GetPointer(handle, names[i], &err);
        D(bug("[processor.linux] %s: 0x%p = '%s'\n", __func__, funcptr[i], names[i]);)
        if (err != NULL) {
            D(bug("[processor.linux] %s: failed to get symbol '%s' (%s)\n", __func__, names[i], err);)
            HostLib_Close(handle, NULL);
            return NULL;
        }
    }

    return handle;
}

static int linuxcpu_hostlib_init(void *unused)
{
    D(bug("[processor.linux] %s()\n", __func__);)

    if ((HostLibBase = OpenResource("hostlib.resource")) != NULL) {
		linuxcpu_handle = linuxcpu_hostlib_load_so("libc.so.6", linuxcpu_func_names, LCPUFUNCCOUNT, (void **)linuxcpu_func);
	} else {
        D(bug("[processor.linux] %s: failed to open hostlib.resource!\n", __func__););
    }
	return TRUE;
}

static int linuxcpu_hostlib_expunge(void *unused)
{
    D(bug("[processor.linux] %s()\n", __func__);)

    if (linuxcpu_handle != NULL)
        HostLib_Close(linuxcpu_handle, NULL);

    return TRUE;
}

ADD2INITLIB(linuxcpu_hostlib_init, 0)
ADD2EXPUNGELIB(linuxcpu_hostlib_expunge, 0)
