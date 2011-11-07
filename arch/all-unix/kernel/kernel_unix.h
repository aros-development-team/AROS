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
