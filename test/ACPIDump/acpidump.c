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

static void dumpData(unsigned char * data, int length)
{
      int len;
      int i, left = length;
      while (left > 0) {
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

void parseFADT(struct ACPI_TABLE_TYPE_FADT *fadt)
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
                dumpData(j, ((struct ACPI_TABLE_DEF_HEADER*)j)->length);
            }
            
            /* FACS don't have checksum */
            j = (unsigned char*)(IPTR)fadt->facs_addr;
            printf("ACPI table %c%c%c%c %08X\n", j[0], j[1], j[2], j[3], fadt->facs_addr);
            dumpData(j, ((struct ACPI_TABLE_DEF_HEADER*)j)->length);
        }
    }else
    {
        printf("FADT table have wrong checksum\n");
    }
    

}

void parseRSDP(struct ACPI_TABLE_TYPE_RSDP *rsdp)
{
    struct ACPI2_TABLE_TYPE_RSDP *xsdp;
    struct ACPI_TABLE_TYPE_RSDT *rsdt;
    IPTR xsdt;

    unsigned char sum;
    unsigned char *j, *k;
    unsigned int i;
    int num;
    char *offset;

    xsdp = (struct ACPI2_TABLE_TYPE_RSDP*)rsdp;
    if (xsdp->revision > 1 && xsdp->xsdt_address)
    {
        UQUAD address;

        printf("ACPI is using XSDT table.\n");
        xsdt = (IPTR)xsdp->xsdt_address;

        printf("ACPI table XSDT %08X\n", (unsigned int)xsdt);
        
        sum = 0;

        j = (unsigned char*)xsdt;
        k = j + ((struct ACPI_TABLE_DEF_HEADER*)j)->length;
 
        /* checksuming, sum shall be zero here */
        for (; j < k; sum += *(j++));  
 
        if(!sum)
        {
            j = (unsigned char*)xsdt;
            dumpData(j, ((struct ACPI_TABLE_DEF_HEADER*)j)->length);

            
            /* how many table do we have ? */
            num = (((struct ACPI_TABLE_DEF_HEADER *)xsdt)->length - sizeof(struct ACPI_TABLE_DEF_HEADER)) / sizeof(UQUAD);

            /* loop through all the tables ptr */
            offset = (char*)xsdt + sizeof(struct ACPI_TABLE_DEF_HEADER);
            for(i=0 ; i < num; i++, offset += sizeof(UQUAD))
            {
                address = (UQUAD)(*(UQUAD*)offset);
                if(!address) continue;
                j = (unsigned char*)address;
                k = j + ((struct ACPI_TABLE_DEF_HEADER*)j)->length;
                
                /* checksuming, sum shall be zero here */
                for (; j < k; sum += *(j++));   
                if(!sum)
                {
                    j = (unsigned char*)address;
                    
                    printf("ACPI table %c%c%c%c %08X\n",j[0], j[1], j[2], j[3],(unsigned int)address);
                    dumpData(j, ((struct ACPI_TABLE_DEF_HEADER*)j)->length);
                    if(j[0] == 'F' && j[1] == 'A' && j[2] == 'C' && j[3] == 'P') parseFADT((struct ACPI_TABLE_TYPE_FADT*)address);

                } 
                else
                {
                    
                    /* reset sum for next checksum */
                    sum = 0;    
                    printf("ACPI table have wrong checksum !\n");
                }
            }        
        }
        else
        {
            printf("RSDT wrong checksum!\n");
        }

 
    } /* XSDT END */
    else
    {  
        
        /* ACPI is using RSDT table */
        unsigned long address;
        
        rsdt = (void *)(IPTR)rsdp->rsdt_address;

        printf("ACPI table RSDT 0x%p\n", rsdt);
        
        sum = 0;

        j = (unsigned char*)rsdt;
        k = j + ((struct ACPI_TABLE_DEF_HEADER*)j)->length;
 
        /* checksuming, sum shall be zero here */
        for (; j < k; sum += *(j++));   
 
        if(!sum)
        {
            j = (unsigned char*)rsdt;
            dumpData(j, ((struct ACPI_TABLE_DEF_HEADER*)j)->length);

            /* how many table do we have ? */
            num = (((struct ACPI_TABLE_DEF_HEADER *)rsdt)->length - sizeof(struct ACPI_TABLE_DEF_HEADER)) / sizeof(unsigned int);
 
            /* loop through all the tables ptr*/
            offset = (char*)rsdt + sizeof(struct ACPI_TABLE_DEF_HEADER);
            for(i=0 ; i < num; i++, offset += sizeof(unsigned long))
            {
                address = (unsigned long)(*(unsigned long*)offset);
                if(!address) continue;
                j = (unsigned char*)address;
                k = j + ((struct ACPI_TABLE_DEF_HEADER*)j)->length;
                
                /* checksuming, sum shall be zero here */
                for (; j < k; sum += *(j++));   
                if(!sum)
                {
                    j = (unsigned char*)address;
                    
                    printf("ACPI table %c%c%c%c %08X\n",j[0], j[1], j[2], j[3],(unsigned int)address);
                    dumpData(j, ((struct ACPI_TABLE_DEF_HEADER*)j)->length);
                    if(j[0] == 'F' && j[1] == 'A' && j[2] == 'C' && j[3] == 'P') parseFADT((struct ACPI_TABLE_TYPE_FADT*)address);

                } 
                else
                {
                    
                    /* reset sum for next checksum */
                    sum = 0;    
                    printf("ACPI table have wrong checksum !\n");
                }
            }      
        }
        else
        {
            printf("RSDT wrong checksum!\n");
        }
    }
}

int main(int argc, char **argv)
{
    struct ACPIBase *ACPIBase = OpenResource("acpi.resource");
    
    if (!ACPIBase)
    {
        printf("acpi.resource not found, no ACPI!\n");
        return 0;
    }
    
    printf("RSD PTR : 0x%p\n", ACPIBase->ACPIB_RSDP_Addr);
    parseRSDP(ACPIBase->ACPIB_RSDP_Addr);

    return 0;
} 
