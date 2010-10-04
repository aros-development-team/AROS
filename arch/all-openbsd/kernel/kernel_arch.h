/*
 * We can't include <signal.h> and <sys/types.h> here because
 * in this case we will include AROS include files and not host OS ones.
 * In order to overcome this we rely on internal OpenBSD definitions.
 * They should not change, otherwise binary compatibility between OpenBSD
 * versions will be broken.
 */
#include <sys/sigtypes.h>

/* Our timer is SIGALRM (N14) */
#define IRQ_TIMER 14

#define IRQ_COUNT 32

/* We have no interrupt controller */
#define ictl_enable_irq(irq)
#define ictl_disable_irq(irq)

#define HAVE_PLATFORM_DATA

struct PlatformData
{
    unsigned int sig_int_mask;	/* Mask of signals that Disable() block */
    unsigned int supervisor;
    int		 pid;		/* pid_t may conflict with AROS declaration */
};
