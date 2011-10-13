struct itimerval;

struct PlatformTimer
{
    APTR	   hostlibBase;
    APTR	   libcHandle;
    int  	   (*setitimer)(int which, const struct itimerval *value, struct itimerval *ovalue);
    struct timeval tb_VBlankTime;	/* Software-emulated periodic timer interval	*/
    unsigned int   tb_VBlankTicks;	/* Divisor reload value for VBlank		*/
    unsigned int   tb_TimerCount;	/* VBlank tick counter				*/
};

#define HostLibBase TimerBase->tb_Platform.hostlibBase
