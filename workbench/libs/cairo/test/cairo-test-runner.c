/*
 * Copyright © 2008 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Chris Wilson not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Chris Wilson makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * CHRIS WILSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL CHRIS WILSON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "cairo-test-private.h"
#include "cairo-boilerplate-getopt.h"

/* get the "real" version info instead of dummy cairo-version.h */
#undef CAIRO_VERSION_H
#undef CAIRO_VERSION_MAJOR
#undef CAIRO_VERSION_MINOR
#undef CAIRO_VERSION_MICRO
#include "../cairo-version.h"

#include <pixman.h> /* for version information */

/* Coregraphics doesn't seem to like being forked and reports:
 * "The process has forked and you cannot use this CoreFoundation functionality safely. You MUST exec()."
 * so we don't for on OS X */
#define SHOULD_FORK HAVE_FORK && HAVE_WAITPID && !__APPLE__
#if SHOULD_FORK
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#endif
#if HAVE_LIBGEN_H
#include <libgen.h>
#endif

#if HAVE_VALGRIND
#include <valgrind.h>
#else
#define RUNNING_ON_VALGRIND 0
#endif

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

typedef struct _cairo_test_list {
    const cairo_test_t *test;
    struct _cairo_test_list *next;
} cairo_test_list_t;

typedef struct _cairo_test_runner {
    cairo_test_context_t base;

    unsigned int num_device_offsets;

    cairo_bool_t passed;
    int num_passed;
    int num_skipped;
    int num_failed;
    int num_xfailed;
    int num_error;
    int num_crashed;

    cairo_test_list_t *crashes_preamble;
    cairo_test_list_t *errors_preamble;
    cairo_test_list_t *fails_preamble;

    cairo_test_list_t **crashes_per_target;
    cairo_test_list_t **errors_per_target;
    cairo_test_list_t **fails_per_target;

    int *num_failed_per_target;
    int *num_error_per_target;
    int *num_crashed_per_target;

    cairo_bool_t foreground;
    cairo_bool_t exit_on_failure;
    cairo_bool_t list_only;
    cairo_bool_t full_test;
    cairo_bool_t keyword_match;
    cairo_bool_t slow;
    cairo_bool_t force_pass;
} cairo_test_runner_t;

typedef enum {
    GE,
    GT
} cairo_test_compare_op_t;

static cairo_test_t *tests;

static void CAIRO_BOILERPLATE_PRINTF_FORMAT(2,3)
_log (cairo_test_context_t *ctx,
      const char *fmt,
      ...)
{
    va_list ap;

    va_start (ap, fmt);
    vprintf (fmt, ap);
    va_end (ap);

    va_start (ap, fmt);
    cairo_test_logv (ctx, fmt, ap);
    va_end (ap);
}

static cairo_test_list_t *
_list_prepend (cairo_test_list_t *head, const cairo_test_t *test)
{
    cairo_test_list_t *list;

    list = xmalloc (sizeof (cairo_test_list_t));
    list->test = test;
    list->next = head;
    head = list;

    return head;
}

static cairo_test_list_t *
_list_reverse (cairo_test_list_t *head)
{
    cairo_test_list_t *list, *next;

    for (list = head, head = NULL; list != NULL; list = next) {
	next = list->next;
	list->next = head;
	head = list;
    }

    return head;
}

static void
_list_free (cairo_test_list_t *list)
{
    while (list != NULL) {
	cairo_test_list_t *next = list->next;
	free (list);
	list = next;
    }
}

static cairo_bool_t
is_running_under_debugger (void)
{
    char buf[1024];

    if (RUNNING_ON_VALGRIND)
	return TRUE;

#if HAVE_UNISTD_H && HAVE_LIBGEN_H && __linux__
    sprintf (buf, "/proc/%d/exe", getppid ());
    if (readlink (buf, buf, sizeof (buf)) != -1 &&
	strncmp (basename (buf), "gdb", 3) == 0)
    {
	return TRUE;
    }
#endif

    return FALSE;
}

