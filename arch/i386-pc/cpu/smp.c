/* AROS Multi Processor Support Functions by NicJA. */
#ifndef _SMP_C
#define _SMP_C

#ifndef _CPU_INTERN_H
#   include "cpu_intern.h"
#endif

/**********************************************************************/
void parse_MP_Processor (struct mpc_config_processor *mc, struct SMP_Definition *SMP_Group )
{
    struct  CPU_Definition          *ThisCPU, *CPUList;
    struct  i386_compat_intern      *ThisCPU_intern;
    int                             cpuapicver, apicid;
    //physid_mask_t           tmp;

    CPUList = CPUBase->CPUB_Processors;

    //apicid = mpc_apic_id(mc, translation_table[SMP_Group->SMP_RecordCount]);

    kprintf(DEBUG_NAME_STR ": Parsing MP Processor Features :-\n");

    if (mc->mpc_featureflag&(1<<0))     kprintf(DEBUG_NAME_STR ":     *Floating point unit present.\n");
    if (mc->mpc_featureflag&(1<<7))     kprintf(DEBUG_NAME_STR ":     *Machine Exception supported.\n");
    if (mc->mpc_featureflag&(1<<8))     kprintf(DEBUG_NAME_STR ":     *64 bit compare & exchange supported.\n");
    if (mc->mpc_featureflag&(1<<9))     kprintf(DEBUG_NAME_STR ":     *Internal APIC present.\n");
    if (mc->mpc_featureflag&(1<<11))	kprintf(DEBUG_NAME_STR ":     *SEP present.\n");
    if (mc->mpc_featureflag&(1<<12))	kprintf(DEBUG_NAME_STR ":     *MTRR present.\n");
    if (mc->mpc_featureflag&(1<<13))	kprintf(DEBUG_NAME_STR ":     *PGE present.\n");
    if (mc->mpc_featureflag&(1<<14))	kprintf(DEBUG_NAME_STR ":     *MCA present.\n");
    if (mc->mpc_featureflag&(1<<15))	kprintf(DEBUG_NAME_STR ":     *CMOV present.\n");
    if (mc->mpc_featureflag&(1<<16))	kprintf(DEBUG_NAME_STR ":     *PAT present.\n");
    if (mc->mpc_featureflag&(1<<17))	kprintf(DEBUG_NAME_STR ":     *PSE present.\n");
    if (mc->mpc_featureflag&(1<<18))	kprintf(DEBUG_NAME_STR ":     *PSN present.\n");
    if (mc->mpc_featureflag&(1<<19))	kprintf(DEBUG_NAME_STR ":     *Cache Line Flush Instruction present.\n");
    /* 20 Reserved */
    if (mc->mpc_featureflag&(1<<21))	kprintf(DEBUG_NAME_STR ":     *Debug Trace and EMON Store present.\n");
    if (mc->mpc_featureflag&(1<<22))	kprintf(DEBUG_NAME_STR ":     *ACPI Thermal Throttle Registers present.\n");
    if (mc->mpc_featureflag&(1<<23))	kprintf(DEBUG_NAME_STR ":     *MMX present.\n");
    if (mc->mpc_featureflag&(1<<24))	kprintf(DEBUG_NAME_STR ":     *FXSR present.\n");
    if (mc->mpc_featureflag&(1<<25))	kprintf(DEBUG_NAME_STR ":     *XMM present.\n");
    if (mc->mpc_featureflag&(1<<26))	kprintf(DEBUG_NAME_STR ":     *Willamette New Instructions present.\n");
    if (mc->mpc_featureflag&(1<<27))	kprintf(DEBUG_NAME_STR ":     *Self Snoop  present.\n");
    if (mc->mpc_featureflag&(1<<28))	kprintf(DEBUG_NAME_STR ":     *HT present.\n");
    if (mc->mpc_featureflag&(1<<29))	kprintf(DEBUG_NAME_STR ":     *Thermal Monitor present.\n");
    /* 30, 31 Reserved */

    /*if (SMP_Group->SMP_CPUCount >= MAX_CPU) 
    {
	    kprintf(DEBUG_NAME_STR ": WARNING - Reached MAX_CPU count [%i]. CPU(apicid 0x%x) not booted.\n", MAX_CPU, mc->mpc_apicid);
	    return;
    }*/

    if (MAX_APICS - mc->mpc_apicid <= 0)
    {
	    kprintf(DEBUG_NAME_STR ": WARNING - INVALID Processor [#%d - Max ID: %d].\n", mc->mpc_apicid, MAX_APICS);
	    return;
    }

    if (mc->mpc_cpuflag & CPU_BOOTPROCESSOR)
    {
	kprintf(DEBUG_NAME_STR ":     Bootup CPU\n");                                                     /* CPU booted system..                  */
	CPUBase->CPUB_BOOT_Physical = mc->mpc_apicid;
	CPUBase->CPUB_BOOT_Logical  = apicid;
        ThisCPU = (struct CPU_Definition *)CPUList->CPU_CPUList.mlh_Head;
        ThisCPU->CPU_Physical = mc->mpc_apicid;
        ThisCPU->CPU_Enabled = TRUE;    /* got to be enabled or we wouldnt be here.. */
        ThisCPU->CPU_IsOnline = TRUE;                                                                   /* CPU is online ..                     */
        ThisCPU->CPU_BootCPU = TRUE;
        ThisCPU->CPU_SMPGroup = SMP_Group;
        kprintf(DEBUG_NAME_STR ": CPU List item for BOOT CPU updated..[PhysicalID=%d]!\n", ThisCPU->CPU_Physical);
    }
    else
    {
        ThisCPU = AllocMem( sizeof(struct CPU_Definition), MEMF_CLEAR | MEMF_PUBLIC );                  /* Add this CPU to the CPU List         */

        if( ThisCPU == NULL )
        {
            kprintf(DEBUG_NAME_STR ":ERROR - Couldnt allocate CPU list item memory!\n");
        }
        else
        {
            CPUList->CPU_Physical += 1;                                                               /* Increment the System Processor count */

            ThisCPU->CPU_Family = CPU_Family_i386;                                                      /* we are i386 compatable..             */
            ThisCPU->CPU_Model = CPU_i386_386;                                                          /* will probe more later ..             */
            ThisCPU->CPU_Physical = mc->mpc_apicid;
            ThisCPU->CPU_Enabled = (mc->mpc_cpuflag & CPU_ENABLED);
            ThisCPU->CPU_ID = CPUList->CPU_Physical;                                                    /* we are the only CPU at this time ..  */
            ThisCPU->CPU_IsOnline = FALSE;                                                              /* CPU is online ..                     */
            ThisCPU->CPU_BootCPU = FALSE;                                                               /* CPU bootd system..                   */
            ThisCPU->CPU_SMPGroup = SMP_Group;

            kprintf(DEBUG_NAME_STR ": New CPU List item created @ %p, for [PhysicalID=%d]\n", ThisCPU_intern, ThisCPU->CPU_Physical);

            ThisCPU_intern = AllocMem(sizeof(struct i386_compat_intern), MEMF_CLEAR | MEMF_PUBLIC );    /* Create its per CPU internal struct   */

            if( ThisCPU_intern == NULL )
            {
                kprintf(DEBUG_NAME_STR ":ERROR - Couldnt allocate CPU private struct!\n");
            }
            else
            {
                kprintf(DEBUG_NAME_STR ": i386 private structure allocated @ %p\n",ThisCPU_intern);
                ThisCPU->CPU_Private1 = ThisCPU_intern;                                                 /* We dont fill it in yet - probed l8r  */
            }
            AddTail(&CPUList->CPU_CPUList,&ThisCPU->CPU_CPUList);                                       /* Add the CPU to the system List       */
        }
    }

    SMP_Group->SMP_CPUCount += 1;                                                                       /* Increment the groups CPU count */

    cpuapicver = mc->mpc_apicver;

    //tmp = apicid_to_cpu_present(apicid);
    //physids_or(phys_cpu_present_map, phys_cpu_present_map, tmp);
    
    /*  Validate version    */
    if ( cpuapicver == 0x0 )
    {
	kprintf(DEBUG_NAME_STR ": WARNING - BIOS bug, APIC version is 0 for CPU#%d! fixing up to 0x10. (tell your hw vendor)\n", mc->mpc_apicid);
	cpuapicver = 0x10;
    }

    //apic_version[m->mpc_apicid] = ver;
    //bios_cpu_apicid[num_processors - 1] = m->mpc_apicid;
}

