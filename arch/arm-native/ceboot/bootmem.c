#include <windows.h>

#include <runtime.h>
#include "bootmem.h"
#include "winapi.h"

/* Size of bootinfo buffer - one page */
#define BOOTMEM_SIZE 4096

static void *bootmem_Virt;
static void *bootmem_Limit;
ULONG_PTR bootmem_Phys;

ULONG_PTR InitBootMem(void)
{
    bootmem_Virt = AllocPhysMem(BOOTMEM_SIZE, PAGE_READWRITE, 0, 0, &bootmem_Phys);

    if (bootmem_Virt)
    {
        bootmem_Limit = bootmem_Virt + BOOTMEM_SIZE;
        /* This routine returns PHYSICAL address */
        return bootmem_Phys;
    }
    else
        return 0;
}

void *AllocBootMem(unsigned int size)
{
    void *ptr = bootmem_Virt;
 
    /* Align size at pointer boundary */
    size = (size + sizeof(void *) - 1) & (~(sizeof(void *) - 1));
    if (bootmem_Limit - ptr < size)
    {
        DisplayError("Not enough memory to build boot information (%d bytes requested)", size);
        exit(0); /* I'm tired to write those if's... */
    }

    bootmem_Virt += size;
    bootmem_Phys += size;
    
    return ptr;
}

void *AddTag(unsigned int tag, ULONG_PTR data)
{
    ULONG_PTR *ptr = AllocBootMem(sizeof(ULONG_PTR) * 2);

    ptr[0] = tag;
    ptr[1] = data;

    return ptr;
}