#if SHOULD_FORK
static cairo_test_status_t
_cairo_test_wait (pid_t pid)
{
    int exitcode;

    if (waitpid (pid, &exitcode, 0) != pid)
	return CAIRO_TEST_CRASHED;

    if (WIFSIGNALED (exitcode)) {
	switch (WTERMSIG (exitcode)) {
	case SIGINT:
#if HAVE_RAISE
	    raise (SIGINT);
#endif
	    return CAIRO_TEST_UNTESTED;
	default:
	    return CAIRO_TEST_CRASHED;
	}
    }

    return WEXITSTATUS (exitcode);
}
#endif

static cairo_test_status_t
_cairo_test_runner_preamble (cairo_test_runner_t *runner,
			     cairo_test_context_t *ctx)
{
#if SHOULD_FORK
    if (! runner->foreground) {
	pid_t pid;

	switch ((pid = fork ())) {
	case -1: /* error */
	    return CAIRO_TEST_UNTESTED;

	case 0: /* child */
	    exit (ctx->test->preamble (ctx));

	default:
	    return _cairo_test_wait (pid);
	}
    }
#endif
    return ctx->test->preamble (ctx);
}

static cairo_test_status_t
_cairo_test_runner_draw (cairo_test_runner_t *runner,
			 cairo_test_context_t *ctx,
			 const cairo_boilerplate_target_t *target,
			 cairo_bool_t similar,
			 int device_offset)
{
#if SHOULD_FORK
    if (! runner->foreground) {
	pid_t pid;

	switch ((pid = fork ())) {
	case -1: /* error */
	    return CAIRO_TEST_UNTESTED;

	case 0: /* child */
	    exit (_cairo_test_context_run_for_target (ctx, target,
						      similar, device_offset));

	default:
	    return _cairo_test_wait (pid);
	}
    }
#endif
    return _cairo_test_context_run_for_target (ctx, target,
					       similar, device_offset);
}

static void
append_argv (int *argc, char ***argv, const char *str)
{
    int old_argc;
    char **old_argv;
    cairo_bool_t doit;
    const char *s, *t;
    int olen;
    int len;
    int i;

    if (str == NULL)
	return;

    old_argc = *argc;
    old_argv = *argv;

    doit = FALSE;
    do {
	if (doit)
	    *argv = xmalloc (sizeof (char *) * (1 + *argc) + olen);

	olen = sizeof (char *) * (1 + *argc);
	for (i = 0; i < old_argc; i++) {
	    len = strlen (old_argv[i]) + 1;
	    if (doit) {
		(*argv)[i] = (char *) *argv + olen;
		memcpy ((*argv)[i], old_argv[i], len);
	    }
	    olen += len;
	}

	s = str;
	while ((t = strpbrk (s, " \t,:;")) != NULL) {
	    if (t - s) {
		len = t - s;
		if (doit) {
		    (*argv)[i] = (char *) *argv + olen;
		    memcpy ((*argv)[i], s, len);
		    (*argv)[i][len] = '\0';
		}
		olen += len + 1;
		i++;
	    }
	    s = t + 1;
	}
	if (*s != '\0') {
	    len = strlen (s) + 1;
	    if (doit) {
		(*argv)[i] = (char *) *argv + olen;
		memcpy ((*argv)[i], s, len);
	    }
	    olen += len;
	    i++;
	}
    } while (doit++ == FALSE);
    (*argv)[i] = NULL;
    *argc += i;
}

static void
usage (const char *argv0)
{
    fprintf (stderr,
	     "Usage: %s [-afkxsl] [test-names|keywords ...]\n"
	     "       %s -l\n"
	     "\n"
	     "Run the cairo conformance test suite over the given tests (all by default)\n"
	     "The command-line arguments are interpreted as follows:\n"
	     "\n"
	     "  -a	all; run the full set of tests. By default the test suite\n"
	     "          skips similar surface and device offset testing.\n"
	     "  -f	foreground; do not fork\n"
	     "  -k	match tests by keyword\n"
	     "  -s	include slow, long running tests\n"
	     "  -x	exit on first failure\n"
	     "  -l	list only; just list selected test case names without executing\n"
	     "\n"
	     "If test names are given they are used as matches either to a specific\n"
	     "test case or to a keyword, so a command such as\n"
	     "\"cairo-test-suite -k text\" can be used to run all text test cases, and\n"
	     "\"cairo-test-suite text-transform\" to run the individual case.\n",
	     argv0, argv0);
}

