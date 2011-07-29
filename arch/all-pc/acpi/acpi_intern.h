APTR core_ACPITableHeaderEarly(int id, struct ACPIBase *ACPIBase);
unsigned char acpi_CheckTable(struct ACPI_TABLE_DEF_HEADER *header, ULONG id);
int acpi_IsBlacklisted(struct ACPIBase *ACPIBase);

/* Result of this must be zero */
static inline unsigned char acpi_CheckSum(void *addr, unsigned int size)
{
    unsigned char *j = addr;
    unsigned char *k = j + size;
    unsigned char sum = 0;

    for (; j < k; sum += *(j++));
    
    return sum;
}