void parse_MP_IOAPIC (struct mpc_config_ioapic *mc, struct SMP_Definition *SMP_Group )
{
    if (!(mc->mpc_flags & MPC_APIC_USABLE)) return;

    kprintf(DEBUG_NAME_STR ": I/O APIC #%d Version %d at 0x%lX.\n",	mc->mpc_apicid, mc->mpc_apicver, mc->mpc_apicaddr);

    //if (nr_ioapics >= MAX_IO_APICS)
    //{
    //    kprintf(DEBUG_NAME_STR ": CRITICAL - Max # of I/O APICs (%d) exceeded (found %d).\n",MAX_IO_APICS, nr_ioapics);
    //}

    if (!mc->mpc_apicaddr)
    {
        kprintf(DEBUG_NAME_STR ":ERROR - bogus zero I/O APIC address found in MP table, skipping!\n");
        return;
    }

    //mp_ioapics[nr_ioapics] = *mc;
	//nr_ioapics++;
}

void parse_MP_IntSrc (struct mpc_config_intsrc *mc, struct SMP_Definition *SMP_Group )
{
    //mp_irqs [mp_irq_entries] = *mc;
    kprintf(DEBUG_NAME_STR ": Int: type %d, pol %d, trig %d, bus %d, IRQ %02x, APIC ID %x, APIC INT %02x\n",	mc->mpc_irqtype, mc->mpc_irqflag & 3, (mc->mpc_irqflag >> 2) & 3, mc->mpc_srcbus, mc->mpc_srcbusirq, mc->mpc_dstapic, mc->mpc_dstirq);

#warning TODO: Replace the following print with ALERT
    //if (++mp_irq_entries == MAX_IRQ_SOURCES)  kprintf(DEBUG_NAME_STR ": DIE HERE!!! (BUG) ");
}

