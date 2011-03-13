#include <stdio.h>
#include <stdlib.h>
#if defined(__AROS__)
#include <exec/types.h>
typedef BOOL gboolean;
typedef void * GdkPixbuf;
#else
#include <glib.h>
#include <gtk/gtk.h>
#endif
#include "pixman.h"

void show_image (pixman_image_t *image);

GdkPixbuf *pixbuf_from_argb32 (uint32_t *bits,
		               gboolean has_alpha,
                               int width,
                               int height,
                               int stride);
