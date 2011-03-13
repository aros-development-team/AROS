/*
 * Copyright © 2004 Red Hat, Inc.
 * Copyright © 2008 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Carl D. Worth <cworth@cworth.org>
 *         Chris Wilson <chris@chris-wilson.co.uk>
 */

#define _GNU_SOURCE 1	/* for feenableexcept() et al */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#if HAVE_FEENABLEEXCEPT
#include <fenv.h>
#endif
#include <assert.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>
#if HAVE_FCFINI
#include <fontconfig/fontconfig.h>
#endif
#if CAIRO_HAS_REAL_PTHREAD
#include <pthread.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_VALGRIND
#include <valgrind.h>
#else
#define RUNNING_ON_VALGRIND 0
#endif

#if HAVE_MEMFAULT
#include <memfault.h>
#define MF(x) x
#else
#define MF(x)
#endif

#include "cairo-test-private.h"

#include "buffer-diff.h"

#ifdef _MSC_VER
#include <crtdbg.h>
#define vsnprintf _vsnprintf
#define access _access
#define F_OK 0
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !FALSE
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof(A) / sizeof (A[0]))
#endif

#if ! HAVE_ALARM
#define alarm(X);
#endif

static const cairo_user_data_key_t _cairo_test_context_key;

static void
_xunlink (const cairo_test_context_t *ctx, const char *pathname);

static const char *fail_face = "", *xfail_face="", *normal_face = "";
static cairo_bool_t print_fail_on_stdout;
static int cairo_test_timeout = 60;

#define NUM_DEVICE_OFFSETS 2

static char *
_cairo_test_fixup_name (const char *original)
{
    int len = strlen (original);
    char *name, *s;

    name = xmalloc (len + 1);
    s = memcpy (name, original, len + 1);
    while ((s = strchr (s, '_')) != NULL)
	*s++ = '-';

    return name;
}

char *
cairo_test_get_name (const cairo_test_t *test)
{
    return _cairo_test_fixup_name (test->name);
}

