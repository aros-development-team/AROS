/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CPU frequency governor helpers for PC platforms
    Lang: english
*/

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "cpu_freq.h"

#define D(x)

static UBYTE core_CPUFreqResolveLevels(const struct CPUFreqPolicy *policy)
{
    UBYTE levels = policy->levels;

    if (!levels)
        levels = CPUFREQ_LEVELS_DEFAULT;
    if (levels < 2)
        levels = 2;

    return levels;
}

static UBYTE core_CPUFreqSelectTarget(const struct CPUData *core, const struct CPUFreqPolicy *policy, ULONG load)
{
    UBYTE low_ratio = core->cpu_PerfMinRatio ? core->cpu_PerfMinRatio : 0x00;
    UBYTE high_ratio = core->cpu_PerfMaxRatio ? core->cpu_PerfMaxRatio : 0xff;
    UBYTE levels = core_CPUFreqResolveLevels(policy);

    if (levels == 2)
    {
        if (load >= policy->up_threshold)
            return high_ratio;
        if (load <= policy->down_threshold)
            return low_ratio;
        return core->cpu_PerfCurRatio;
    }

    if (load <= policy->down_threshold)
        return low_ratio;
    if (load >= policy->up_threshold)
        return high_ratio;

    UQUAD range = (UQUAD)policy->up_threshold - (UQUAD)policy->down_threshold;
    if (!range)
        return core->cpu_PerfCurRatio;

    UQUAD pos = (UQUAD)load - (UQUAD)policy->down_threshold;
    UQUAD level = ((pos * (levels - 1)) + (range / 2)) / range;
    if (level >= levels)
        level = levels - 1;

    int delta = (int)high_ratio - (int)low_ratio;
    int target = (int)low_ratio + ((delta * (int)level) / (int)(levels - 1));
    if (target < 0)
        target = 0;
    if (target > 0xff)
        target = 0xff;

    return (UBYTE)target;
}

void core_CPUFreqUpdate(struct PlatformData *pdata, apicid_t cpuNum)
{
    struct APICData *apicData;
    struct CPUData *core;
    ULONG load;
    UBYTE target;

    if (!pdata || !(pdata->kb_PDFlags & PLATFORMF_CPUFREQ) || !pdata->kb_CPUFreqSet)
        return;

    apicData = pdata->kb_APIC;
    if (!apicData || cpuNum >= apicData->apic_count)
        return;

    core = &apicData->cores[cpuNum];

    load = core->cpu_Load;

    if (pdata->kb_CPUFreqPolicy.up_threshold == 0)
        pdata->kb_CPUFreqPolicy.up_threshold = CPUFREQ_LOAD_HIGH;
    if (pdata->kb_CPUFreqPolicy.down_threshold == 0)
        pdata->kb_CPUFreqPolicy.down_threshold = CPUFREQ_LOAD_LOW;
    if (pdata->kb_CPUFreqPolicy.levels == 0)
        pdata->kb_CPUFreqPolicy.levels = CPUFREQ_LEVELS_DEFAULT;

    target = core_CPUFreqSelectTarget(core, &pdata->kb_CPUFreqPolicy, load);
    if (target == core->cpu_PerfCurRatio)
        return;

    if (pdata->kb_CPUFreqSet(pdata, cpuNum, target))
    {
        core->cpu_PerfCurRatio = target;
        D(bug("[Kernel] %s: PerfRatio %d\n", __func__, core->cpu_PerfCurRatio);)
    }
}
