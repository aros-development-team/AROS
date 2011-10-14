/* Windows-hosted kernel provides VBlank interrupt on itself, we can use it */
#define USE_VBLANK_INT

struct PlatformTimer
{
    APTR	   hostlibBase;
    APTR	   kernelHandle;
    ULONG	   (*StartClock)(UBYTE irq, ULONG freq);
    struct timeval tb_VBlankTime;	/* Our periodic timer interval   */
};

#define HostLibBase TimerBase->tb_Platform.hostlibBase