static void
_cairo_test_init (cairo_test_context_t *ctx,
		  const cairo_test_context_t *parent,
		  const cairo_test_t *test,
		  const char *test_name)
{
    char *log_name;

    MF (MEMFAULT_DISABLE_FAULTS ());

#if HAVE_FEENABLEEXCEPT
    feenableexcept (FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

    ctx->test = test;
    ctx->test_name = _cairo_test_fixup_name (test_name);

    ctx->malloc_failure = 0;
#if HAVE_MEMFAULT
    if (getenv ("CAIRO_TEST_MALLOC_FAILURE"))
	ctx->malloc_failure = atoi (getenv ("CAIRO_TEST_MALLOC_FAILURE"));
    if (ctx->malloc_failure && ! RUNNING_ON_MEMFAULT ())
	ctx->malloc_failure = 0;
#endif

    ctx->timeout = cairo_test_timeout;
    if (getenv ("CAIRO_TEST_TIMEOUT"))
	ctx->timeout = atoi (getenv ("CAIRO_TEST_TIMEOUT"));

    xasprintf (&log_name, "%s%s", ctx->test_name, CAIRO_TEST_LOG_SUFFIX);
    _xunlink (NULL, log_name);

    ctx->log_file = fopen (log_name, "a");
    if (ctx->log_file == NULL) {
	fprintf (stderr, "Error opening log file: %s\n", log_name);
	ctx->log_file = stderr;
    }
    free (log_name);

    ctx->ref_name = NULL;
    ctx->ref_image = NULL;
    ctx->ref_image_flattened = NULL;

    ctx->thread = 0;

    if (parent != NULL) {
	ctx->targets_to_test = parent->targets_to_test;
	ctx->num_targets = parent->num_targets;
	ctx->limited_targets = parent->limited_targets;
	ctx->own_targets = FALSE;

	ctx->srcdir = parent->srcdir;
	ctx->refdir = parent->refdir;
    } else {
	int tmp_num_targets;
	cairo_bool_t tmp_limited_targets;

	ctx->targets_to_test = cairo_boilerplate_get_targets (&tmp_num_targets, &tmp_limited_targets);
	ctx->num_targets = tmp_num_targets;
	ctx->limited_targets = tmp_limited_targets;
	ctx->own_targets = TRUE;

	ctx->srcdir = getenv ("srcdir");
	if (ctx->srcdir == NULL)
	    ctx->srcdir = ".";

	ctx->refdir = getenv ("CAIRO_REF_DIR");
    }

#ifdef HAVE_UNISTD_H
    if (*fail_face == '\0' && isatty (2)) {
	fail_face = "\033[41;37;1m";
	xfail_face = "\033[43;37;1m";
	normal_face = "\033[m";
	if (isatty (1))
	    print_fail_on_stdout = FALSE;
    }
#endif

    printf ("\nTESTING %s\n", ctx->test_name);
}

void
_cairo_test_context_init_for_test (cairo_test_context_t *ctx,
				   const cairo_test_context_t *parent,
				   const cairo_test_t *test)
{
    _cairo_test_init (ctx, parent, test, test->name);
}

void
cairo_test_init (cairo_test_context_t *ctx,
		 const char *test_name)
{
    _cairo_test_init (ctx, NULL, NULL, test_name);
}

static void
cairo_test_init_thread (cairo_test_context_t *ctx,
			cairo_test_context_t *master,
			int thread)
{
    MF (MEMFAULT_DISABLE_FAULTS ());

    *ctx = *master;
    ctx->thread = thread;
}

void
cairo_test_fini (cairo_test_context_t *ctx)
{
    if (ctx->thread != 0)
	return;

    if (ctx->log_file == NULL)
	return;

    if (ctx->log_file != stderr)
	fclose (ctx->log_file);
    ctx->log_file = NULL;

    if (ctx->ref_name != NULL)
	free (ctx->ref_name);
    cairo_surface_destroy (ctx->ref_image);
    cairo_surface_destroy (ctx->ref_image_flattened);

    if (ctx->test_name != NULL)
	free ((char *) ctx->test_name);

    if (ctx->own_targets)
	cairo_boilerplate_free_targets (ctx->targets_to_test);

    cairo_boilerplate_fini ();

    cairo_debug_reset_static_data ();
#if HAVE_FCFINI
    FcFini ();
#endif
}

void
cairo_test_logv (const cairo_test_context_t *ctx,
	        const char *fmt, va_list va)
{
    FILE *file = ctx && ctx->log_file ? ctx->log_file : stderr;
    vfprintf (file, fmt, va);
}

void
cairo_test_log (const cairo_test_context_t *ctx, const char *fmt, ...)
{
    va_list va;
    FILE *file = ctx && ctx->log_file ? ctx->log_file : stderr;

    va_start (va, fmt);
    vfprintf (file, fmt, va);
    va_end (va);
}

void
cairo_test_log_path (const cairo_test_context_t *ctx,
		     const cairo_path_t *path)
{
  int i;

  for (i = 0; i < path->num_data; i += path->data[i].header.length) {
    cairo_path_data_t *data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
	cairo_test_log (ctx,
		        "    cairo_move_to (cr, %g, %g);\n",
			data[1].point.x, data[1].point.y);
        break;
    case CAIRO_PATH_LINE_TO:
	cairo_test_log (ctx,
		        "    cairo_line_to (cr, %g, %g);\n",
			data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_CURVE_TO:
	cairo_test_log (ctx,
		        "    cairo_curve_to (cr, %g, %g, %g, %g, %g, %g);\n",
			data[1].point.x, data[1].point.y,
			data[2].point.x, data[2].point.y,
			data[3].point.x, data[3].point.y);
	break;
    case CAIRO_PATH_CLOSE_PATH:
	cairo_test_log (ctx,
		        "    cairo_close_path (cr);\n\n");
	break;
    default:
	assert (0);
    }
  }
}

static void
_xunlink (const cairo_test_context_t *ctx, const char *pathname)
{
    if (unlink (pathname) < 0 && errno != ENOENT) {
	cairo_test_log (ctx, "Error: Cannot remove %s: %s\n",
			pathname, strerror (errno));
	exit (1);
    }
}

char *
cairo_test_reference_filename (const cairo_test_context_t *ctx,
			       const char *base_name,
			       const char *test_name,
			       const char *target_name,
			       const char *base_target_name,
			       const char *format,
			       const char *suffix,
			       const char *extension)
{
    char *ref_name = NULL;

    /* First look for a previous build for comparison. */
    if (ctx->refdir != NULL) {
	xasprintf (&ref_name, "%s/%s%s%s",
		   ctx->refdir,
		   base_name,
		   suffix,
		   extension);
	if (access (ref_name, F_OK) != 0)
	    free (ref_name);
	else
	    goto done;
    }

    if (target_name != NULL) {
	/* Next look for a target/format-specific reference image. */
	xasprintf (&ref_name, "%s/%s.%s.%s%s%s",
		   ctx->srcdir,
		   test_name,
		   target_name,
		   format,
		   suffix,
		   extension);
	if (access (ref_name, F_OK) != 0)
	    free (ref_name);
	else
	    goto done;

	/* Next, look for target-specific reference image. */
	xasprintf (&ref_name, "%s/%s.%s%s%s",
		   ctx->srcdir,
		   test_name,
		   target_name,
		   suffix,
		   extension);
	if (access (ref_name, F_OK) != 0)
	    free (ref_name);
	else
	    goto done;
    }

    if (base_target_name != NULL) {
	/* Next look for a base/format-specific reference image. */
	xasprintf (&ref_name, "%s/%s.%s.%s%s%s",
		   ctx->srcdir,
		   test_name,
		   base_target_name,
		   format,
		   suffix,
		   extension);
	if (access (ref_name, F_OK) != 0)
	    free (ref_name);
	else
	    goto done;

	/* Next, look for base-specific reference image. */
	xasprintf (&ref_name, "%s/%s.%s%s%s",
		   ctx->srcdir,
		   test_name,
		   base_target_name,
		   suffix,
		   extension);
	if (access (ref_name, F_OK) != 0)
	    free (ref_name);
	else
	    goto done;
    }

    /* Next, look for format-specific reference image. */
    xasprintf (&ref_name, "%s/%s.%s%s%s",
	       ctx->srcdir,
	       test_name,
	       format,
	       suffix,
	       extension);
    if (access (ref_name, F_OK) != 0)
	free (ref_name);
    else
	goto done;

    /* Finally, look for the standard reference image. */
    xasprintf (&ref_name, "%s/%s%s%s", ctx->srcdir,
	       test_name,
	       suffix,
	       extension);
    if (access (ref_name, F_OK) != 0)
	free (ref_name);
    else
	goto done;

    ref_name = NULL;

done:
    return ref_name;
}

static cairo_bool_t
_cairo_test_mkdir (const char *path)
{
#if ! HAVE_MKDIR
    return FALSE;
#elif HAVE_MKDIR == 1
    if (mkdir (path) == 0)
	return TRUE;
#elif HAVE_MKDIR == 2
    if (mkdir (path, 0770) == 0)
	return TRUE;
#else
#error Bad value for HAVE_MKDIR
#endif

    return errno == EEXIST;
}

cairo_bool_t
cairo_test_target_has_similar (const cairo_test_context_t *ctx,
			       const cairo_boilerplate_target_t *target)
{
    cairo_surface_t *surface;
    cairo_bool_t has_similar;
    cairo_t * cr;
    cairo_surface_t *similar;
    cairo_status_t status;
    void *closure;
    char *path;

    /* ignore image intermediate targets */
    if (target->expected_type == CAIRO_SURFACE_TYPE_IMAGE)
	return FALSE;

    if (getenv ("CAIRO_TEST_IGNORE_SIMILAR"))
	return FALSE;

    xasprintf (&path, "%s/%s",
	       _cairo_test_mkdir (CAIRO_TEST_OUTPUT_DIR) ? CAIRO_TEST_OUTPUT_DIR : ".",
	       ctx->test_name);

    has_similar = FALSE;
    do {
	do {
	    surface = (target->create_surface) (path,
						target->content,
						ctx->test->width,
						ctx->test->height,
						ctx->test->width + 25 * NUM_DEVICE_OFFSETS,
						ctx->test->height + 25 * NUM_DEVICE_OFFSETS,
						CAIRO_BOILERPLATE_MODE_TEST,
						0,
						&closure);
	    if (surface == NULL)
		goto out;
	} while (cairo_test_malloc_failure (ctx, cairo_surface_status (surface)));

	if (cairo_surface_status (surface))
	    goto out;

	cr = cairo_create (surface);
	cairo_push_group_with_content (cr,
				       cairo_boilerplate_content (target->content));
	similar = cairo_get_group_target (cr);
	status = cairo_surface_status (similar);

	has_similar = cairo_surface_get_type (similar) == cairo_surface_get_type (surface);

	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	if (target->cleanup)
	    target->cleanup (closure);
    } while (! has_similar && cairo_test_malloc_failure (ctx, status));
out:
    free (path);

    return has_similar;
}

static cairo_surface_t *
_cairo_test_flatten_reference_image (cairo_test_context_t *ctx,
				     cairo_bool_t flatten)
{
    cairo_surface_t *surface;
    cairo_t *cr;

    if (! flatten)
	return ctx->ref_image;

    if (ctx->ref_image_flattened != NULL)
	return ctx->ref_image_flattened;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					  cairo_image_surface_get_width (ctx->ref_image),
					  cairo_image_surface_get_height (ctx->ref_image));
    cr = cairo_create (surface);
    cairo_surface_destroy (surface);

    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_paint (cr);

    cairo_set_source_surface (cr, ctx->ref_image, 0, 0);
    cairo_paint (cr);

    surface = cairo_surface_reference (cairo_get_target (cr));
    cairo_destroy (cr);

    if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
	ctx->ref_image_flattened = surface;
    return surface;
}

cairo_surface_t *
cairo_test_get_reference_image (cairo_test_context_t *ctx,
				const char *filename,
				cairo_bool_t flatten)
{
    cairo_surface_t *surface;
    int len;

    if (ctx->ref_name != NULL) {
	if (strcmp (ctx->ref_name, filename) == 0)
	    return _cairo_test_flatten_reference_image (ctx, flatten);

	cairo_surface_destroy (ctx->ref_image);
	ctx->ref_image = NULL;

	cairo_surface_destroy (ctx->ref_image_flattened);
	ctx->ref_image_flattened = NULL;

	free (ctx->ref_name);
	ctx->ref_name = NULL;
    }

    surface = cairo_image_surface_create_from_png (filename);
    if (cairo_surface_status (surface))
	return surface;

    len = strlen (filename);
    ctx->ref_name = xmalloc (len + 1);
    memcpy (ctx->ref_name, filename, len + 1);

    ctx->ref_image = surface;
    return _cairo_test_flatten_reference_image (ctx, flatten);
}

static cairo_bool_t
cairo_test_file_is_older (const char *filename,
	                  char **ref_filenames,
			  int num_ref_filenames)
{
#if HAVE_SYS_STAT_H
    struct stat st;

    if (stat (filename, &st) < 0)
	return FALSE;

    while (num_ref_filenames--) {
	struct stat ref;
	char *ref_filename = *ref_filenames++;

	if (ref_filename == NULL)
	    continue;

	if (stat (ref_filename++, &ref) < 0)
	    continue;

	if (st.st_mtime <= ref.st_mtime)
	    return TRUE;
    }
#endif

    return FALSE;
}

