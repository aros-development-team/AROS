/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <ntstatus.h>
#include <windows.h>
#include <winternl.h>
#include <powrprof.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __x86_64__
#define __aros __attribute__((sysv_abi))
#else
#define __aros
#endif

// Processor Power Informatio
typedef struct _PROCESSOR_POWER_INFORMATION {
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

volatile int __declspec(dllexport) __aros win_getcpucount(void)
{
	SYSTEM_INFO si;

    printf("[processor.win:native] %s()\n", __func__);

	GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

volatile DWORD64 __declspec(dllexport) __aros win_getcpufreq(int cpu, PPROCESSOR_POWER_INFORMATION pppi)
{
	SYSTEM_INFO si;
    DWORD64 freq = 0;

    printf("[processor.win:native] %s(%u)\n", __func__, cpu);

	GetSystemInfo(&si);
    if (cpu < si.dwNumberOfProcessors)
    {
        if (pppi)
        {
            NTSTATUS status = CallNtPowerInformation(ProcessorInformation ,
                    NULL, 0,
                    pppi, sizeof(PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors);
            if (status == STATUS_SUCCESS)
            {
                pppi = &pppi[cpu];
                freq = (DWORD64)pppi->CurrentMhz * 1000000;
            }
        }
    }
    return freq;
}
