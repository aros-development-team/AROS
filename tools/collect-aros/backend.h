#ifndef _BACKEND_H_
#define _BACKEND_H_

#include "gensets.h"

#define AROSOBJ_CXXPUREVIRT		"static-cxx-cxa-pure-virtual.o"
#define AROSLIB_PTHREAD			"libpthread.a"

extern int have_gnunm;

/*
 * Path of a link library or object that collect-aros adds on its own
 * (AROSOBJ_CXXPUREVIRT, AROSLIB_PTHREAD), in a buffer the caller frees.
 * The file is taken from the lib/ directory of the sysroot of this link
 * when it is there, otherwise from the directory collect-aros was built for.
 */
extern char *objlib_path(const char *name);

extern void backend_init(char *);
extern int check_and_print_undefined_symbols(const char *file);
extern int has_undefined_symbols(const char *file);
extern void collect_sets(const char *file, setnode **setlist_ptr);
extern void collect_libs(const char *file, setnode **liblist_ptr);
extern void collect_extra(const char *file, setnode **liblist_ptr);
#endif /* !_BACKEND_H_ */