static cairo_bool_t
cairo_test_files_equal (const char *test_filename,
			const char *pass_filename)
{
    FILE *test, *pass;
    int t, p;

    if (test_filename == NULL || pass_filename == NULL)
	return FALSE;

    test = fopen (test_filename, "rb");
    if (test == NULL)
	return FALSE;

    pass = fopen (pass_filename, "rb");
    if (pass == NULL) {
	fclose (test);
	return FALSE;
    }

    /* as simple as it gets */
    do {
	t = getc (test);
	p = getc (pass);
	if (t != p)
	    break;
    } while (t != EOF && p != EOF);

    fclose (pass);
    fclose (test);

    return t == p; /* both EOF */
}

static cairo_bool_t
cairo_test_copy_file (const char *src_filename,
		      const char *dst_filename)
{
    FILE *src, *dst;
    int c;

#if HAVE_LINK
    if (link (src_filename, dst_filename) == 0)
	return TRUE;

    unlink (dst_filename);
#endif

    src = fopen (src_filename, "rb");
    if (src == NULL)
	return FALSE;

    dst = fopen (dst_filename, "wb");
    if (dst == NULL) {
	fclose (src);
	return FALSE;
    }

    /* as simple as it gets */
    while ((c = getc (src)) != EOF)
	putc (c, dst);

    fclose (src);
    fclose (dst);

    return TRUE;
}

static cairo_test_status_t
cairo_test_for_target (cairo_test_context_t		 *ctx,
		       const cairo_boilerplate_target_t	 *target,
		       int				  dev_offset,
		       cairo_bool_t                       similar)
{
    cairo_test_status_t status;
    cairo_surface_t *surface = NULL;
    cairo_t *cr;
    const char *empty_str = "";
    char *offset_str, *thread_str;
    char *base_name, *base_path;
    char *out_png_path;
    char *ref_path = NULL, *ref_png_path, *cmp_png_path = NULL;
    char *new_path = NULL, *new_png_path;
    char *xfail_path = NULL, *xfail_png_path;
    char *base_ref_png_path;
    char *base_new_png_path;
    char *base_xfail_png_path;
    char *diff_png_path;
    char *test_filename = NULL, *pass_filename = NULL, *fail_filename = NULL;
    cairo_test_status_t ret;
    cairo_content_t expected_content;
    cairo_font_options_t *font_options;
    const char *format;
    cairo_bool_t have_output = FALSE;
    cairo_bool_t have_result = FALSE;
    void *closure;
    double width, height;
    cairo_bool_t have_output_dir;
#if HAVE_MEMFAULT
    int malloc_failure_iterations = ctx->malloc_failure;
    int last_fault_count = 0;
#endif

    /* Get the strings ready that we'll need. */
    format = cairo_boilerplate_content_name (target->content);
    if (dev_offset)
	xasprintf (&offset_str, ".%d", dev_offset);
    else
	offset_str = (char *) empty_str;
    if (ctx->thread)
	xasprintf (&thread_str, ".thread%d", ctx->thread);
    else
	thread_str = (char *) empty_str;

    xasprintf (&base_name, "%s.%s.%s%s%s%s",
	       ctx->test_name,
	       target->name,
	       format,
	       similar ? ".similar" : "",
	       offset_str,
	       thread_str);

    if (offset_str != empty_str)
      free (offset_str);
    if (thread_str != empty_str)
      free (thread_str);


    ref_png_path = cairo_test_reference_filename (ctx,
						  base_name,
						  ctx->test_name,
						  target->name,
						  target->basename,
						  format,
						  CAIRO_TEST_REF_SUFFIX,
						  CAIRO_TEST_PNG_EXTENSION);
    new_png_path = cairo_test_reference_filename (ctx,
						  base_name,
						  ctx->test_name,
						  target->name,
						  target->basename,
						  format,
						  CAIRO_TEST_NEW_SUFFIX,
						  CAIRO_TEST_PNG_EXTENSION);
    xfail_png_path = cairo_test_reference_filename (ctx,
						    base_name,
						    ctx->test_name,
						    target->name,
						    target->basename,
						    format,
						    CAIRO_TEST_XFAIL_SUFFIX,
						    CAIRO_TEST_PNG_EXTENSION);

    base_ref_png_path = cairo_test_reference_filename (ctx,
						  base_name,
						  ctx->test_name,
						  NULL, NULL,
						  format,
						  CAIRO_TEST_REF_SUFFIX,
						  CAIRO_TEST_PNG_EXTENSION);
    base_new_png_path = cairo_test_reference_filename (ctx,
						  base_name,
						  ctx->test_name,
						  NULL, NULL,
						  format,
						  CAIRO_TEST_NEW_SUFFIX,
						  CAIRO_TEST_PNG_EXTENSION);
    base_xfail_png_path = cairo_test_reference_filename (ctx,
						    base_name,
						    ctx->test_name,
						    NULL, NULL,
						    format,
						    CAIRO_TEST_XFAIL_SUFFIX,
						    CAIRO_TEST_PNG_EXTENSION);

    if (target->file_extension != NULL) {
	ref_path = cairo_test_reference_filename (ctx,
						  base_name,
						  ctx->test_name,
						  target->name,
						  target->basename,
						  format,
						  CAIRO_TEST_REF_SUFFIX,
						  target->file_extension);
	new_path = cairo_test_reference_filename (ctx,
						  base_name,
						  ctx->test_name,
						  target->name,
						  target->basename,
						  format,
						  CAIRO_TEST_NEW_SUFFIX,
						  target->file_extension);
	xfail_path = cairo_test_reference_filename (ctx,
						    base_name,
						    ctx->test_name,
						    target->name,
						    target->basename,
						    format,
						    CAIRO_TEST_XFAIL_SUFFIX,
						    target->file_extension);
    }

    have_output_dir = _cairo_test_mkdir (CAIRO_TEST_OUTPUT_DIR);
    xasprintf (&base_path, "%s/%s",
	       have_output_dir ? CAIRO_TEST_OUTPUT_DIR : ".",
	       base_name);
    xasprintf (&out_png_path, "%s" CAIRO_TEST_OUT_PNG, base_path);
    xasprintf (&diff_png_path, "%s" CAIRO_TEST_DIFF_PNG, base_path);

    if (ctx->test->requirements != NULL) {
	const char *required;

	required = target->is_vector ? "target=raster" : "target=vector";
	if (strstr (ctx->test->requirements, required) != NULL) {
	    cairo_test_log (ctx, "Error: Skipping for %s target %s\n",
			    target->is_vector ? "vector" : "raster",
			    target->name);
	    ret = CAIRO_TEST_UNTESTED;
	    goto UNWIND_STRINGS;
	}

	required = target->is_recording ? "target=!recording" : "target=recording";
	if (strstr (ctx->test->requirements, required) != NULL) {
	    cairo_test_log (ctx, "Error: Skipping for %s target %s\n",
			    target->is_recording ? "recording" : "non-recording",
			    target->name);
	    ret = CAIRO_TEST_UNTESTED;
	    goto UNWIND_STRINGS;
	}
    }

    width = ctx->test->width;
    height = ctx->test->height;
    if (width && height) {
	width += dev_offset;
	height += dev_offset;
    }

#if HAVE_MEMFAULT
REPEAT:
    MEMFAULT_CLEAR_FAULTS ();
    MEMFAULT_RESET_LEAKS ();
    ctx->last_fault_count = 0;
    last_fault_count = MEMFAULT_COUNT_FAULTS ();

    /* Pre-initialise fontconfig so that the configuration is loaded without
     * malloc failures (our primary goal is to test cairo fault tolerance).
     */
#if HAVE_FCINIT
    FcInit ();
#endif

    MEMFAULT_ENABLE_FAULTS ();
#endif
    have_output = FALSE;
    have_result = FALSE;

    /* Run the actual drawing code. */
    ret = CAIRO_TEST_SUCCESS;
    surface = (target->create_surface) (base_path,
					target->content,
					width, height,
					ctx->test->width + 25 * NUM_DEVICE_OFFSETS,
					ctx->test->height + 25 * NUM_DEVICE_OFFSETS,
					CAIRO_BOILERPLATE_MODE_TEST,
					ctx->thread,
					&closure);
    if (surface == NULL) {
	cairo_test_log (ctx, "Error: Failed to set %s target\n", target->name);
	ret = CAIRO_TEST_UNTESTED;
	goto UNWIND_STRINGS;
    }

#if HAVE_MEMFAULT
    if (ctx->malloc_failure &&
	MEMFAULT_COUNT_FAULTS () - last_fault_count > 0 &&
	cairo_surface_status (surface) == CAIRO_STATUS_NO_MEMORY)
    {
	goto REPEAT;
    }
#endif

    if (cairo_surface_status (surface)) {
	MF (MEMFAULT_PRINT_FAULTS ());
	cairo_test_log (ctx, "Error: Created an error surface: %s\n",
			cairo_status_to_string (cairo_surface_status (surface)));
	ret = CAIRO_TEST_FAILURE;
	goto UNWIND_STRINGS;
    }

    /* Check that we created a surface of the expected type. */
    if (cairo_surface_get_type (surface) != target->expected_type) {
	MF (MEMFAULT_PRINT_FAULTS ());
	cairo_test_log (ctx, "Error: Created surface is of type %d (expected %d)\n",
			cairo_surface_get_type (surface), target->expected_type);
	ret = CAIRO_TEST_UNTESTED;
	goto UNWIND_SURFACE;
    }

    /* Check that we created a surface of the expected content,
     * (ignore the artificial CAIRO_TEST_CONTENT_COLOR_ALPHA_FLATTENED value).
     */
    expected_content = cairo_boilerplate_content (target->content);

    if (cairo_surface_get_content (surface) != expected_content) {
	MF (MEMFAULT_PRINT_FAULTS ());
	cairo_test_log (ctx, "Error: Created surface has content %d (expected %d)\n",
			cairo_surface_get_content (surface), expected_content);
	ret = CAIRO_TEST_FAILURE;
	goto UNWIND_SURFACE;
    }

    if (cairo_surface_set_user_data (surface,
				     &cairo_boilerplate_output_basename_key,
				     base_path,
				     NULL))
    {
#if HAVE_MEMFAULT
	cairo_surface_destroy (surface);

	if (target->cleanup)
	    target->cleanup (closure);

	goto REPEAT;
#else
	ret = CAIRO_TEST_FAILURE;
	goto UNWIND_SURFACE;
#endif
    }

    cairo_surface_set_device_offset (surface, dev_offset, dev_offset);

    cr = cairo_create (surface);
    if (cairo_set_user_data (cr, &_cairo_test_context_key, (void*) ctx, NULL)) {
#if HAVE_MEMFAULT
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	if (target->cleanup)
	    target->cleanup (closure);

	goto REPEAT;
#else
	ret = CAIRO_TEST_FAILURE;
	goto UNWIND_CAIRO;
#endif
    }

    if (similar)
	cairo_push_group_with_content (cr, expected_content);

    /* Clear to transparent (or black) depending on whether the target
     * surface supports alpha. */
    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);

    /* Set all components of font_options to avoid backend differences
     * and reduce number of needed reference images. */
    font_options = cairo_font_options_create ();
    cairo_font_options_set_hint_style (font_options, CAIRO_HINT_STYLE_NONE);
    cairo_font_options_set_hint_metrics (font_options, CAIRO_HINT_METRICS_ON);
    cairo_font_options_set_antialias (font_options, CAIRO_ANTIALIAS_GRAY);
    cairo_set_font_options (cr, font_options);
    cairo_font_options_destroy (font_options);

    cairo_save (cr);
    alarm (ctx->timeout);
    status = (ctx->test->draw) (cr, ctx->test->width, ctx->test->height);
    alarm (0);
    cairo_restore (cr);

    if (similar) {
	cairo_pop_group_to_source (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);
    }

