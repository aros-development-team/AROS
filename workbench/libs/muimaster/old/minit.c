/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <config.h>

#include <gdk/gdk.h>
#ifdef WITH_IMLIB
#include <gdk_imlib.h>
#endif
#include <zunepriv.h>
#include <prefs.h>
#include <drawing.h>
#include <event_impl.h>

#include <signal.h>

int __zune_signals;
int __zune_vararg_hacks; /* 1 if hacks allowed */

extern void __zune_clipping_init (void);
extern void __zune_imspec_init (void);

static void
zune_signals_intr_handler (int sig)
{
    __zune_signals |= SIGBREAKF_CTRL_C;
}

/*
 * Currently handle some termination signals to handle nice exit
 * of the application loop. Handling other signals may be useful ?
 */

static void
zune_signals_setup ()
{
    struct sigaction sa;
    sigset_t mask;

    __zune_signals = 0;
    sa.sa_handler = zune_signals_intr_handler;
    sa.sa_flags = 0;
    sigemptyset(&mask);
    sa.sa_mask = mask;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

static void
zune_check_iptr ()
{
    if ((sizeof(IPTR) < sizeof(void *))
     || (sizeof(IPTR) < sizeof(int)))
    {
	g_error("zune_check_iptr : The IPTR type doesn't seem "
		"big enough for ints and pointers.\n"
		"Edit exec/types.h and recompile.\n");
    }
}

static int
varfunA_a (int *intv)
{
   if (   intv[0] != 1
       || intv[1] != 2
       || intv[2] != 3
       || intv[3] != 4
       || intv[4] != 0)
       return 0;
   return 1;
}

static int
varfun_a (int arg1, ...)
{
    return varfunA_a (&arg1);
}

static int
varfunA_b (int fixed, int *intv)
{
   if (   fixed != 42
       || intv[0] != 1
       || intv[1] != 2
       || intv[2] != 3
       || intv[3] != 4
       || intv[4] != 0)
       return 0;
   return 1;
}

static int
varfun_b (int fixed, ...)
{
    va_list args;
    int r;

    va_start (args, fixed);
    r = varfunA_b (fixed, (int *)args);
    va_end (args);
    return r;
}

static void
zune_check_varargs ()
{
    __zune_vararg_hacks = 1;
    if (varfun_a (1, 2, 3, 4, 0) == 0)
	__zune_vararg_hacks = 0;
    if (varfun_b (42, 1, 2, 3, 4, 0) == 0)
	__zune_vararg_hacks = 0;
}

/*
 * Parse argc/argv for lowlevel stuff (display ...)
 */
void
MUI_Init (int *argc, char ***argv)
{
    gdk_init(argc, argv);
#ifdef WITH_IMLIB
    gdk_imlib_init();
#endif
    zune_check_iptr();
    zune_check_varargs();

    gdk_event_handler_set ((GdkEventFunc)__zune_main_do_event, NULL, NULL);

    __zune_clipping_init();
    __zune_imspec_init();
    __zune_images_init();
    /*
     * init prefs before loading from files - in case there's no file
     */
    __zune_prefs_init(&__zprefs);

    zune_signals_setup();
}

