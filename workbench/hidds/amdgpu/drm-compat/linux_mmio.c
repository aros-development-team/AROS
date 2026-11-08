/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/kernel.h>

/*
 * Platform registers outside the PCI windows are not mapped by default;
 * map them identity, the way the PCI driver maps its own controller.
 */
void *compat_map_mmio(unsigned long pa, unsigned long size)
{
    APTR KernelBase = OpenResource("kernel.resource");

    if (!KernelBase)
        return NULL;
#if defined(__riscv) || defined(__aarch64__) || defined(__arm__)
    if (!KrnMapGlobal((APTR)pa, (APTR)pa, size, MAP_Readable | MAP_Writable))
        return NULL;
#endif
    return (void *)pa;
}

/*
 * Write-combining for the 2 MiB pages wholly inside [va, va + size). An
 * identity mapping covers the whole range already; this only changes the
 * memory type.
 */
int compat_map_write_combining(void *va, unsigned long size)
{
#if defined(__x86_64__)
    const IPTR big = 2UL << 20, chunk = 1UL << 30;
    APTR KernelBase = OpenResource("kernel.resource");
    IPTR start = ((IPTR)va + big - 1) & ~(big - 1);
    IPTR end = ((IPTR)va + size) & ~(big - 1);

    if (!KernelBase)
        return -1;
    while (start < end)
    {
        IPTR len = end - start > chunk ? chunk : end - start;

        if (!KrnMapGlobal((APTR)start, (APTR)start, len,
                         MAP_Readable | MAP_Writable | MAP_WriteCombine))
            return -1;
        start += len;
    }
#endif
    return 0;
}

#include <proto/dos.h>
#include <dos/dos.h>
#include <stdio.h>

/* Diagnostics: drop a buffer into SYS:amdgpu-dump/<name>. */
int compat_dump_file(const char *name, const void *data, unsigned long size)
{
    char path[64];
    BPTR lock, file;
    LONG done = 0;

    lock = Lock("SYS:amdgpu-dump", ACCESS_READ);
    if (!lock)
        lock = CreateDir("SYS:amdgpu-dump");
    if (!lock)
        return -1;
    UnLock(lock);

    snprintf(path, sizeof(path), "SYS:amdgpu-dump/%s", name);
    file = Open(path, MODE_NEWFILE);
    if (!file)
        return -1;
    while ((unsigned long)done < size) {
        LONG n = Write(file, (APTR)((const char *)data + done), size - done);
        if (n <= 0)
            break;
        done += n;
    }
    Close(file);
    return (unsigned long)done == size ? 0 : -1;
}
