#include <io.h>
#include <stdio.h>
#include <windows.h>

#include "sharedmem.h"

/* This is not included in my Mingw32 headers */
#ifndef FILE_MAP_EXECUTE
#define FILE_MAP_EXECUTE 0x0020
#endif

#define D(x)

#define ID_LEN 64

HANDLE RAM_Handle = NULL;
void *RAM_Address = NULL;

void *AllocateRO(size_t len)
{
    return VirtualAlloc(NULL, len, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

/*
 * Commit executable and read-only state for kickstart's .code
 */
int SetRO(void *addr, size_t len)
{
    DWORD old;

    return !VirtualProtect(addr, len, PAGE_EXECUTE_READ, &old);
}

void *AllocateRW(size_t len)
{
    return VirtualAlloc(NULL, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
}

void *AllocateRAM(size_t len)
{
    void *addr = NULL;
    char *var;
    SECURITY_ATTRIBUTES sa;

    var = getenv(SHARED_RAM_VAR);
    if (var)
    {
	D(printf("[AllocateRAM] Found RAM specification: %s\n", var));
	if (sscanf(var, "%p:%p", &RAM_Handle, &addr) != 2) {
	    D(printf("[AllocateRAM] Error parsing specification\n"));
	    RAM_Handle = NULL;
	    addr = NULL;
	}
    }

    D(printf("[AllocateRAM] Inherited memory handle 0x%p address 0x%p\n", RAM_Handle, addr));
    if (!RAM_Handle)
    {
        sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	RAM_Handle = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_EXECUTE_READWRITE, 0, len, NULL);
    }
    if (!RAM_Handle)
    {
	D(printf("[AllocateRAM] PAGE_EXECUTE_READWRITE failed, retrying with PAGE_READWRITE\n"));
	RAM_Handle = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, len, NULL);
    }
    D(printf("[AllocateRAM] Shared memory handle 0x%p\n", RAM_Handle));
    if (!RAM_Handle)
	return NULL;

    RAM_Address = MapViewOfFileEx(RAM_Handle, FILE_MAP_ALL_ACCESS|FILE_MAP_EXECUTE, 0, 0, 0, addr);
    D(printf("[AllocateRAM] Requested address 0x%p, mapped at 0x%p\n", addr, RAM_Address));

    if (!RAM_Address)
    {
	CloseHandle(RAM_Handle);
	RAM_Handle = NULL;
    }

    return RAM_Address;
}
