#ifndef __ZUNE_GC_H__
#define __ZUNE_GC_H__

/*
 * pasted from gtk/gtkgc.h
 */

#include <gdk/gdk.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

GdkGC* zune_gc_get     (gint             depth,
		       GdkColormap     *colormap,
		       GdkGCValues     *values,
		       GdkGCValuesMask  values_mask);

GdkGC *zune_gc_get_with_color (int depth, GdkColormap *cmap,
			       const GdkColor *color);

void   zune_gc_release (GdkGC           *gc);

void   zune_gc_cache_cleanup (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __ZUNE_GC_H__ */
