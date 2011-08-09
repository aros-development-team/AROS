#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#define ALERT_WIDTH 80

static inline void PrintChars(char c, ULONG n, struct KernelBase *KernelBase)
{
    while (n--)
        krnPutC(c, KernelBase);
}

/*
 * This function calculates length of line.
 * It's similar to strlen(), but stops also at LF and FF codes.
 */
static inline int linelen(const char *str)
{
    int l;

    for (l = 0; str[l] && str[l] != '\n' && str[l] != 0x0F; l++);
    return l;
}

static const char *PrintCentered(const char *str, struct KernelBase *KernelBase)
{
    int len = linelen(str);
    int i;
    ULONG s;

    if (len > (ALERT_WIDTH - 2))
    	    len = (ALERT_WIDTH - 2);

    s = ALERT_WIDTH - 2 - len;

    krnPutC('#', KernelBase);
    if (s & 1)
        krnPutC(' ', KernelBase);
    s >>= 1;
    PrintChars(' ', s, KernelBase);

    for (i = 0; i < len; i++)
        krnPutC(*str++, KernelBase);

    PrintChars(' ', s, KernelBase);
    krnPutC('#', KernelBase);
    krnPutC('\n', KernelBase);

    return str;
}

static inline void PrintFrame(struct KernelBase *KernelBase)
{
    PrintChars('#', ALERT_WIDTH, KernelBase);
    krnPutC('\n', KernelBase);
}

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(void, KrnDisplayAlert,

/*  SYNOPSIS */
	AROS_LHA(uint32_t, code, D0),
	AROS_LHA(const char *, text, A0),

/*  LOCATION */
	struct KernelBase *, KernelBase, 35, Kernel)

/*  FUNCTION
	Inform the user about critical system failure.

    INPUTS
    	code - Corresponding alert code.
	text - A NULL-terminated text to print out.

	First three lines are assumed to be a header. Some implementations
	may print them centered inside a frame.

    RESULT
	None. This function is not guaranteed to return.

    NOTES
	This function exists for system internal purposes. Please do not
	call it from within regular applications! In 99% of cases this function
	will halt or reboot the machine. Certain structures in RAM, as well as
	video hardware state, will be irreversibly destroyed.

	'code' parameter is passed for convenience. Based on it, the system
	can make a decision to log the alert in debug output and continue,
	instead of displaying a message and halting.

	This function is currently experimental. Its definition may change.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * This is a generic version that simply formats the text into debug log.
     * It can be replaced with machine-specific implementation which can do more.
     */
    unsigned int i;

    /* Make sure that the output starts from a new line */
    krnPutC('\n', KernelBase);

    PrintFrame(KernelBase);

    /* Print first three lines (title, task and error) centered */
    for (i = 0; i < 3; i++)
    {
    	text = PrintCentered(text, KernelBase);
    	text++;	/* Skip a newline */
    }

    PrintFrame(KernelBase);

    /* Print the rest of alert text */
    while (*text)
    	krnPutC(*text++, KernelBase);

    krnPutC('\n', KernelBase);
    PrintFrame(KernelBase);

    AROS_LIBFUNC_EXIT
}
