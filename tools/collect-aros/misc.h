#ifndef _MISC_H_
#define _MISC_H_

#include <stddef.h>

/************ In libiberty.a ************/
extern void *xmalloc(size_t size);

extern char *make_temp_file(const char *suffix);

/****************************************/

extern char *program_name;
extern void nonfatal(const char *msg, const char *errorstr);
extern void fatal(const char *msg, const char *errorstr);
extern void set_compiler_path(void);

#endif /* !_MISC_H */
