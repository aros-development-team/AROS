/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS ACPIDump tool.
    Lang: english
*/

#include <resources/acpi.h>
#include <proto/exec.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void dumpData(unsigned char *data, int length)
{
      int len;
      int i, left = length;

      while (left > 0)
      {
            len = printf("%04x:", length - left);
            for (i = 0; i < 16 && i < left; i++) {
                      printf(" %02x", data[i]);
            }
            if(i != 15) for(;i<16;i++) printf("   ");
            printf("  ");
            for(i = 0; i < 16 && i < left; i++)
            {
                printf("%c", isprint(data[i])?data[i]:'.');
            }
            printf("\n");
            data += 16;
            left -= 16;
      }
}

static void dumpTable(struct ACPI_TABLE_DEF_HEADER *table, BOOL verbose)
{
     printf("%4.4s: 0x%p (%u bytes)\n", (char *)&table->signature, table, table->length);
     if (verbose)
         dumpData((unsigned char *)table, table->length);
}

static void parseFADT(struct ACPI_TABLE_TYPE_FADT *fadt, BOOL verbose)
{
    unsigned char sum = 0;
    unsigned char *j, *k;
    
    j = (unsigned char*)fadt;
    k = j + ((struct ACPI_TABLE_DEF_HEADER*)j)->length;
    for (; j < k; sum += *(j++));
    if(!sum)
    {
        if( ((struct ACPI_TABLE_DEF_HEADER*)fadt)->length >= 44 && fadt->dsdt_addr)
        {
            j = (unsigned char*)(IPTR)fadt->dsdt_addr;
            k = j + ((struct ACPI_TABLE_DEF_HEADER *)j)->length;
            for (; j < k; sum += *(j++));
            if(!sum)
            {
                j = (unsigned char*)(IPTR)fadt->dsdt_addr;
                printf("ACPI table %c%c%c%c %08X\n", j[0], j[1], j[2], j[3], fadt->dsdt_addr);
                if (verbose)
                    dumpData(j, ((struct ACPI_TABLE_DEF_HEADER*)j)->length);
            }
            
            /* FACS don't have checksum */
            j = (unsigned char*)(IPTR)fadt->facs_addr;
            printf("ACPI table %c%c%c%c %08X\n", j[0], j[1], j[2], j[3], fadt->facs_addr);
            if (verbose)
            	dumpData(j, ((struct ACPI_TABLE_DEF_HEADER*)j)->length);
        }
    }else
    {
        printf("FADT table have wrong checksum\n");
    }
    

}

int main(int argc, char **argv)
{
    int i;
    struct ACPIBase *ACPIBase = OpenResource("acpi.resource");
    struct ACPI_TABLE_TYPE_FADT *fadt = NULL;
    BOOL dump = 0;

    if (!ACPIBase)
    {
        printf("acpi.resource not found, no ACPI!\n");
        return 0;
    }

    if (argc > 1)
    	dump = !stricmp(argv[1], "--contents");

    printf("Root tables:\n");
    printf("RSD PTR : 0x%p\n", ACPIBase->ACPIB_RSDP_Addr);
    
    if (dump)
    {
    	if (ACPIBase->ACPIB_RSDP_Addr > 0x1000)
	    dumpData((unsigned char *)ACPIBase->ACPIB_RSDP_Addr, (ACPIBase->ACPIB_RSDP_Addr->revision < 2) ? 20 : 36);
	else
	    printf("Protected location, no dump available\n");
    }

    dumpTable(ACPIBase->ACPIB_SDT_Addr, dump);

    printf("Top-level system description tables:\n");
    for (i = 0; i < ACPIBase->ACPIB_SDT_Count; i++)
    {
    	dumpTable(ACPIBase->ACPIB_SDT_Entry[i], dump);

    	if (ACPIBase->ACPIB_SDT_Entry[i]->signature == ACPI_MAKE_ID('F', 'A', 'C', 'P'))
    	    fadt = (struct ACPI_TABLE_TYPE_FADT *)ACPIBase->ACPIB_SDT_Entry[i];
    }

    if (fadt)
    {
    	printf("FADT tables:\n");
    	parseFADT(fadt, dump);
    }

    return 0;
} 