#if HAVE_MEMFAULT
    MEMFAULT_DISABLE_FAULTS ();

    /* repeat test after malloc failure injection */
    if (ctx->malloc_failure &&
	MEMFAULT_COUNT_FAULTS () - last_fault_count > 0 &&
	(status == CAIRO_TEST_NO_MEMORY ||
	 cairo_status (cr) == CAIRO_STATUS_NO_MEMORY ||
	 cairo_surface_status (surface) == CAIRO_STATUS_NO_MEMORY))
    {
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	if (target->cleanup)
	    target->cleanup (closure);
	if (ctx->thread == 0) {
	    cairo_debug_reset_static_data ();
#if HAVE_FCFINI
	    FcFini ();
#endif
	    if (MEMFAULT_COUNT_LEAKS () > 0) {
		MEMFAULT_PRINT_FAULTS ();
		MEMFAULT_PRINT_LEAKS ();
	    }
	}

	goto REPEAT;
    }
#endif

    /* Then, check all the different ways it could fail. */
    if (status) {
	cairo_test_log (ctx, "Error: Function under test failed\n");
	ret = status;
	goto UNWIND_CAIRO;
    }

#if HAVE_MEMFAULT
    if (MEMFAULT_COUNT_FAULTS () - last_fault_count > 0 &&
	MEMFAULT_HAS_FAULTS ())
    {
	VALGRIND_PRINTF ("Unreported memfaults...");
	MEMFAULT_PRINT_FAULTS ();
    }
