#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/kernel.h>

#include <stdio.h>

int __nocommandline = 1;

static const char *types[] =
{
    "Unknown  ",
    "Avalable ",
    "Reserved ",
    "ACPI Data",
    "ACPI NVS "
};

int main(void)
{
    struct TagItem *tags;
    struct mb_mmap *mmap;
    IPTR len;
    APTR KernelBase = OpenResource("kernel.resource");

    if (!KernelBase)
    {
	printf("Failed to open kernel.resource!\n");
	return 20;
    }

    tags = KrnGetBootInfo();
    if (!tags)
    {
    	printf("No boot information from the bootstrap!\n");
    	return 20;
    }

    mmap = (struct mb_mmap *)GetTagData(KRN_MMAPAddress, 0, tags);
    len  = GetTagData(KRN_MMAPLength, 0, tags);

    if (!mmap || !len)
    {
        printf("No memory map provided by the bootstrap!\n");
        return 20;
    }

    printf("Memory map at 0x%p, length %lu:\n", mmap, len);
    while (len >= sizeof(struct mb_mmap))
    {
        unsigned int type = mmap->type;
        unsigned long long memaddr = mmap->addr;
        unsigned long long memlen  = mmap->len;

        if (type > MMAP_TYPE_ACPINVS)
            type = 0;

#if __WORDSIZE < 64
	memaddr |= (unsigned long long)mmap->addr_high << 32;
	memlen  |= (unsigned long long)mmap->len_high << 32;
#endif

	printf("Entry size %d type %d <%s> addr 0x%016llx len 0x%016llx\n", mmap->size, mmap->type, types[type], memaddr, memlen);

        len -= mmap->size + 4;
        mmap = (struct mb_mmap *)(mmap->size + (unsigned long)mmap + 4);
    }

    return 0;
}