void parse_MP_LIntSrc (struct mpc_config_lintsrc *mc, struct SMP_Definition *SMP_Group )
{
    kprintf(DEBUG_NAME_STR ": Lint: type %d, pol %d, trig %d, bus %d, IRQ %02x, APIC ID %x, APIC LINT %02x\n", mc->mpc_irqtype, mc->mpc_irqflag & 3, (mc->mpc_irqflag >> 2) &3, mc->mpc_srcbusid, mc->mpc_srcbusirq, mc->mpc_destapic, mc->mpc_destapiclint);
    /*
	Apparently all exisitng SMP boards use ExtINT/LVT1 == LINT0 and NMI/LVT2 == LINT1 
    The following check will show us if this assumptions is false. Until then we do not have to add baggage.
    */
#warning TODO: Replace the following prints with ALERTS

    if ((mc->mpc_irqtype == mp_ExtINT) && (mc->mpc_destapiclint != 0)) kprintf(DEBUG_NAME_STR ": DIE HERE!!! (BUG) ");//BUG();

    if ((mc->mpc_irqtype == mp_NMI) && (mc->mpc_destapiclint != 1)) kprintf(DEBUG_NAME_STR ": DIE HERE!!! (BUG) ");//BUG();
}

void parse_MP_Bus (struct mpc_config_bus *mc, struct SMP_Definition *SMP_Group )
{
    char str[7];

    memcpy(str, mc->mpc_bustype, 6);
    str[6] = 0;

#warning TODO: Implemenet parse_MP_Bus
    //mpc_oem_bus_info(mc, str, translation_table[SMP_Group->SMP_RecordCount]);

    //if (strncmp(str, BUSTYPE_ISA, sizeof(BUSTYPE_ISA)-1) == 0)
    //{
    //	mp_bus_id_to_type[mc->mpc_busid] = MP_BUS_ISA;
    //}
    //else if (strncmp(str, BUSTYPE_EISA, sizeof(BUSTYPE_EISA)-1) == 0) mp_bus_id_to_type[mc->mpc_busid] = MP_BUS_EISA;
    //else if (strncmp(str, BUSTYPE_PCI, sizeof(BUSTYPE_PCI)-1) == 0)
    //{
    //	mpc_oem_pci_bus(mc, translation_table[SMP_Group->SMP_RecordCount]);
    //	mp_bus_id_to_type[mc->mpc_busid] = MP_BUS_PCI;
    //	mp_bus_id_to_pci_bus[mc->mpc_busid] = mp_current_pci_id;
    //	mp_current_pci_id++;
    //}
    //else if (strncmp(str, BUSTYPE_MCA, sizeof(BUSTYPE_MCA)-1) == 0) mp_bus_id_to_type[mc->mpc_busid] = MP_BUS_MCA;
    //else if (strncmp(str, BUSTYPE_NEC98, sizeof(BUSTYPE_NEC98)-1) == 0) mp_bus_id_to_type[mc->mpc_busid] = MP_BUS_NEC98;
    //else kprintf(DEBUG_NAME_STR ": WARNING - Unknown bustype %s - ignoring\n", str);
}

