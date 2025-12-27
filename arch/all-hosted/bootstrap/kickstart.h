// For kernel_entry_fun_t
#include "elfloader.h"

#ifdef _WIN64
#define __aros __attribute__((sysv_abi))
#else
#define __aros
#endif

int kick(__aros kernel_entry_fun_t addr, struct TagItem *msg);
