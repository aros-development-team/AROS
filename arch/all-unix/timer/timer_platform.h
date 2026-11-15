struct itimerval;
struct timespec;

struct PlatformTimer
{
    APTR	   hostlibBase;
    APTR	   libcHandle;
    int  	   (*setitimer)(int which, const struct itimerval *value, struct itimerval *ovalue);
    int  	   (*clock_gettime)(int clk, struct timespec *tp);	/* the host's, or NULL	*/
    UQUAD	   tb_HostLast;		/* host monotonic time at the last tick, in us	*/
    UQUAD	   tb_HostElapsed;	/* host time counted so far (us), for the EClock	*/
    struct timeval tb_VBlankTime;	/* Software-emulated periodic timer interval	*/
    unsigned int   tb_VBlankTicks;	/* Divisor reload value for VBlank		*/
    unsigned int   tb_TimerCount;	/* VBlank tick counter				*/
};

#define HostLibBase TimerBase->tb_Platform.hostlibBase