void parse_MP_Translation (struct mpc_config_translation *mc, struct SMP_Definition *SMP_Group)
{
    kprintf(DEBUG_NAME_STR ": Translation: record %d, type %d, quad %d, global %d, local %d\n", SMP_Group->SMP_RecordCount, mc->trans_type, mc->trans_quad, mc->trans_global, mc->trans_local);

    if (SMP_Group->SMP_RecordCount >= MAX_MPC_ENTRY) kprintf(DEBUG_NAME_STR ": ERROR - MAX_MPC_ENTRY exceeded!\n");
    //else translation_table[SMP_Group->SMP_RecordCount] = mc;                                /* stash this for later */
	
    //if (mc->trans_quad+1 > numnodes) numnodes = mc->trans_quad+1;
}

/**************************************************************************/

int mpfcb_checksum(unsigned char *mpcb, int len)
{
    int sum = 0;            /* Quickly grab the checksum of the config block */

    while (len--) sum += *mpcb++;

    return sum & 0xFF;
}

/**************************************************************************/

void smp_read_mpc_oem(struct mp_config_oemtable *oemtable, unsigned short oemsize, struct SMP_Definition *SMP_Group )
{
    int count = sizeof (*oemtable);                                                 /* the header size */
    unsigned char *oemptr = ((unsigned char *)oemtable)+count;
    
    SMP_Group->SMP_RecordCount = 0;
    kprintf(DEBUG_NAME_STR ": Found an OEM MPC table at %8p - parsing it ... \n", oemtable);

    if (memcmp(oemtable->oem_signature,MPC_OEM_SIGNATURE,4))
    {
	kprintf(DEBUG_NAME_STR ": WARNING - SMP mpc oemtable: bad signature [%c%c%c%c]!\n", oemtable->oem_signature[0],	oemtable->oem_signature[1],	oemtable->oem_signature[2],	oemtable->oem_signature[3]);
	return;
    }

    if (mpfcb_checksum((unsigned char *)oemtable,oemtable->oem_length))
    {
	kprintf(DEBUG_NAME_STR ": WARNING - SMP oem mptable: checksum error\n");
	return;
    }

    while (count < oemtable->oem_length)
    {
	switch (*oemptr) {
	    case MP_TRANSLATION:
	    {
		struct mpc_config_translation *mc=(struct mpc_config_translation *)oemptr;
		parse_MP_Translation( mc, SMP_Group );
		oemptr += sizeof(*mc);
		count += sizeof(*mc);
		++SMP_Group->SMP_RecordCount;
		break;
	    }
	    default:
	    {
		kprintf(DEBUG_NAME_STR ": WARNING - Unrecognised OEM table entry type - %d\n", (int) *oemptr);
		return;
	    }
	}
    }
}

