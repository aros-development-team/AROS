#include <stdio.h>
#include <stdarg.h>
#include <proto/exec.h>
#include "debug.h"
#define DBUG 1
#include <dbug.h>

void kprintf(char *fmt, ...) {
va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

