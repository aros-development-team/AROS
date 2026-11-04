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

#include <proto/dos.h>
#include <dos/dos.h>
#include <stdio.h>

/* Diagnostics: drop a buffer into SYS:nouveau-dump/<name>. */
int compat_dump_file(const char *name, const void *data, unsigned long size)
{
    char path[64];
    BPTR lock, file;
    LONG done = 0;

    lock = Lock("SYS:nouveau-dump", ACCESS_READ);
    if (!lock)
        lock = CreateDir("SYS:nouveau-dump");
    if (!lock)
        return -1;
    UnLock(lock);

    snprintf(path, sizeof(path), "SYS:nouveau-dump/%s", name);
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
