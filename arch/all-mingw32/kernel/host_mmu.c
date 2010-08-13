#include <windows.h>

/*
 * A simple glue code. I'm too lazy to import kernel32.dll via hostlib.resource
 * just for one function.
 */

unsigned int __declspec(dllexport) core_protect(void *addr, unsigned int len, unsigned int prot)
{
    DWORD oldprot;

    return VirtualProtect(addr, len, prot, &oldprot);
}