void mps_oem_check(struct mp_config_table *mpcf, char *oem, char *productid, struct SMP_Definition *SMP_Group )
{
    if (strncmp(oem, "IBM NUMA", 8)) kprintf( DEBUG_NAME_STR ": WARNING -  May not be a NUMA-Q system.\n");
    if (mpcf->mpc_oemptr) smp_read_mpc_oem((struct mp_config_oemtable *) mpcf->mpc_oemptr, mpcf->mpc_oemsize, SMP_Group);
}

/****************************************************************************************/
/*  We are called very early to get the low memory for the SMP bootup trampoline page.  */

int smp_alloc_memory(void)
{
    ULONG                   trampoline_base;
#warning TODO: properly allocate SMP memory    
    //if ( ( trampoline_base = (void *) alloc_bootmem_low_pages(PAGE_SIZE)) >= 0x0009F000)   /* Has to be in very low memory so we can execute real-mode AP code.  */
    //{
        trampoline_base = 0x0009E000;
    //}
    
    return trampoline_base;
}

/**************************************************************************/

int scan_for_smpconfig (unsigned long base, unsigned long length)
{
    unsigned    int                 *basepointer = base;
    struct      intel_mp_confblock  *mpcfb;

    kprintf(DEBUG_NAME_STR ": Scan for SMP = %p for %ld bytes.\n", basepointer,length);

    while (length > 0)
    {
        mpcfb = (struct intel_mp_floating *)basepointer;

        if ((*basepointer == SMP_MAGIC_IDENT))
        {
            if ( (mpcfb->mpcf_length == 1) && !mpfcb_checksum( (unsigned char *)basepointer, 16) )
            {
                if ( ( ( mpcfb->mpcf_specification == 1 ) || ( mpcfb->mpcf_specification == 4 ) ) )
                {
                    return basepointer;
                }
            }
        }

        basepointer += 4;
	length -= 16;
    }
    return NULL;
}

int find_smp_config (void)
{
    unsigned int confAddr;

    if  ((confAddr = scan_for_smpconfig(0x00000000,0x400))) return confAddr;          /* Scan the bottom 1K for a signature        */
    if	((confAddr = scan_for_smpconfig(0x0009FC00,0x400))) return confAddr;          /* Scan the top 1K of base RAM (639*0x400)   */
    if  ((confAddr = scan_for_smpconfig(0x000F0000,0x10000))) return confAddr;        /* Scan the 64K of bios                      */

    /*  If it is an SMP machine we should know now, unless the
        configuration is in an EISA/MCA bus machine with an
        extended bios data area.

        there is a real-mode segmented pointer pointing to the
        4K EBDA area at 0x0000040E, calculate and scan it here.

        Some Linux loaders corrupt the EBDA area, so this kind
        of SMP config may be less reliable, because the SMP
        table may have been corrupted during early boot. These
        loaders are buggy and should be fixed.                      */

    confAddr = *(unsigned short *)(0x0000040E);
    confAddr <<= 4;
    return scan_for_smpconfig(confAddr, 0x1000);
}

