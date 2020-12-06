struct PlatformTimer
{
    LONG	        tb_TimerIRQNum;	/* Timer IRQ number			     */
    struct timeval      tb_VBlankTime;	/* Software-emulated periodic timer interval */
    UWORD               tb_InitValue;
    UWORD               tb_ReloadValue;
    UWORD               tb_ReloadMin;
};