#endif

    if (target->finish_surface != NULL) {
#if HAVE_MEMFAULT
	/* We need to re-enable faults as most recording-surface processing
	 * is done during cairo_surface_finish().
	 */
	MEMFAULT_CLEAR_FAULTS ();
	last_fault_count = MEMFAULT_COUNT_FAULTS ();
	MEMFAULT_ENABLE_FAULTS ();
#endif

	/* also check for infinite loops whilst replaying */
	alarm (ctx->timeout);
	status = target->finish_surface (surface);
	alarm (0);

#if HAVE_MEMFAULT
	MEMFAULT_DISABLE_FAULTS ();

	if (ctx->malloc_failure &&
	    MEMFAULT_COUNT_FAULTS () - last_fault_count > 0 &&
	    status == CAIRO_STATUS_NO_MEMORY)
	{
	    cairo_destroy (cr);
	    cairo_surface_destroy (surface);
	    if (target->cleanup)
		target->cleanup (closure);
	    if (ctx->thread == 0) {
		cairo_debug_reset_static_data ();
#if HAVE_FCFINI
		FcFini ();
#endif
		if (MEMFAULT_COUNT_LEAKS () > 0) {
		    MEMFAULT_PRINT_FAULTS ();
		    MEMFAULT_PRINT_LEAKS ();
		}
	    }

	    goto REPEAT;
	}
#endif
	if (status) {
	    cairo_test_log (ctx, "Error: Failed to finish surface: %s\n",
			    cairo_status_to_string (status));
	    ret = CAIRO_TEST_FAILURE;
	    goto UNWIND_CAIRO;
	}
    }

    /* Skip image check for tests with no image (width,height == 0,0) */
    if (ctx->test->width != 0 && ctx->test->height != 0) {
	cairo_surface_t *ref_image;
	cairo_surface_t *test_image;
	cairo_surface_t *diff_image;
	buffer_diff_result_t result;
	cairo_status_t diff_status;

	if (ref_png_path == NULL) {
	    cairo_test_log (ctx, "Error: Cannot find reference image for %s\n",
			    base_name);

	    /* we may be running this test to generate reference images */
	    _xunlink (ctx, out_png_path);
	    /* be more generous as we may need to use external renderers */
	    alarm (4 * ctx->timeout);
	    test_image = target->get_image_surface (surface, 0,
		                                    ctx->test->width,
						    ctx->test->height);
	    alarm (0);
	    diff_status = cairo_surface_write_to_png (test_image, out_png_path);
	    cairo_surface_destroy (test_image);
	    if (diff_status) {
		if (cairo_surface_status (test_image) == CAIRO_STATUS_INVALID_STATUS)
		    ret = CAIRO_TEST_CRASHED;
		else
		    ret = CAIRO_TEST_FAILURE;
		cairo_test_log (ctx,
			        "Error: Failed to write output image: %s\n",
			        cairo_status_to_string (diff_status));
	    }
	    have_output = TRUE;

	    ret = CAIRO_TEST_XFAILURE;
	    goto UNWIND_CAIRO;
	}

	if (target->file_extension != NULL) { /* compare vector surfaces */
	    char *filenames[] = {
		ref_png_path,
		ref_path,
		new_png_path,
		new_path,
		xfail_png_path,
		xfail_path,
		base_ref_png_path,
		base_new_png_path,
		base_xfail_png_path,
	    };

	    xasprintf (&test_filename, "%s.out%s",
		       base_path, target->file_extension);
	    xasprintf (&pass_filename, "%s.pass%s",
		       base_path, target->file_extension);
	    xasprintf (&fail_filename, "%s.fail%s",
		       base_path, target->file_extension);

	    if (cairo_test_file_is_older (pass_filename,
					  filenames,
					  ARRAY_SIZE (filenames)))
	    {
		_xunlink (ctx, pass_filename);
	    }
	    if (cairo_test_file_is_older (fail_filename,
					  filenames,
					  ARRAY_SIZE (filenames)))
	    {
		_xunlink (ctx, fail_filename);
	    }

	    if (cairo_test_files_equal (out_png_path, ref_path)) {
		cairo_test_log (ctx, "Vector surface matches reference.\n");
		have_output = FALSE;
		ret = CAIRO_TEST_SUCCESS;
		goto UNWIND_CAIRO;
	    }
	    if (cairo_test_files_equal (out_png_path, new_path)) {
		cairo_test_log (ctx, "Vector surface matches current failure.\n");
		have_output = FALSE;
		ret = CAIRO_TEST_NEW;
		goto UNWIND_CAIRO;
	    }
	    if (cairo_test_files_equal (out_png_path, xfail_path)) {
		cairo_test_log (ctx, "Vector surface matches known failure.\n");
		have_output = FALSE;
		ret = CAIRO_TEST_XFAILURE;
		goto UNWIND_CAIRO;
	    }

	    if (cairo_test_files_equal (test_filename, pass_filename)) {
		/* identical output as last known PASS */
		cairo_test_log (ctx, "Vector surface matches last pass.\n");
		have_output = TRUE;
		ret = CAIRO_TEST_SUCCESS;
		goto UNWIND_CAIRO;
	    }
	    if (cairo_test_files_equal (test_filename, fail_filename)) {
		/* identical output as last known FAIL, fail */
		cairo_test_log (ctx, "Vector surface matches last fail.\n");
		have_result = TRUE; /* presume these were kept around as well */
		have_output = TRUE;
		ret = CAIRO_TEST_FAILURE;
		goto UNWIND_CAIRO;
	    }
	}

	/* be more generous as we may need to use external renderers */
	alarm (4 * ctx->timeout);
	test_image = target->get_image_surface (surface, 0,
					        ctx->test->width,
						ctx->test->height);
	alarm (0);
	if (cairo_surface_status (test_image)) {
	    cairo_test_log (ctx, "Error: Failed to extract image: %s\n",
			    cairo_status_to_string (cairo_surface_status (test_image)));
	    if (cairo_surface_status (test_image) == CAIRO_STATUS_INVALID_STATUS)
		ret = CAIRO_TEST_CRASHED;
	    else
		ret = CAIRO_TEST_FAILURE;
	    cairo_surface_destroy (test_image);
	    goto UNWIND_CAIRO;
	}

	_xunlink (ctx, out_png_path);
	diff_status = cairo_surface_write_to_png (test_image, out_png_path);
	if (diff_status) {
	    cairo_test_log (ctx, "Error: Failed to write output image: %s\n",
			    cairo_status_to_string (diff_status));
	    cairo_surface_destroy (test_image);
	    ret = CAIRO_TEST_FAILURE;
	    goto UNWIND_CAIRO;
	}
	have_output = TRUE;

	/* binary compare png files (no decompression) */
	if (target->file_extension == NULL) {
	    char *filenames[] = {
		ref_png_path,
		new_png_path,
		xfail_png_path,
		base_ref_png_path,
		base_new_png_path,
		base_xfail_png_path,
	    };

	    xasprintf (&test_filename, "%s", out_png_path);
	    xasprintf (&pass_filename, "%s.pass.png", base_path);
	    xasprintf (&fail_filename, "%s.fail.png", base_path);

	    if (cairo_test_file_is_older (pass_filename,
					  filenames,
					  ARRAY_SIZE (filenames)))
	    {
		_xunlink (ctx, pass_filename);
	    }
	    if (cairo_test_file_is_older (fail_filename,
					  filenames,
					  ARRAY_SIZE (filenames)))
	    {
		_xunlink (ctx, fail_filename);
	    }

	    if (cairo_test_files_equal (test_filename, pass_filename)) {
		cairo_test_log (ctx, "PNG file exactly matches last pass.\n");
                have_result = TRUE;
		cairo_surface_destroy (test_image);
		ret = CAIRO_TEST_SUCCESS;
		goto UNWIND_CAIRO;
	    }
	    if (cairo_test_files_equal (out_png_path, ref_png_path)) {
		cairo_test_log (ctx, "PNG file exactly matches reference image.\n");
                have_result = TRUE;
		cairo_surface_destroy (test_image);
		ret = CAIRO_TEST_SUCCESS;
		goto UNWIND_CAIRO;
	    }
	    if (cairo_test_files_equal (out_png_path, new_png_path)) {
		cairo_test_log (ctx, "PNG file exactly matches current failure image.\n");
                have_result = TRUE;
		cairo_surface_destroy (test_image);
		ret = CAIRO_TEST_NEW;
		goto UNWIND_CAIRO;
	    }
	    if (cairo_test_files_equal (out_png_path, xfail_png_path)) {
		cairo_test_log (ctx, "PNG file exactly matches known failure image.\n");
                have_result = TRUE;
		cairo_surface_destroy (test_image);
		ret = CAIRO_TEST_XFAILURE;
		goto UNWIND_CAIRO;
	    }
	    if (cairo_test_files_equal (test_filename, fail_filename)) {
		cairo_test_log (ctx, "PNG file exactly matches last fail.\n");
		have_result = TRUE; /* presume these were kept around as well */
		cairo_surface_destroy (test_image);
		ret = CAIRO_TEST_FAILURE;
		goto UNWIND_CAIRO;
	    }
	} else {
	    if (cairo_test_files_equal (out_png_path, ref_png_path)) {
		cairo_test_log (ctx, "PNG file exactly matches reference image.\n");
		have_result = TRUE;
		cairo_surface_destroy (test_image);
		ret = CAIRO_TEST_SUCCESS;
		goto UNWIND_CAIRO;
	    }
	    if (cairo_test_files_equal (out_png_path, new_png_path)) {
		cairo_test_log (ctx, "PNG file exactly matches current failure image.\n");
		have_result = TRUE;
		cairo_surface_destroy (test_image);
		ret = CAIRO_TEST_NEW;
		goto UNWIND_CAIRO;
	    }
	    if (cairo_test_files_equal (out_png_path, xfail_png_path)) {
		cairo_test_log (ctx, "PNG file exactly matches known failure image.\n");
		have_result = TRUE;
		cairo_surface_destroy (test_image);
		ret = CAIRO_TEST_XFAILURE;
		goto UNWIND_CAIRO;
	    }
	}

	if (cairo_test_files_equal (out_png_path, base_ref_png_path)) {
	    cairo_test_log (ctx, "PNG file exactly reference image.\n");
	    have_result = TRUE;
	    cairo_surface_destroy (test_image);
	    ret = CAIRO_TEST_SUCCESS;
	    goto UNWIND_CAIRO;
	}
	if (cairo_test_files_equal (out_png_path, base_new_png_path)) {
	    cairo_test_log (ctx, "PNG file exactly current failure image.\n");
	    have_result = TRUE;
	    cairo_surface_destroy (test_image);
	    ret = CAIRO_TEST_NEW;
	    goto UNWIND_CAIRO;
	}
	if (cairo_test_files_equal (out_png_path, base_xfail_png_path)) {
	    cairo_test_log (ctx, "PNG file exactly known failure image.\n");
	    have_result = TRUE;
	    cairo_surface_destroy (test_image);
	    ret = CAIRO_TEST_XFAILURE;
	    goto UNWIND_CAIRO;
	}

	/* first compare against the ideal reference */
	ref_image = cairo_test_get_reference_image (ctx, base_ref_png_path,
						    target->content == CAIRO_TEST_CONTENT_COLOR_ALPHA_FLATTENED);
	if (cairo_surface_status (ref_image)) {
	    cairo_test_log (ctx, "Error: Cannot open reference image for %s: %s\n",
			    base_ref_png_path,
			    cairo_status_to_string (cairo_surface_status (ref_image)));
	    cairo_surface_destroy (test_image);
	    ret = CAIRO_TEST_FAILURE;
	    goto UNWIND_CAIRO;
	}

	diff_image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						 ctx->test->width,
						 ctx->test->height);

	cmp_png_path = base_ref_png_path;
	diff_status = image_diff (ctx,
				  test_image, ref_image, diff_image,
				  &result);
	_xunlink (ctx, diff_png_path);
	if (diff_status ||
            image_diff_is_failure (&result, target->error_tolerance))
	{
	    /* that failed, so check against the specific backend */
	    ref_image = cairo_test_get_reference_image (ctx, ref_png_path,
							target->content == CAIRO_TEST_CONTENT_COLOR_ALPHA_FLATTENED);
	    if (cairo_surface_status (ref_image)) {
		cairo_test_log (ctx, "Error: Cannot open reference image for %s: %s\n",
				ref_png_path,
				cairo_status_to_string (cairo_surface_status (ref_image)));
		cairo_surface_destroy (test_image);
		ret = CAIRO_TEST_FAILURE;
		goto UNWIND_CAIRO;
	    }

	    cmp_png_path = ref_png_path;
	    diff_status = image_diff (ctx,
				      test_image, ref_image,
				      diff_image,
				      &result);
	    if (diff_status)
	    {
		cairo_test_log (ctx, "Error: Failed to compare images: %s\n",
				cairo_status_to_string (diff_status));
		ret = CAIRO_TEST_FAILURE;
	    }
	    else if (image_diff_is_failure (&result, target->error_tolerance))
	    {
		ret = CAIRO_TEST_FAILURE;

		diff_status = cairo_surface_write_to_png (diff_image,
							  diff_png_path);
		if (diff_status) {
		    cairo_test_log (ctx, "Error: Failed to write differences image: %s\n",
				    cairo_status_to_string (diff_status));
		} else {
		    have_result = TRUE;
		}

		cairo_test_copy_file (test_filename, fail_filename);
	    }
	    else
	    { /* success */
		cairo_test_copy_file (test_filename, pass_filename);
	    }
	}
	else
	{ /* success */
	    cairo_test_copy_file (test_filename, pass_filename);
	}

	/* If failed, compare against the current image output,
	 * and attempt to detect systematic failures.
	 */
	if (ret == CAIRO_TEST_FAILURE) {
	    char *image_out_path;

	    image_out_path =
		cairo_test_reference_filename (ctx,
					       base_name,
					       ctx->test_name,
					       "image",
					       "image",
					       format,
					       CAIRO_TEST_OUT_SUFFIX,
					       CAIRO_TEST_PNG_EXTENSION);
	    if (image_out_path != NULL) {
		if (cairo_test_files_equal (out_png_path,
					    image_out_path))
		{
		    ret = CAIRO_TEST_XFAILURE;
		}
		else
		{
		    ref_image =
			cairo_image_surface_create_from_png (image_out_path);
		    if (cairo_surface_status (ref_image) == CAIRO_STATUS_SUCCESS)
		    {
			diff_status = image_diff (ctx,
						  test_image, ref_image,
						  diff_image,
						  &result);
			if (diff_status == CAIRO_STATUS_SUCCESS &&
			    !image_diff_is_failure (&result, target->error_tolerance))
			{
			    ret = CAIRO_TEST_XFAILURE;
			}

			cairo_surface_destroy (ref_image);
		    }
		}

		free (image_out_path);
	    }
	}

	cairo_surface_destroy (test_image);
	cairo_surface_destroy (diff_image);
    }

    if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
	cairo_test_log (ctx, "Error: Function under test left cairo status in an error state: %s\n",
			cairo_status_to_string (cairo_status (cr)));
	ret = CAIRO_TEST_ERROR;
	goto UNWIND_CAIRO;
    }

