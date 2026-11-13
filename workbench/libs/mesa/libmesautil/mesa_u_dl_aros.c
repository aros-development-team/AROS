#include "util/u_dl.h"

#include <stddef.h>

struct util_dl_library {
   int unused;
};

struct util_dl_library *
util_dl_open(const char *filename)
{
   (void)filename;
   return NULL;
}

util_dl_proc
util_dl_get_proc_address(struct util_dl_library *library, const char *procname)
{
   (void)library;
   (void)procname;
   return NULL;
}

char *
util_dl_get_path_from_proc(const void *func_proc)
{
   (void)func_proc;
   return NULL;
}

void
util_dl_close(struct util_dl_library *library)
{
   (void)library;
}

const char *
util_dl_error(void)
{
   return "dynamic loading unavailable on this target";
}
