/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <resources/processor.h>
#include <string.h>

#include "processor_arch_intern.h"

static const char *vendors[] =
{
    "AuthenticAMD",
    "GenuineIntel",
    "CyrixInstead",
    "UMC UMC UMC ",
    "NexGenDriven",
    "CentaurHauls",
    "RiseRiseRise",
    "SiS SiS SiS ",
    "GenuineTMx86",
    "Geode by NSC",
    NULL
};

static void ReadVendorID(struct X86ProcessorInformation * info)
{
    ULONG eax, ebx, ecx, edx;
    ULONG * ulongptr = NULL;
    ULONG index = 0;
    info->Vendor = VENDOR_UNKNOWN;
    info->CPUIDHighestStandardFunction = 0x0;
    info->CPUIDHighestExtendedFunction = 0x0;
    
D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));
    
    /* Reading CPU Vendor ID */
    ulongptr = (ULONG *)info->VendorID;
    index = 0;
    cpuid(0x00000000);
    info->CPUIDHighestStandardFunction = eax;
    ulongptr[index++] = ebx;ulongptr[index++] = edx;
    ulongptr[index++] = ecx;info->VendorID[12] = 0;

    /* Select manufacturer based on Vendor ID */
    for (index = 0; vendors[index]; index++)
    {
        if (strcmp(info->VendorID, vendors[index]) == 0)
        {
            info->Vendor = index + VENDOR_AMD;
            break;
        }
    }

    /* Reading Highest Extended Function */
    cpuid(0x80000000);
    info->CPUIDHighestExtendedFunction = eax;
    

}

static void ReadBrandString(struct X86ProcessorInformation * info)
{
    ULONG eax, ebx, ecx, edx;
    ULONG * ulongptr = NULL;
    ULONG index = 0;

D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));
    
    /* Reading CPU Brand String */
    ulongptr = (ULONG *)info->BrandStringBuffer;
    index = 0;
    cpuid(0x80000002);
    ulongptr[index++] = eax;ulongptr[index++] = ebx;
    ulongptr[index++] = ecx;ulongptr[index++] = edx;
    cpuid(0x80000003);
    ulongptr[index++] = eax;ulongptr[index++] = ebx;
    ulongptr[index++] = ecx;ulongptr[index++] = edx;
    cpuid(0x80000004);
    ulongptr[index++] = eax;ulongptr[index++] = ebx;
    ulongptr[index++] = ecx;ulongptr[index++] = edx;
    
    /* Left trim */
    info->BrandString = info->BrandStringBuffer;
    
    while(*info->BrandString == ' ') info->BrandString++;
    
}

static ULONG AMDDeriveFamily(UBYTE basefamily, UBYTE extendedfamily)
{
    ULONG family = 0;
    ULONG ret;

D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    if (basefamily == 0x0f) 
        family = basefamily + extendedfamily;
    else
        family = basefamily;
    
    switch(family)
    {
    case(0x0b): ret = CPUFAMILY_AMD_K5; break;
    case(0x0c): ret = CPUFAMILY_AMD_K6; break;
    case(0x0d): ret = CPUFAMILY_AMD_K7; break;
    case(0x0e): ret = CPUFAMILY_AMD_K8; break;
    case(0x0f): ret = CPUFAMILY_AMD_K9; break;
    case(0x10): ret = CPUFAMILY_AMD_K10; break;
    default: ret = CPUFAMILY_UNKNOWN; break;
    }
    
    return ret;
}

static ULONG IntelDeriveFamily(UBYTE basefamily, UBYTE extendedfamily)
{
    ULONG family = extendedfamily + basefamily;
    ULONG ret;

D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    switch(family)
    {
    case(4): ret = CPUFAMILY_INTEL_486; break;
    case(5): ret = CPUFAMILY_INTEL_PENTIUM; break;
    case(6): ret = CPUFAMILY_INTEL_PENTIUM_PRO; break;
    case(15): ret = CPUFAMILY_INTEL_PENTIUM4; break;
    default: ret = CPUFAMILY_UNKNOWN; break;
    }
    
    return ret;
}

static void ReadFamilyModelStepping(struct X86ProcessorInformation * info)
{
    ULONG eax, ebx, ecx, edx;
    UBYTE stepping, basefamily, extendedfamily, basemodel, extendedmodel;

D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    /* Reading Family/Model/Stepping */
    cpuid(0x00000001);

    stepping = eax & 0x0f;
    basefamily = (eax >> 8) & 0x0f;
    extendedfamily = (eax >> 20) & 0xff;
    basemodel = (eax >> 4) & 0x0f;
    extendedmodel = (eax >> 16) & 0x0f;
    
    /* Read family */
    info->Family = CPUFAMILY_UNKNOWN;
    switch(info->Vendor)
    {
    case(VENDOR_AMD): 
        info->Family = AMDDeriveFamily(basefamily, extendedfamily); 
        info->Model = basemodel | (basefamily < 0x0f ? 0 : (extendedmodel << 4));
        (void)stepping; /* FIXME: Why is this unused? */
        break;
    case(VENDOR_INTEL): 
        info->Family = IntelDeriveFamily(basefamily, extendedfamily);
        info->Model = basemodel | (extendedmodel << 4);
        (void)stepping; /* FIXME: Why is this unused? */
        break;
    }
}

