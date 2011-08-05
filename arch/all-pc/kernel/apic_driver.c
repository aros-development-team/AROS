#include <exec/types.h>

#include "apic_driver.h"
#include "kernel_base.h"
#include "kernel_debug.h"

#define D(x)

extern const struct GenericAPIC apic_ia32_default;

static const struct GenericAPIC *probe_APIC[] =
{
    &apic_ia32_default, /* must be last */
    NULL,
};

const struct GenericAPIC *core_APIC_Probe(void)
{
    int driver_count;

    for (driver_count = 0; probe_APIC[driver_count]; driver_count++)
    {
	IPTR retval = probe_APIC[driver_count]->probe();

    	if (retval)
        {
	    D(bug("[Kernel] core_APICProbe: Using APIC driver '%s'\n", probe_APIC[driver_count]->name));
	    return probe_APIC[driver_count];
        }
    }

    D(bug("[Kernel] core_APICProbe: No suitable APIC driver found.\n"));
    return NULL;
}