UNWIND_CAIRO:
    if (test_filename != NULL) {
	free (test_filename);
	test_filename = NULL;
    }
    if (fail_filename != NULL) {
	free (fail_filename);
	fail_filename = NULL;
    }
    if (pass_filename != NULL) {
	free (pass_filename);
	pass_filename = NULL;
    }

#if HAVE_MEMFAULT
    if (ret == CAIRO_TEST_FAILURE)
	MEMFAULT_PRINT_FAULTS ();
#endif
    cairo_destroy (cr);
UNWIND_SURFACE:
    cairo_surface_destroy (surface);

    if (target->cleanup)
	target->cleanup (closure);

#if HAVE_MEMFAULT
    if (ctx->thread == 0) {
	cairo_debug_reset_static_data ();

#if HAVE_FCFINI
	FcFini ();
#endif

	if (MEMFAULT_COUNT_LEAKS () > 0) {
	    if (ret != CAIRO_TEST_FAILURE)
		MEMFAULT_PRINT_FAULTS ();
	    MEMFAULT_PRINT_LEAKS ();
	}
    }

    if (ret == CAIRO_TEST_SUCCESS && --malloc_failure_iterations > 0)
	goto REPEAT;
#endif

    if (ctx->thread == 0) {
	if (have_output)
	    cairo_test_log (ctx, "OUTPUT: %s\n", out_png_path);

	if (have_result) {
	    if (cmp_png_path == NULL) {
		/* XXX presume we matched the normal ref last time */
		cmp_png_path = ref_png_path;
	    }
	    cairo_test_log (ctx,
		            "REFERENCE: %s\nDIFFERENCE: %s\n",
			    cmp_png_path, diff_png_path);
	}
    }

UNWIND_STRINGS:
    if (out_png_path)
      free (out_png_path);
    if (ref_png_path)
      free (ref_png_path);
    if (base_ref_png_path)
      free (base_ref_png_path);
    if (ref_path)
      free (ref_path);
    if (new_png_path)
      free (new_png_path);
    if (base_new_png_path)
      free (base_new_png_path);
    if (new_path)
      free (new_path);
    if (xfail_png_path)
      free (xfail_png_path);
    if (base_xfail_png_path)
      free (base_xfail_png_path);
    if (xfail_path)
      free (xfail_path);
    if (diff_png_path)
      free (diff_png_path);
    if (base_path)
      free (base_path);
    if (base_name)
      free (base_name);

    return ret;
}

#if defined(HAVE_SIGNAL_H) && defined(HAVE_SETJMP_H)
#include <signal.h>
#include <setjmp.h>
/* Used to catch crashes in a test, so that we report it as such and
 * continue testing, although one crasher may already have corrupted memory in
 * an nonrecoverable fashion. */
static jmp_buf jmpbuf;

static void
segfault_handler (int signal)
{
    longjmp (jmpbuf, signal);
}
#endif