void get_smp_config ( struct intel_mp_confblock *mpcfb, struct CPUBase *CPUBase)
{
    struct  SMP_Definition  *SMP_Group;
    BOOL                    SMPERROR = FALSE;
    ULONG                   pic_mode;
    struct  ACPIBase        *ACPIBase;

    ACPIBase = CPUBase->CPUB_ACPIBase;

    kprintf(DEBUG_NAME_STR ": Processing SMP config...\n");

    /*
	ACPI may be used to obtain the entire SMP configuration or just to 
	enumerate/configure processors (CONFIG_ACPI_HT). 
    
        Note that ACPI supports both logical (e.g. Hyper-Threading) and physical 
	processors, where MPS only supports physical.                              */

    if (ACPIBase)
    {
        if (ACPIBase->ACPIB_ACPI_LAPIC && ACPIBase->ACPIB_ACPI_IOAPIC) {
            kprintf(DEBUG_NAME_STR ":   Using ACPI (MADT) for SMP configuration information\n");
	    return;
	}
	else if (ACPIBase->ACPIB_ACPI_LAPIC) kprintf(DEBUG_NAME_STR ": Using ACPI for processor (LAPIC) configuration information\n");
    }
    kprintf(DEBUG_NAME_STR ":   Intel MultiProcessor Specification v1.%d\n", mpcfb->mpcf_specification);

    if (mpcfb->mpcf_feature2 & (1<<7)) /* test bit 7 (IMCR|PIC) */
    {
        kprintf(DEBUG_NAME_STR ":   IMCR and PIC compatibility mode.\n");
	pic_mode = 1;
    }
    else
    {
        kprintf(DEBUG_NAME_STR ":   Virtual Wire compatibility mode.\n");
	pic_mode = 0;
    }

    /*  Now see if we need to read further. */
    if (mpcfb->mpcf_feature1 != 0)
    {
        kprintf(DEBUG_NAME_STR ":   Default MP configuration #%d\n", mpcfb->mpcf_feature1);
		//construct_default_ISA_mptable(mpf->mpf_feature1);
    }
    else if (mpcfb->mpcf_physptr)
    {

	/*  Read the physical hardware table.  Anything here will override the defaults.    */
	if (!(SMP_Group = smp_read_mpcfb((void *)mpcfb->mpcf_physptr, CPUBase)) )
        {
	    SMPERROR = TRUE;
            kprintf(DEBUG_NAME_STR ": ERROR - BIOS bug, MP table errors detected!...\n");
	}
        
        SMP_Group->SMP_PIC_Mode = pic_mode;

        /*
	    If there are no explicit MP IRQ entries, then we are
	    broken.  We set up most of the low 16 IO-APIC pins to
	    ISA defaults and hope it will work.                         */

        /*if (!mp_irq_entries)
        {
	    struct mpc_config_bus bus;

	    kprintf(DEBUG_NAME_STR ": BIOS bug, no explicit IRQ entries, using default mptable. (tell your hw vendor)\n");

	    bus.mpc_type = MP_BUS;
	    bus.mpc_busid = 0;
	    memcpy(bus.mpc_bustype, "ISA   ", 6);
	    parse_MP_Bus(&bus);

	    construct_default_ioirq_mptable(0);
	} */

    } 
    else SMPERROR = TRUE;

    if ( SMPERROR == TRUE )
    {
        mpcfb = NULL;
        kprintf(DEBUG_NAME_STR ": ERROR - disabling SMP support. (tell your hw vendor)\n");
    }
    else    kprintf(DEBUG_NAME_STR ": Processors: %d\n", SMP_Group->SMP_CPUCount);
    /* Only use the first configuration found. */
}

