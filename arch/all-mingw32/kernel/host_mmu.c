#include <windows.h>

#include "host_intern.h"

/*
 * A simple glue code. I'm too lazy to import kernel32.dll via hostlib.resource
 * just for one function.
 */

unsigned int __declspec(dllexport) __aros core_protect(void *addr, unsigned int len, unsigned int prot)
{
    DWORD oldprot;

    return VirtualProtect(addr, len, prot, &oldprot);
}