static void
_parse_cmdline (cairo_test_runner_t *runner, int *argc, char **argv[])
{
    int c;

    while (1) {
	c = _cairo_getopt (*argc, *argv, ":afkxsl");
	if (c == -1)
	    break;

	switch (c) {
	case 'a':
	    runner->full_test = TRUE;
	    break;
	case 's':
	    runner->slow = TRUE;
	    break;
	case 'l':
	    runner->list_only = TRUE;
	    break;
	case 'f':
	    runner->foreground = TRUE;
	    break;
	case 'x':
	    runner->exit_on_failure = TRUE;
	    break;
	case 'k':
	    runner->keyword_match = TRUE;
	    break;
	default:
	    fprintf (stderr, "Internal error: unhandled option: %c\n", c);
	    /* fall-through */
	case '?':
	    usage ((*argv)[0]);
	    exit (1);
	}
    }

    *argc -= optind;
    *argv += optind;
}

static void
_runner_init (cairo_test_runner_t *runner)
{
    cairo_test_init (&runner->base, "cairo-test-suite");

    runner->passed = TRUE;

    runner->fails_preamble = NULL;
    runner->crashes_preamble = NULL;
    runner->errors_preamble = NULL;

    runner->fails_per_target = xcalloc (sizeof (cairo_test_list_t *),
					runner->base.num_targets);
    runner->crashes_per_target = xcalloc (sizeof (cairo_test_list_t *),
					  runner->base.num_targets);
    runner->errors_per_target = xcalloc (sizeof (cairo_test_list_t *),
					  runner->base.num_targets);
    runner->num_failed_per_target = xcalloc (sizeof (int),
					     runner->base.num_targets);
    runner->num_error_per_target = xcalloc (sizeof (int),
					     runner->base.num_targets);
    runner->num_crashed_per_target = xcalloc (sizeof (int),
					      runner->base.num_targets);
}

static void
_runner_print_versions (cairo_test_runner_t *runner)
{
    _log (&runner->base,
	 "Compiled against cairo %s, running on %s.\n",
	 CAIRO_VERSION_STRING, cairo_version_string ());
    _log (&runner->base,
	 "Compiled against pixman %s, running on %s.\n",
	 PIXMAN_VERSION_STRING, pixman_version_string ());

    fflush (runner->base.log_file);
}

static void
_runner_print_summary (cairo_test_runner_t *runner)
{
    _log (&runner->base,
	  "%d Passed, %d Failed [%d crashed, %d expected], %d Skipped\n",
	  runner->num_passed,

	  runner->num_failed + runner->num_crashed + runner->num_xfailed,
	  runner->num_crashed,
	  runner->num_xfailed,

	  runner->num_skipped);
}

static void
_runner_print_details (cairo_test_runner_t *runner)
{
    cairo_test_list_t *list;
    unsigned int n;

    if (runner->crashes_preamble) {
	int count = 0;

	for (list = runner->crashes_preamble; list != NULL; list = list->next)
	    count++;

	_log (&runner->base, "Preamble: %d crashed! -", count);

	for (list = runner->crashes_preamble; list != NULL; list = list->next) {
	    char *name = cairo_test_get_name (list->test);
	    _log (&runner->base, " %s", name);
	    free (name);
	}
	_log (&runner->base, "\n");
    }
    if (runner->errors_preamble) {
	int count = 0;

	for (list = runner->errors_preamble; list != NULL; list = list->next)
	    count++;

	_log (&runner->base, "Preamble: %d error -", count);

	for (list = runner->errors_preamble; list != NULL; list = list->next) {
	    char *name = cairo_test_get_name (list->test);
	    _log (&runner->base, " %s", name);
	    free (name);
	}
	_log (&runner->base, "\n");
    }
    if (runner->fails_preamble) {
	int count = 0;

	for (list = runner->fails_preamble; list != NULL; list = list->next)
	    count++;

	_log (&runner->base, "Preamble: %d failed -", count);

	for (list = runner->fails_preamble; list != NULL; list = list->next) {
	    char *name = cairo_test_get_name (list->test);
	    _log (&runner->base, " %s", name);
	    free (name);
	}
	_log (&runner->base, "\n");
    }

    for (n = 0; n < runner->base.num_targets; n++) {
	const cairo_boilerplate_target_t *target;

	target = runner->base.targets_to_test[n];
	if (runner->num_crashed_per_target[n]) {
	    _log (&runner->base, "%s (%s): %d crashed! -",
		  target->name,
		  cairo_boilerplate_content_name (target->content),
		  runner->num_crashed_per_target[n]);

	    for (list = runner->crashes_per_target[n];
		 list != NULL;
		 list = list->next)
	    {
		char *name = cairo_test_get_name (list->test);
		_log (&runner->base, " %s", name);
		free (name);
	    }
	    _log (&runner->base, "\n");
	}
	if (runner->num_error_per_target[n]) {
	    _log (&runner->base, "%s (%s): %d error -",
		  target->name,
		  cairo_boilerplate_content_name (target->content),
		  runner->num_error_per_target[n]);

	    for (list = runner->errors_per_target[n];
		 list != NULL;
		 list = list->next)
	    {
		char *name = cairo_test_get_name (list->test);
		_log (&runner->base, " %s", name);
		free (name);
	    }
	    _log (&runner->base, "\n");
	}

	if (runner->num_failed_per_target[n]) {
	    _log (&runner->base, "%s (%s): %d failed -",
		  target->name,
		  cairo_boilerplate_content_name (target->content),
		  runner->num_failed_per_target[n]);

	    for (list = runner->fails_per_target[n];
		 list != NULL;
		 list = list->next)
	    {
		char *name = cairo_test_get_name (list->test);
		_log (&runner->base, " %s", name);
		free (name);
	    }
	    _log (&runner->base, "\n");
	}
    }
}

