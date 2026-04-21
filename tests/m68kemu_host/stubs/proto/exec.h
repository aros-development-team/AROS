#pragma once
#include <cstdlib>
#include <cstring>
#define AllocMem(size, flags) calloc(1, size)
#define FreeMem(ptr, size) free(ptr)
#define CopyMem(src, dst, n) memcpy(dst, src, n)
