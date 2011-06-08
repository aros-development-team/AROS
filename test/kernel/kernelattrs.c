#include <aros/kernel.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <stdio.h>

int main(void)
{
#ifdef KrnGetSystemAttr
    APTR KernelBase = OpenResource("kernel.resource");
    
    if (!KernelBase)
    {
	printf("Failed to open kernel.resource!\n");
	return 1;
    }

    printf("Architecture   : %s\n", (char *)KrnGetSystemAttr(KATTR_Architecture));
    printf("VBlank enabled : %ld\n", KrnGetSystemAttr(KATTR_VBlankEnable));
    printf("Timer IRQ      : %ld\n", KrnGetSystemAttr(KATTR_TimerIRQ));
    printf("Minimum stack  : %ld\n", KrnGetSystemAttr(KATTR_MinStack));
#else
    printf("The test can't be built for this kernel.resource implementation\n");
#endif

    return 0;
}