static void
_runner_print_results (cairo_test_runner_t *runner)
{
    _runner_print_summary (runner);
    _runner_print_details (runner);

    if (! runner->passed && ! runner->num_crashed) {
	_log (&runner->base,
"\n"
"Note: These failures may be due to external factors.\n"
"Please read test/README -- \"Getting the elusive zero failures\".\n");
    }
}

static cairo_test_status_t
_runner_fini (cairo_test_runner_t *runner)
{
    unsigned int n;

    _list_free (runner->crashes_preamble);
    _list_free (runner->errors_preamble);
    _list_free (runner->fails_preamble);

    for (n = 0; n < runner->base.num_targets; n++) {
	_list_free (runner->crashes_per_target[n]);
	_list_free (runner->errors_per_target[n]);
	_list_free (runner->fails_per_target[n]);
    }
    free (runner->crashes_per_target);
    free (runner->errors_per_target);
    free (runner->fails_per_target);

    free (runner->num_crashed_per_target);
    free (runner->num_error_per_target);
    free (runner->num_failed_per_target);

    cairo_test_fini (&runner->base);

    if (runner->force_pass)
	return CAIRO_TEST_SUCCESS;

    return runner->num_failed + runner->num_crashed ?
	CAIRO_TEST_FAILURE :
	runner->num_passed + runner->num_xfailed ?
	CAIRO_TEST_SUCCESS : CAIRO_TEST_UNTESTED;
}

static cairo_bool_t
_version_compare (int a, cairo_test_compare_op_t op, int b)
{
    switch (op) {
    case GT: return a > b;
    case GE: return a >= b;
    default: return FALSE;
    }
}


static cairo_bool_t
_get_required_version (const char *str,
		       cairo_test_compare_op_t *op,
		       int *major,
		       int *minor,
		       int *micro)
{
    while (*str == ' ')
	str++;

    if (strncmp (str, ">=", 2) == 0) {
	*op = GE;
	str += 2;
    } else if (strncmp (str, ">", 1) == 0) {
	*op = GT;
	str += 1;
    } else
	return FALSE;

    while (*str == ' ')
	str++;

    if (sscanf (str, "%d.%d.%d", major, minor, micro) != 3) {
	*micro = 0;
	if (sscanf (str, "%d.%d", major, minor) != 2)
	    return FALSE;
    }

    return TRUE;
}

static cairo_bool_t
_has_required_cairo_version (const char *str)
{
    cairo_test_compare_op_t op;
    int major, minor, micro;

    if (! _get_required_version (str + 5 /* advance over "cairo" */,
				 &op, &major, &minor, &micro))
    {
	fprintf (stderr, "unrecognised cairo version requirement '%s'\n", str);
	return FALSE;
    }

    return _version_compare (cairo_version (),
			     op,
			     CAIRO_VERSION_ENCODE (major, minor, micro));
}

