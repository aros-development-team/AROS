/* FIXME: Add autodoc */

#include <string.h> // Avoid it is being parsed inside memmove.c

#define memmove memcpy
#define restrict
#include "memmove.c"

