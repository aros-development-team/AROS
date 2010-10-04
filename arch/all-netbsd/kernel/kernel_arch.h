/*
 * We can't include <signal.h> and <sys/types.h> here because
 * in this case we will include AROS include files and not host OS ones.
 * In order to overcome this we include NetBSD-specific stuff directly
 */
#include <sys/sigtypes.h>

/* Our timer is SIGALRM */
#define IRQ_TIMER SIGALRM

#define IRQ_COUNT 32

/* We have no interrupt controller */
#define ictl_enable_irq(irq)
#define ictl_disable_irq(irq)

#define HAVE_PLATFORM_DATA

struct PlatformData
{
    sigset_t	 sig_int_mask;	/* Mask of signals that Disable() block */
    unsigned int supervisor;
    int		 pid;		/* pid_t may conflict with AROS declaration */
};