static cairo_bool_t
_has_required_ghostscript_version (const char *str)
{
#if ! CAIRO_CAN_TEST_PS_SURFACE
    return TRUE;
#endif

    str += 2; /* advance over "gs" */

    return TRUE;
}

static cairo_bool_t
_has_required_poppler_version (const char *str)
{
#if ! CAIRO_CAN_TEST_PDF_SURFACE
    return TRUE;
#endif

    str += 7; /* advance over "poppler" */

    return TRUE;
}

static cairo_bool_t
_has_required_rsvg_version (const char *str)
{
#if ! CAIRO_CAN_TEST_SVG_SURFACE
    return TRUE;
#endif

    str += 4; /* advance over "rsvg" */

    return TRUE;
}

int
main (int argc, char **argv)
{
    cairo_test_runner_t runner;
    cairo_test_t *test;
    cairo_test_status_t *target_status;
    unsigned int n, m;
    char targets[4096];
    int len;

#ifdef _MSC_VER
    /* We don't want an assert dialog, we want stderr */
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif

    _cairo_test_runner_register_tests ();

    memset (&runner, 0, sizeof (runner));
    runner.num_device_offsets = 1;

    if (is_running_under_debugger ())
	runner.foreground = TRUE;

    if (getenv ("CAIRO_TEST_MODE")) {
	const char *env = getenv ("CAIRO_TEST_MODE");

	if (strstr (env, "full")) {
	    runner.full_test = TRUE;
	}
	if (strstr (env, "foreground")) {
	    runner.foreground = TRUE;
	}
	if (strstr (env, "exit-on-failure")) {
	    runner.exit_on_failure = TRUE;
	}
    }

    if (getenv ("CAIRO_TEST_FORCE_PASS")) {
	const char *env = getenv ("CAIRO_TEST_FORCE_PASS");

	runner.force_pass = atoi (env);
    }

    _parse_cmdline (&runner, &argc, &argv);
    append_argv (&argc, &argv, getenv ("CAIRO_TESTS"));

    if (runner.full_test) {
	runner.num_device_offsets = 2;
    }

    target_status = NULL; /* silence the compiler */
    if (! runner.list_only) {
	_runner_init (&runner);
	_runner_print_versions (&runner);
	target_status = xmalloc (sizeof (cairo_test_status_t) *
				 runner.base.num_targets);
    }

    for (test = tests; test != NULL; test = test->next) {
	cairo_test_context_t ctx;
	cairo_test_status_t status;
	cairo_bool_t failed = FALSE, xfailed = FALSE, error = FALSE, crashed = FALSE, skipped = TRUE;
	cairo_bool_t in_preamble = FALSE;
	char *name = cairo_test_get_name (test);
	int i;

	/* check for restricted runs */
	if (argc) {
	    cairo_bool_t found = FALSE;
	    const char *keywords = test->keywords;

	    for (i = 0; i < argc; i++) {
		const char *match = argv[i];
		cairo_bool_t invert = match[0] == '!';
		if (invert)
		    match++;

		if (runner.keyword_match) {
		    if (keywords != NULL && strstr (keywords, match) != NULL) {
			found = ! invert;
			break;
		    } else if (invert) {
			found = TRUE;
		    }
		} else {
		    /* exact match on test name */
		    if (strcmp (name, match) == 0) {
			found = ! invert;
			break;
		    } else if (invert) {
			found = TRUE;
		    }
		}
	    }

	    if (! found) {
		free (name);
		continue;
	    }
	}

	/* check to see if external requirements match */
	if (test->requirements != NULL) {
	    const char *requirements = test->requirements;
	    const char *str;

	    str = strstr (requirements, "slow");
	    if (str != NULL && ! runner.slow) {
		if (runner.list_only)
		    goto TEST_NEXT;
		else
		    goto TEST_SKIPPED;
	    }

	    str = strstr (requirements, "cairo");
	    if (str != NULL && ! _has_required_cairo_version (str)) {
		if (runner.list_only)
		    goto TEST_NEXT;
		else
		    goto TEST_SKIPPED;
	    }

	    str = strstr (requirements, "gs");
	    if (str != NULL && ! _has_required_ghostscript_version (str)) {
		if (runner.list_only)
		    goto TEST_NEXT;
		else
		    goto TEST_SKIPPED;
	    }

	    str = strstr (requirements, "poppler");
	    if (str != NULL && ! _has_required_poppler_version (str)) {
		if (runner.list_only)
		    goto TEST_NEXT;
		else
		    goto TEST_SKIPPED;
	    }

	    str = strstr (requirements, "rsvg");
	    if (str != NULL && ! _has_required_rsvg_version (str)) {
		if (runner.list_only)
		    goto TEST_NEXT;
		else
		    goto TEST_SKIPPED;
	    }
	}

	if (runner.list_only) {
	    printf ("%s ", name);
	    goto TEST_NEXT;
	}

	_cairo_test_context_init_for_test (&ctx, &runner.base, test);
	memset (target_status, 0,
		sizeof (cairo_test_status_t) * ctx.num_targets);

	if (ctx.test->preamble != NULL) {
	    status = _cairo_test_runner_preamble (&runner, &ctx);
	    switch (status) {
	    case CAIRO_TEST_SUCCESS:
		in_preamble = TRUE;
		skipped = FALSE;
		break;

	    case CAIRO_TEST_XFAILURE:
		in_preamble = TRUE;
		xfailed = TRUE;
		goto TEST_DONE;

	    case CAIRO_TEST_NEW:
	    case CAIRO_TEST_FAILURE:
		runner.fails_preamble = _list_prepend (runner.fails_preamble,
						       test);
		in_preamble = TRUE;
		failed = TRUE;
		goto TEST_DONE;

	    case CAIRO_TEST_ERROR:
		runner.errors_preamble = _list_prepend (runner.errors_preamble,
							 test);
		in_preamble = TRUE;
		failed = TRUE;
		goto TEST_DONE;

	    case CAIRO_TEST_NO_MEMORY:
	    case CAIRO_TEST_CRASHED:
		runner.crashes_preamble = _list_prepend (runner.crashes_preamble,
							 test);
		in_preamble = TRUE;
		failed = TRUE;
		goto TEST_DONE;

	    case CAIRO_TEST_UNTESTED:
		goto TEST_DONE;
	    }
	}

	if (ctx.test->draw == NULL)
	    goto TEST_DONE;

	for (n = 0; n < ctx.num_targets; n++) {
	    const cairo_boilerplate_target_t *target;
	    cairo_bool_t target_failed = FALSE,
			 target_xfailed = FALSE,
			 target_error = FALSE,
			 target_crashed = FALSE,
			 target_skipped = TRUE;
	    int has_similar;

	    target = ctx.targets_to_test[n];

	    has_similar = runner.full_test ?
			  cairo_test_target_has_similar (&ctx, target) :
		          0;
	    for (m = 0; m < runner.num_device_offsets; m++) {
		int dev_offset = m * 25;
		int similar;

		for (similar = 0; similar <= has_similar; similar++) {
		    status = _cairo_test_runner_draw (&runner, &ctx, target,
						      similar, dev_offset);
		    switch (status) {
		    case CAIRO_TEST_SUCCESS:
			target_skipped = FALSE;
			break;
		    case CAIRO_TEST_XFAILURE:
			target_xfailed = TRUE;
			break;
		    case CAIRO_TEST_NEW:
		    case CAIRO_TEST_FAILURE:
			target_failed = TRUE;
			break;
		    case CAIRO_TEST_ERROR:
			target_error = TRUE;
			break;
		    case CAIRO_TEST_NO_MEMORY:
		    case CAIRO_TEST_CRASHED:
			target_crashed = TRUE;
			break;
		    case CAIRO_TEST_UNTESTED:
			break;
		    }
		}
	    }

	    if (target_crashed) {
		target_status[n] = CAIRO_TEST_CRASHED;
		runner.num_crashed_per_target[n]++;
		runner.crashes_per_target[n] = _list_prepend (runner.crashes_per_target[n],
							      test);
		crashed = TRUE;
	    } else if (target_error) {
		target_status[n] = CAIRO_TEST_ERROR;
		runner.num_error_per_target[n]++;
		runner.errors_per_target[n] = _list_prepend (runner.errors_per_target[n],
							     test);

		error = TRUE;
	    } else if (target_failed) {
		target_status[n] = CAIRO_TEST_FAILURE;
		runner.num_failed_per_target[n]++;
		runner.fails_per_target[n] = _list_prepend (runner.fails_per_target[n],
							    test);

		failed = TRUE;
	    } else if (target_xfailed) {
		target_status[n] = CAIRO_TEST_XFAILURE;
		xfailed = TRUE;
	    } else if (target_skipped) {
		target_status[n] = CAIRO_TEST_UNTESTED;
	    } else {
		target_status[n] = CAIRO_TEST_SUCCESS;
		skipped = FALSE;
	    }
	}

  TEST_DONE:
	cairo_test_fini (&ctx);
  TEST_SKIPPED:
	targets[0] = '\0';
	if (crashed) {
	    if (! in_preamble) {
		len = 0;
		for (n = 0 ; n < runner.base.num_targets; n++) {
		    if (target_status[n] == CAIRO_TEST_CRASHED) {
			if (strstr (targets,
				    runner.base.targets_to_test[n]->name) == NULL)
			{
			    len += snprintf (targets + len, sizeof (targets) - len,
					     "%s, ",
					     runner.base.targets_to_test[n]->name);
			}
		    }
		}
		targets[len-2] = '\0';
		_log (&runner.base, "\n%s: CRASH! (%s)\n", name, targets);
	    } else {
		_log (&runner.base, "\n%s: CRASH!\n", name);
	    }
	    runner.num_crashed++;
	    runner.passed = FALSE;
	} else if (error) {
	    if (! in_preamble) {
		len = 0;
		for (n = 0 ; n < runner.base.num_targets; n++) {
		    if (target_status[n] == CAIRO_TEST_ERROR) {
			if (strstr (targets,
				    runner.base.targets_to_test[n]->name) == NULL)
			{
			    len += snprintf (targets + len,
					     sizeof (targets) - len,
					     "%s, ",
					     runner.base.targets_to_test[n]->name);
			}
		    }
		}
		targets[len-2] = '\0';
		_log (&runner.base, "%s: ERROR (%s)\n", name, targets);
	    } else {
		_log (&runner.base, "%s: ERROR\n", name);
	    }
	    runner.num_error++;
	    runner.passed = FALSE;
	} else if (failed) {
	    if (! in_preamble) {
		len = 0;
		for (n = 0 ; n < runner.base.num_targets; n++) {
		    if (target_status[n] == CAIRO_TEST_FAILURE) {
			if (strstr (targets,
				    runner.base.targets_to_test[n]->name) == NULL)
			{
			    len += snprintf (targets + len,
					     sizeof (targets) - len,
					     "%s, ",
					     runner.base.targets_to_test[n]->name);
			}
		    }
		}
		targets[len-2] = '\0';
		_log (&runner.base, "%s: FAIL (%s)\n", name, targets);
	    } else {
		_log (&runner.base, "%s: FAIL\n", name);
	    }
	    runner.num_failed++;
	    runner.passed = FALSE;
	} else if (xfailed) {
	    _log (&runner.base, "%s: XFAIL\n", name);
	    runner.num_xfailed++;
	} else if (skipped) {
	    _log (&runner.base, "%s: UNTESTED\n", name);
	    runner.num_skipped++;
	} else {
	    _log (&runner.base, "%s: PASS\n", name);
	    runner.num_passed++;
	}
	fflush (runner.base.log_file);

  TEST_NEXT:
	free (name);
	if (runner.exit_on_failure && ! runner.passed)
	    break;

    }

    if (runner.list_only) {
	printf ("\n");
	return CAIRO_TEST_SUCCESS;
    }

    for (n = 0 ; n < runner.base.num_targets; n++) {
	runner.crashes_per_target[n] = _list_reverse (runner.crashes_per_target[n]);
	runner.errors_per_target[n] = _list_reverse (runner.errors_per_target[n]);
	runner.fails_per_target[n] = _list_reverse (runner.fails_per_target[n]);
    }

    _runner_print_results (&runner);

    free (target_status);
    return _runner_fini (&runner);
}

void
cairo_test_register (cairo_test_t *test)
{
    test->next = tests;
    tests = test;
}
