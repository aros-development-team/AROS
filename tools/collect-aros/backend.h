#ifndef _BACKEND_H_
#define _BACKEND_H_

#include "gensets.h"

extern int check_and_print_undefined_symbols(const char *file);
extern void collect_sets(const char *file, setnode **setlist_ptr);

#endif /* !_BACKEND_H_ */
