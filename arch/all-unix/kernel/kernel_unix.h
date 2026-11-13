/* Things declared here do not depend on host OS includes */

struct HostInterface;

extern unsigned int SupervisorCount;
extern struct HostInterface *HostIFace;

unsigned int krnGetPageSize(void *libc);
int core_Start(void *libc);

/* Our own add-ons to KernelBase */
struct UnixKernelBase
{
    struct KernelBase kb;
    unsigned int      SupervisorCount;
};

#define UKB(base) ((struct UnixKernelBase *)base)

#ifdef AROS_NO_ATOMIC_OPERATIONS

#define SUPERVISOR_ENTER UKB(KernelBase)->SupervisorCount++
#define SUPERVISOR_LEAVE UKB(KernelBase)->SupervisorCount--

#else

#define SUPERVISOR_ENTER AROS_ATOMIC_INC(UKB(KernelBase)->SupervisorCount)
#define SUPERVISOR_LEAVE AROS_ATOMIC_DEC(UKB(KernelBase)->SupervisorCount)

#endif