cairo_test_status_t
_cairo_test_context_run_for_target (cairo_test_context_t *ctx,
				    const cairo_boilerplate_target_t *target,
				    cairo_bool_t similar,
				    int dev_offset)
{
    cairo_test_status_t status;

    if (target->get_image_surface == NULL)
	return CAIRO_TEST_UNTESTED;

    if (similar && ! cairo_test_target_has_similar (ctx, target))
	return CAIRO_TEST_UNTESTED;

    cairo_test_log (ctx,
		    "Testing %s with %s%s target (dev offset %d)\n",
		    ctx->test_name,
		    similar ? " (similar) " : "",
		    target->name,
		    dev_offset);

    if (ctx->thread == 0) {
	printf ("%s.%s.%s [%d]%s:\t", ctx->test_name, target->name,
		cairo_boilerplate_content_name (target->content),
		dev_offset,
		similar ? " (similar)": "");
	fflush (stdout);
    }

#if defined(HAVE_SIGNAL_H) && defined(HAVE_SETJMP_H)
    if (ctx->thread == 0 && ! RUNNING_ON_VALGRIND) {
	void (* volatile old_segfault_handler)(int);
	void (* volatile old_segfpe_handler)(int);
	void (* volatile old_sigpipe_handler)(int);
	void (* volatile old_sigabrt_handler)(int);
	void (* volatile old_sigalrm_handler)(int);

	/* Set up a checkpoint to get back to in case of segfaults. */
#ifdef SIGSEGV
	old_segfault_handler = signal (SIGSEGV, segfault_handler);
#endif
#ifdef SIGFPE
	old_segfpe_handler = signal (SIGFPE, segfault_handler);
#endif
#ifdef SIGPIPE
	old_sigpipe_handler = signal (SIGPIPE, segfault_handler);
#endif
#ifdef SIGABRT
	old_sigabrt_handler = signal (SIGABRT, segfault_handler);
#endif
#ifdef SIGALRM
	old_sigalrm_handler = signal (SIGALRM, segfault_handler);
#endif
	if (0 == setjmp (jmpbuf))
	    status = cairo_test_for_target (ctx, target, dev_offset, similar);
	else
	    status = CAIRO_TEST_CRASHED;
#ifdef SIGSEGV
	signal (SIGSEGV, old_segfault_handler);
#endif
#ifdef SIGFPE
	signal (SIGFPE, old_segfpe_handler);
#endif
#ifdef SIGPIPE
	signal (SIGPIPE, old_sigpipe_handler);
#endif
#ifdef SIGABRT
	signal (SIGABRT, old_sigabrt_handler);
#endif
#ifdef SIGALRM
	signal (SIGALRM, old_sigalrm_handler);
#endif
    } else {
	status = cairo_test_for_target (ctx, target, dev_offset, similar);
    }
#else
    status = cairo_test_for_target (ctx, target, dev_offset, similar);
#endif

    if (ctx->thread == 0) {
	cairo_test_log (ctx,
		"TEST: %s TARGET: %s FORMAT: %s OFFSET: %d SIMILAR: %d RESULT: ",
			ctx->test_name, target->name,
			cairo_boilerplate_content_name (target->content),
			dev_offset, similar);
	switch (status) {
	case CAIRO_TEST_SUCCESS:
	    printf ("PASS\n");
	    cairo_test_log (ctx, "PASS\n");
	    break;

	case CAIRO_TEST_UNTESTED:
	    printf ("UNTESTED\n");
	    cairo_test_log (ctx, "UNTESTED\n");
	    break;

	default:
	case CAIRO_TEST_CRASHED:
	    if (print_fail_on_stdout && ctx->thread == 0) {
		printf ("!!!CRASHED!!!\n");
	    } else {
		/* eat the test name */
		printf ("\r");
		fflush (stdout);
	    }
	    cairo_test_log (ctx, "CRASHED\n");
	    fprintf (stderr, "%s.%s.%s [%d]%s:\t%s!!!CRASHED!!!%s\n",
		     ctx->test_name, target->name,
		     cairo_boilerplate_content_name (target->content), dev_offset, similar ? " (similar)" : "",
		     fail_face, normal_face);
	    break;

	case CAIRO_TEST_ERROR:
	    if (print_fail_on_stdout && ctx->thread == 0) {
		printf ("!!!ERROR!!!\n");
	    } else {
		/* eat the test name */
		printf ("\r");
		fflush (stdout);
	    }
	    cairo_test_log (ctx, "ERROR\n");
	    fprintf (stderr, "%s.%s.%s [%d]%s:\t%s!!!ERROR!!!%s\n",
		     ctx->test_name, target->name,
		     cairo_boilerplate_content_name (target->content), dev_offset, similar ? " (similar)" : "",
		     fail_face, normal_face);
	    break;

	case CAIRO_TEST_XFAILURE:
	    if (print_fail_on_stdout && ctx->thread == 0) {
		printf ("XFAIL\n");
	    } else {
		/* eat the test name */
		printf ("\r");
		fflush (stdout);
	    }
	    fprintf (stderr, "%s.%s.%s [%d]%s:\t%sXFAIL%s\n",
		     ctx->test_name, target->name,
		     cairo_boilerplate_content_name (target->content), dev_offset, similar ? " (similar)" : "",
		     xfail_face, normal_face);
	    cairo_test_log (ctx, "XFAIL\n");
	    break;

	case CAIRO_TEST_NEW:
	    if (print_fail_on_stdout && ctx->thread == 0) {
		printf ("NEW\n");
	    } else {
		/* eat the test name */
		printf ("\r");
		fflush (stdout);
	    }
	    fprintf (stderr, "%s.%s.%s [%d]%s:\t%sNEW%s\n",
		     ctx->test_name, target->name,
		     cairo_boilerplate_content_name (target->content), dev_offset, similar ? " (similar)" : "",
		     fail_face, normal_face);
	    cairo_test_log (ctx, "NEW\n");
	    break;

	case CAIRO_TEST_NO_MEMORY:
	case CAIRO_TEST_FAILURE:
	    if (print_fail_on_stdout && ctx->thread == 0) {
		printf ("FAIL\n");
	    } else {
		/* eat the test name */
		printf ("\r");
		fflush (stdout);
	    }
	    fprintf (stderr, "%s.%s.%s [%d]%s:\t%sFAIL%s\n",
		     ctx->test_name, target->name,
		     cairo_boilerplate_content_name (target->content), dev_offset, similar ? " (similar)" : "",
		     fail_face, normal_face);
	    cairo_test_log (ctx, "FAIL\n");
	    break;
	}
	fflush (stdout);
    } else {
#if HAVE_FLOCKFILE && HAVE_FUNLOCKFILE
	flockfile (stdout);
#endif
	printf ("%s.%s.%s %d [%d]:\t",
		ctx->test_name, target->name,
		cairo_boilerplate_content_name (target->content),
		ctx->thread,
		dev_offset);
	switch (status) {
	case CAIRO_TEST_SUCCESS:
	    printf ("PASS\n");
	    break;
	case CAIRO_TEST_UNTESTED:
	    printf ("UNTESTED\n");
	    break;
	default:
	case CAIRO_TEST_CRASHED:
	    printf ("!!!CRASHED!!!\n");
	    break;
	case CAIRO_TEST_ERROR:
	    printf ("!!!ERRORED!!!\n");
	    break;
	case CAIRO_TEST_XFAILURE:
	    printf ("XFAIL\n");
	    break;
	case CAIRO_TEST_NEW:
	    printf ("NEW\n");
	    break;
	case CAIRO_TEST_NO_MEMORY:
	case CAIRO_TEST_FAILURE:
	    printf ("FAIL\n");
	    break;
	}

	fflush (stdout);
#if  HAVE_FLOCKFILE && HAVE_FUNLOCKFILE
	funlockfile (stdout);
#endif
    }

    return status;
}

