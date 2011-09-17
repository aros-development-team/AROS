unsigned char acpi_CheckTable(struct ACPI_TABLE_DEF_HEADER *header, ULONG id);
int acpi_IsBlacklisted(struct ACPIBase *ACPIBase);

AROS_LD1(ULONG, ShutdownA,
	 AROS_LHA(ULONG, action, D0),
	 struct ExecBase *, SysBase, 173, Acpi);

/* Result of this must be zero */
static inline unsigned char acpi_CheckSum(void *addr, unsigned int size)
{
    unsigned char *j = addr;
    unsigned char *k = j + size;
    unsigned char sum = 0;

    for (; j < k; sum += *(j++));
    
    return sum;
}