static void ReadFeaturesFlags(struct X86ProcessorInformation * info)
{
    ULONG eax, ebx, ecx, edx;

D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    /* Reading standard feature flags */
    cpuid(0x00000001);

    switch(info->Vendor)
    {
    case(VENDOR_AMD): edx &= FEATURE_MASK_EDX_AMD; ecx &= FEATURE_MASK_ECX_AMD; break;
    case(VENDOR_INTEL): edx &= FEATURE_MASK_EDX_INTEL; ecx &= FEATURE_MASK_ECX_INTEL; break;
    default: edx &= FEATURE_MASK_EDX_UNKNOWN; ecx &= FEATURE_MASK_ECX_UNKNOWN; break;
    }
    
    info->Features1 = edx;
    info->Features2 = ecx;

    /* Read extended information */
    cpuid(0x80000001);
    switch(info->Vendor)
    {
    case(VENDOR_AMD): edx &= FEATURE_MASK_EDX_EXT_AMD; ecx &= FEATURE_MASK_ECX_EXT_AMD; break;
    case(VENDOR_INTEL): edx &= FEATURE_MASK_EDX_EXT_INTEL; ecx &= FEATURE_MASK_ECX_EXT_INTEL; break;
    default: edx &= FEATURE_MASK_EDX_UNKNOWN; ecx &= FEATURE_MASK_ECX_UNKNOWN; break;
    }
    
    info->Features3 = edx;
    info->Features4 = ecx;
    
    /* Calculate the vector unit */
    if (info->Features2 & FEATF_SSE42)
        info->VectorUnit = VECTORTYPE_SSE42;
    else if (info->Features2 & FEATF_SSE41)
        info->VectorUnit = VECTORTYPE_SSE41;
    else if (info->Features4 & FEATF_SSE4A)
        info->VectorUnit = VECTORTYPE_SSE4A;
    else if (info->Features2 & FEATF_SSSE3)
        info->VectorUnit = VECTORTYPE_SSSE3;
    else if (info->Features2 & FEATF_SSE3)
        info->VectorUnit = VECTORTYPE_SSE3;
    else if (info->Features1 & FEATF_SSE2)
        info->VectorUnit = VECTORTYPE_SSE2;
    else if (info->Features1 & FEATF_SSE)
        info->VectorUnit = VECTORTYPE_SSE;
    else if (info->Features3 & FEATF_3DNOWEXT)
        info->VectorUnit = VECTORTYPE_3DNOWEXT;
    else if (info->Features3 & FEATF_3DNOW)
        info->VectorUnit = VECTORTYPE_3DNOW;
    else if (info->Features3 & FEATF_MMXEXT)
        info->VectorUnit = VECTORTYPE_MMXEXT;
    else if (info->Features1 & FEATF_MMX)
        info->VectorUnit = VECTORTYPE_MMX;
    else
        info->VectorUnit = VECTORTYPE_NONE;
}

static void AMDDeriveCacheInformation(struct X86ProcessorInformation * info)
{
    ULONG eax, ebx, ecx, edx;
    info->CacheLineSize = 0xff;

D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    /* Reading L1 information */
    cpuid(0x80000005);

    info->L1DataCacheSize = (ecx >> 24);
    if ((ecx & 0xff) < info->CacheLineSize) info->CacheLineSize = (ecx & 0xff);
    info->L1InstructionCacheSize = (edx >> 24);
    if ((edx & 0xff) < info->CacheLineSize) info->CacheLineSize = (edx & 0xff);

    /* Reading L2/L3 information */
    cpuid(0x80000006);

    info->L2CacheSize = (ecx >> 16);
    if ((ecx & 0xff) < info->CacheLineSize) info->CacheLineSize = (ecx & 0xff);
    info->L3CacheSize = (edx >> 18) * 512;
    if ((edx & 0xff) < info->CacheLineSize) info->CacheLineSize = (edx & 0xff);
}

#define min(a,b) (a > b ? b : a)