cairo_test_status_t
_cairo_test_context_run (cairo_test_context_t *ctx)
{
    size_t i, j;
    cairo_test_status_t ret;

    ret = CAIRO_TEST_UNTESTED;
    if (ctx->test->preamble != NULL)
	ret = ctx->test->preamble (ctx);

    if (ctx->test->draw == NULL)
	return ret;

    /* The intended logic here is that we return overall SUCCESS
     * iff. there is at least one tested backend and that all tested
     * backends return SUCCESS, OR, there's backends were manually
     * limited, and none were tested.
     * In other words:
     *
     *  if      backends limited and no backend tested
     *          -> SUCCESS
     *	else if any backend not SUCCESS
     *		-> FAILURE
     *	else if all backends UNTESTED
     *		-> FAILURE
     *	else    (== some backend SUCCESS)
     *		-> SUCCESS
     *
     * Also, on a crash, run no further tests.
     */
    for (i = 0; i < ctx->num_targets && ret != CAIRO_TEST_CRASHED; i++) {
	const cairo_boilerplate_target_t *target = ctx->targets_to_test[(i + ctx->thread) % ctx->num_targets];

	for (j = 0; j < NUM_DEVICE_OFFSETS; j++) {
	    int dev_offset = ((j + ctx->thread) % NUM_DEVICE_OFFSETS) * 25;
	    int similar, has_similar;

	    has_similar = cairo_test_target_has_similar (ctx, target);
	    for (similar = 0; similar <= has_similar ; similar++) {
		cairo_status_t status;

		status = _cairo_test_context_run_for_target (ctx,
							     target,
							     similar,
							     dev_offset);
		if (ret == CAIRO_TEST_UNTESTED)
		    ret = status;
	    }
	}
    }

    return ret;
}

#if CAIRO_HAS_REAL_PTHREAD
typedef struct _cairo_test_thread {
    pthread_t thread;
    cairo_test_context_t *ctx;
    size_t id;
} cairo_test_thread_t;

static void *
cairo_test_run_threaded (void *closure)
{
    cairo_test_thread_t *arg = closure;
    cairo_test_context_t ctx;
    cairo_test_status_t ret;

    cairo_test_init_thread (&ctx, arg->ctx, arg->id);

    ret = _cairo_test_context_run (&ctx);

    cairo_test_fini (&ctx);

    return (void *) ret;
}
#endif


static cairo_test_status_t
cairo_test_expecting (const cairo_test_t *test)
{
    cairo_test_context_t ctx;
    cairo_test_status_t ret = CAIRO_TEST_SUCCESS;
    size_t num_threads;

    _cairo_test_init (&ctx, NULL, test, test->name);
    printf ("%s\n", test->description);

#if CAIRO_HAS_REAL_PTHREAD
    num_threads = 0;
    if (getenv ("CAIRO_TEST_NUM_THREADS"))
	num_threads = atoi (getenv ("CAIRO_TEST_NUM_THREADS"));
    if (num_threads > 1) {
	cairo_test_thread_t *threads;
	size_t n;

	threads = xmalloc (sizeof (cairo_test_thread_t) * num_threads);
	for (n = 0; n < num_threads; n++) {
	    threads[n].ctx = &ctx;
	    threads[n].id = n + 1;
	    pthread_create (&threads[n].thread, NULL,
		    cairo_test_run_threaded, &threads[n]);
	}
	for (n = 0; n < num_threads; n++) {
	    void *tmp;
	    pthread_join (threads[n].thread, &tmp);
	    if (ret == CAIRO_TEST_SUCCESS)
		ret = (cairo_test_status_t) tmp;
	}
	free (threads);
    }

    if (ret == CAIRO_TEST_SUCCESS)
#endif
	ret = _cairo_test_context_run (&ctx);

    if (ret != CAIRO_TEST_SUCCESS)
        printf ("Check %s%s out for more information.\n", ctx.test_name, CAIRO_TEST_LOG_SUFFIX);

    cairo_test_fini (&ctx);

    return ret;
}

cairo_test_status_t
cairo_test (const cairo_test_t *test)
{
#ifdef _MSC_VER
    /* We don't want an assert dialog, we want stderr */
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif

    return cairo_test_expecting (test);
}

const cairo_test_context_t *
cairo_test_get_context (cairo_t *cr)
{
    return cairo_get_user_data (cr, &_cairo_test_context_key);
}

cairo_surface_t *
cairo_test_create_surface_from_png (const cairo_test_context_t *ctx,
	                            const char *filename)
{
    cairo_surface_t *image;
    cairo_status_t status;

    image = cairo_image_surface_create_from_png (filename);
    status = cairo_surface_status (image);
    if (status == CAIRO_STATUS_FILE_NOT_FOUND) {
        /* expect not found when running with srcdir != builddir
         * such as when 'make distcheck' is run
         */
	if (ctx->srcdir) {
	    char *srcdir_filename;
	    xasprintf (&srcdir_filename, "%s/%s", ctx->srcdir, filename);
	    cairo_surface_destroy (image);
	    image = cairo_image_surface_create_from_png (srcdir_filename);
	    free (srcdir_filename);
	}
    }

    return image;
}

cairo_pattern_t *
cairo_test_create_pattern_from_png (const cairo_test_context_t *ctx,
	                            const char *filename)
{
    cairo_surface_t *image;
    cairo_pattern_t *pattern;

    image = cairo_test_create_surface_from_png (ctx, filename);

    pattern = cairo_pattern_create_for_surface (image);

    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

    cairo_surface_destroy (image);

    return pattern;
}

static cairo_surface_t *
_draw_check (int width, int height)
{
    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 12, 12);
    cr = cairo_create (surface);
    cairo_surface_destroy (surface);

    cairo_set_source_rgb (cr, 0.75, 0.75, 0.75); /* light gray */
    cairo_paint (cr);

    cairo_set_source_rgb (cr, 0.25, 0.25, 0.25); /* dark gray */
    cairo_rectangle (cr, width / 2,  0, width / 2, height / 2);
    cairo_rectangle (cr, 0, height / 2, width / 2, height / 2);
    cairo_fill (cr);

    surface = cairo_surface_reference (cairo_get_target (cr));
    cairo_destroy (cr);

    return surface;
}

void
cairo_test_paint_checkered (cairo_t *cr)
{
    cairo_surface_t *check;

    check = _draw_check (12, 12);

    cairo_save (cr);
    cairo_set_source_surface (cr, check, 0, 0);
    cairo_surface_destroy (check);

    cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
    cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
    cairo_paint (cr);

    cairo_restore (cr);
}

cairo_bool_t
cairo_test_is_target_enabled (const cairo_test_context_t *ctx,
			      const char *target)
{
    size_t i;

    for (i = 0; i < ctx->num_targets; i++) {
	const cairo_boilerplate_target_t *t = ctx->targets_to_test[i];
	if (strcmp (t->name, target) == 0) {
	    /* XXX ask the target whether is it possible to run?
	     * e.g. the xlib backend could check whether it is able to connect
	     * to the Display.
	     */
	    return t->get_image_surface != NULL;
	}
    }

    return FALSE;
}

cairo_bool_t
cairo_test_malloc_failure (const cairo_test_context_t *ctx,
			   cairo_status_t status)
{
    if (! ctx->malloc_failure)
	return FALSE;

    if (status != CAIRO_STATUS_NO_MEMORY)
	return FALSE;

#if HAVE_MEMFAULT
    {
	int n_faults;

	/* prevent infinite loops... */
	n_faults = MEMFAULT_COUNT_FAULTS ();
	if (n_faults == ctx->last_fault_count)
	    return FALSE;

	((cairo_test_context_t *) ctx)->last_fault_count = n_faults;
    }
#endif

    return TRUE;
}

cairo_test_status_t
cairo_test_status_from_status (const cairo_test_context_t *ctx,
			       cairo_status_t status)
{
    if (status == CAIRO_STATUS_SUCCESS)
	return CAIRO_TEST_SUCCESS;

    if (cairo_test_malloc_failure (ctx, status))
	return CAIRO_TEST_NO_MEMORY;

    return CAIRO_TEST_FAILURE;
}
