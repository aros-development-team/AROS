/*
    Copyright (C) 2026 John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL), Version 1.1.
*/
#include <unistd.h>

ssize_t pwrite(int d, const void *buffer, size_t count, off_t offset)
{
    return pwrite64(d, buffer, count, offset);
}