int smp_read_mpcfb(struct mp_config_table *mpcf, struct CPUBase *CPUBase)
{
    char                            str[16];
    char                            oem[10];
    int                             count = sizeof(*mpcf);
    unsigned char                   *mpt = ((unsigned char *)mpcf) + count;
    struct  SMP_Definition          *SMP_Group;
    struct  ACPIBase                *ACPIBase;

    ACPIBase = CPUBase->CPUB_ACPIBase;

    if (memcmp(mpcf->mpc_signature,MPC_SIGNATURE,4))
    {
	kprintf(DEBUG_NAME_STR ": ERROR - SMP mptable: bad signature [0x%x]!\n", *(ULONG *)mpcf->mpc_signature);
	return 0;
    }

    if (mpfcb_checksum((unsigned char *)mpcf,mpcf->mpc_length))
    {
	kprintf(DEBUG_NAME_STR ": ERROR - SMP mptable: checksum error!\n");
	return 0;
    }

    if (mpcf->mpc_spec != 0x01 && mpcf->mpc_spec != 0x04)
    {
        kprintf(DEBUG_NAME_STR ": ERROR - SMP mptable: bad table version (%d)!!\n",mpcf->mpc_spec);
        return 0;
    }

    if (!mpcf->mpc_lapic)
    {
	kprintf(DEBUG_NAME_STR ": ERROR - SMP mptable: null local APIC address!\n");
	return 0;
    }

    /* Create the SMP group block */
    SMP_Group = AllocMem( sizeof(struct SMP_Definition), MEMF_CLEAR | MEMF_PUBLIC );
    if (CPUBase->CPUB_SMP_Groups)
    {  
        AddTail(CPUBase->CPUB_SMP_Groups,&SMP_Group->SMP_SMPList);                       /* Add this SMP group to the list */
        kprintf(DEBUG_NAME_STR ": SMP Group Item @ 0x%p, Inserted into List @ 0x%p\n",&SMP_Group->SMP_SMPList,CPUBase->CPUB_SMP_Groups); 

        InitSemaphore( &SMP_Group->SMP_GrpLock);
        kprintf(DEBUG_NAME_STR ": Initialised SMP Group Semaphore\n");
    }
    else
    {
        NEWLIST((struct List *)&(SMP_Group->SMP_SMPList));                              /* Create This SMP group list                       */
        kprintf(DEBUG_NAME_STR ": First SMP List & Group created @ 0x%p\n",&SMP_Group->SMP_SMPList);   

        InitSemaphore( &SMP_Group->SMP_GrpLock);
        kprintf(DEBUG_NAME_STR ": Initialised SMP Group Semaphore\n");

        CPUBase->CPUB_SMP_Groups = SMP_Group;
    }

    memcpy(oem,mpcf->mpc_oem,8);
    oem[8]=0;

    memcpy(str,mpcf->mpc_productid,12);
    str[12]=0;

    kprintf(DEBUG_NAME_STR ":   OEM ID: %s Product ID: %s \n",oem,str);

    mps_oem_check( mpcf, oem, str, SMP_Group );

    kprintf(DEBUG_NAME_STR ":   APIC at: 0x%lX\n",mpcf->mpc_lapic);

    /*  Save the local APIC address (it might be non-default) - though only if we're not using ACPI.	*/

    if (!( ACPIBase )||( (ACPIBase) && (!ACPIBase->ACPIB_ACPI_LAPIC) )) SMP_Group->SMP_APIC = mpcf->mpc_lapic;

    /*  Now process the configuration blocks.   */

    SMP_Group->SMP_RecordCount = 0;
    while (count < mpcf->mpc_length)
    {
	switch(*mpt)
        {
	    case MP_PROCESSOR:
	    {
		struct mpc_config_processor *mc= (struct mpc_config_processor *)mpt;
		
		if (!( ACPIBase )||( (ACPIBase) && (!ACPIBase->ACPIB_ACPI_LAPIC) )) parse_MP_Processor( mc, SMP_Group ); /* ACPI may have already provided this data */

                mpt += sizeof( *mc );
		count += sizeof( *mc );
		break;
	    }
	    case MP_IOAPIC:
	    {
		struct mpc_config_ioapic *mc= (struct mpc_config_ioapic *)mpt;

		parse_MP_IOAPIC( mc, SMP_Group );
		mpt += sizeof( *mc );
		count += sizeof( *mc );
		break;
	    }
	    case MP_INTSRC:
	    {
		struct mpc_config_intsrc *mc= (struct mpc_config_intsrc *)mpt;

		parse_MP_IntSrc( mc, SMP_Group );
		mpt += sizeof( *mc );
		count += sizeof( *mc );
		break;
	    }
	    case MP_LINTSRC:
	    {
		struct mpc_config_lintsrc *mc= (struct mpc_config_lintsrc *)mpt;

		parse_MP_LIntSrc( mc, SMP_Group );
		mpt += sizeof( *mc );
		count += sizeof( *mc );
		break;
	    }
	    case MP_BUS:
	    {
		struct mpc_config_bus *mc= (struct mpc_config_bus *)mpt;

		parse_MP_Bus( mc, SMP_Group );
		mpt += sizeof( *mc );
		count += sizeof( *mc );
		break;
	    }
            default:
	    {
		count = mpcf->mpc_length;
		break;
	    }
	}
	++SMP_Group->SMP_RecordCount;
    }

    //clustered_apic_check();
    if (!SMP_Group->SMP_CPUCount) 	kprintf(DEBUG_NAME_STR ": SMP mptable - NO Processors Registered!\n");
    return SMP_Group;
}

/**************************************************************************/

#endif /* _SMP_C */