static void IntelDecodeCacheKeyValue(struct X86ProcessorInformation * info, UBYTE key)
{

D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    switch(key)
    {
    case(0x06): info->L1InstructionCacheSize = 8; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x08): info->L1InstructionCacheSize = 16; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x09): info->L1InstructionCacheSize = 32; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x0A): info->L1DataCacheSize = 8; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x0C): info->L1DataCacheSize = 16; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x0D): info->L1DataCacheSize = 16; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x21): info->L2CacheSize = 256; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x22): info->L3CacheSize = 512; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x23): info->L3CacheSize = 1024; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x25): info->L3CacheSize = 2048; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x29): info->L3CacheSize = 4096; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x2C): info->L1DataCacheSize = 32; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x30): info->L1InstructionCacheSize = 32; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x39): info->L2CacheSize = 128; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x3A): info->L2CacheSize = 192; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x3B): info->L2CacheSize = 128; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x3C): info->L2CacheSize = 256; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x3D): info->L2CacheSize = 384; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x3E): info->L2CacheSize = 512; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x41): info->L2CacheSize = 128; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x42): info->L2CacheSize = 256; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x43): info->L2CacheSize = 512; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x44): info->L2CacheSize = 1024; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x45): info->L2CacheSize = 2048; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x46): info->L3CacheSize = 4096; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x47): info->L3CacheSize = 8192; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x48): info->L2CacheSize = 3072; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x49): info->L2CacheSize = 4096; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x4A): info->L3CacheSize = 6144; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x4B): info->L3CacheSize = 8192; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x4C): info->L3CacheSize = 12288; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x4D): info->L3CacheSize = 16384; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x4E): info->L2CacheSize = 6144; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x60): info->L1DataCacheSize = 16; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x66): info->L1DataCacheSize = 8; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x67): info->L1DataCacheSize = 16; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x68): info->L1DataCacheSize = 32; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x78): info->L2CacheSize = 1024; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x79): info->L2CacheSize = 128; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x7A): info->L2CacheSize = 256; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x7B): info->L2CacheSize = 512; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x7C): info->L2CacheSize = 1024; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x7D): info->L2CacheSize = 2048; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x7F): info->L2CacheSize = 512; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x82): info->L2CacheSize = 256; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x83): info->L2CacheSize = 512; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x84): info->L2CacheSize = 1024; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x85): info->L2CacheSize = 2048; info->CacheLineSize = min(32, info->CacheLineSize); break;
    case(0x86): info->L2CacheSize = 512; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0x87): info->L2CacheSize = 1024; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xD0): info->L3CacheSize = 512; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xD1): info->L3CacheSize = 1024; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xD2): info->L3CacheSize = 2048; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xD6): info->L3CacheSize = 1024; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xD7): info->L3CacheSize = 2048; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xD8): info->L3CacheSize = 4096; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xDC): info->L3CacheSize = 1536; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xDD): info->L3CacheSize = 3072; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xDE): info->L3CacheSize = 6144; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xE2): info->L3CacheSize = 2048; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xE3): info->L3CacheSize = 4096; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xE4): info->L3CacheSize = 8192; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xEA): info->L3CacheSize = 12288; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xEB): info->L3CacheSize = 18432; info->CacheLineSize = min(64, info->CacheLineSize); break;
    case(0xEC): info->L3CacheSize = 24576; info->CacheLineSize = min(64, info->CacheLineSize); break;
    }
}
static void IntelDeriveCacheInformation(struct X86ProcessorInformation * info)
{
    ULONG eax, ebx, ecx, edx;
    info->CacheLineSize = 0xff;

D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    /* Reading Cache Information */
    cpuid(0x00000002);
    
    if ((eax >> 31) == 0)
    {
        UBYTE temp;
        /* Decoding eax */
        temp = ((eax >> 24) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = ((eax >> 16) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = ((eax >> 8) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        /* AL is reserved */
    }
    
    if ((ebx >> 31) == 0)
    {
        UBYTE temp;
        /* Decoding ebx */
        temp = ((ebx >> 24) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = ((ebx >> 16) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = ((ebx >> 8) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = (ebx & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
    }
    
    if ((ecx >> 31) == 0)
    {
        UBYTE temp;
        /* Decoding ebx */
        temp = ((ecx >> 24) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = ((ecx >> 16) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = ((ecx >> 8) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = (ecx & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
    }

    if ((edx >> 31) == 0)
    {
        UBYTE temp;
        /* Decoding ebx */
        temp = ((edx >> 24) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = ((edx >> 16) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = ((edx >> 8) & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
        temp = (edx & 0xff);
        IntelDecodeCacheKeyValue(info, temp);
    }
}

static void ReadCacheInformation(struct X86ProcessorInformation * info)
{

D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    info->L1DataCacheSize = 0;
    info->L1InstructionCacheSize = 0;
    info->L2CacheSize = 0;
    info->L3CacheSize = 0;
    info->CacheLineSize = 0;
    
    switch(info->Vendor)
    {
    case(VENDOR_AMD): AMDDeriveCacheInformation(info); break;
    case(VENDOR_INTEL): IntelDeriveCacheInformation(info); break;
    }
}

static void ReadMSRSupportInformation(struct X86ProcessorInformation * info)
{
    ULONG eax, ebx, ecx, edx;

D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    info->APERFMPERF = FALSE;

    /* Check if MSR is supported */
    if (!(info->Features1 & FEATF_MSR))
        return;

    if (info->CPUIDHighestStandardFunction > 0x00000005)
    {   
        /* Reading Power Management Information */
        cpuid(0x00000006);
        if (ecx & 0x01)
            info->APERFMPERF = TRUE;
    }
}

VOID ReadProcessorInformation(struct X86ProcessorInformation * info)
{
D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    ReadVendorID(info);
    ReadBrandString(info);
    ReadFamilyModelStepping(info);
    ReadFeaturesFlags(info);
    ReadCacheInformation(info);
    ReadMSRSupportInformation(info);
    ReadMaxFrequencyInformation(info);
}

