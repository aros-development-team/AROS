/*
    (C) 1997-1999 AROS - The Amiga Research OS
    $Id$

    Desc: Simple CPU type/speed functions
    Lang: English
*/

/*****************************************************************************

    FUNCTION
	I've put there few usefull functions. They allow to determine
	CPU type and its speed.
	This pice of code is based mostly on linux sources.

*****************************************************************************/

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <proto/exec.h>

extern char *strcpy(char *,const char *);
extern int sprintf(char *,const char *,...);

static char	CPUName[48];	/* This one will keep CPU name */
static ULONG	CPUSpeed;	/* This one will keep CPU speed.
				   Fails at >4GHz ;-) */
static LONG	CPUVendor;	/* CPU vendor */
static LONG	CPU_cache_size;	/* CPU cache size */

#define	CPUV_INTEL	1
#define	CPUV_AMD	2
#define CPUV_CYRIX	3
#define CPUV_UMC	4
#define CPUV_CENTAUR	5
#define	CPUV_NEXGEN	6
#define	CPUV_UNKNOWN	-1

#define inb(port) \
    ({	char __value;	\
	__asm__ __volatile__ ("inb $" #port ",%%al":"=a"(__value));	\
	__value;	})

#define outb(val,port) \
    ({	char __value=(val);	\
	__asm__ __volatile__ ("outb %%al,$" #port::"a"(__value)); })

/*
 *	Generic CPUID function
 */
extern inline void cpuid(int op, int *eax, int *ebx, int *ecx, int *edx)
{
	__asm__("cpuid"
		: "=a" (*eax),
		  "=b" (*ebx),
		  "=c" (*ecx),
		  "=d" (*edx)
		: "a" (op)
		: "cc");
}

/* Determine CPU clock speed. */

#define LATCH ((1193180+50)/100)
#define CALIBRATE_LATCH	(5 * LATCH)
#define CALIBRATE_TIME	(5 * 1000020/100)
ULONG CalcCPUSpeed()
{
    /* Check if we have something better than 80486 */
    if (*(BYTE*)0x000009a0 > 4)
    {
	/* Set the Gate high, disable speaker */
	outb((inb(0x61) & ~0x02) | 0x01, 0x61);
	/*
	 * Now let's take care of CTC channel 2
	 *
	 * Set the Gate high, program CTC channel 2 for mode 0,
         * (interrupt on terminal count mode), binary count,
         * load 5 * LATCH count, (LSB and MSB) to begin countdown.
         */
        outb(0xb0, 0x43);			/* binary, mode 0, LSB/MSB, Ch 2 */
        outb(CALIBRATE_LATCH & 0xff, 0x42);	/* LSB of count */
        outb(CALIBRATE_LATCH >> 8, 0x42);	/* MSB of count */
        {	
	    unsigned long startlow, starthigh;
	    unsigned long endlow, endhigh;
	    unsigned long count;
    	    unsigned long eax=0, edx=1000000;
	    unsigned long cpu_hz;
    
	    __asm__ __volatile__("rdtsc":"=a" (startlow),"=d" (starthigh));
	    count = 0;
	    do {
		count++;
	    } while ((inb(0x61) & 0x20) == 0);
	    __asm__ __volatile__("rdtsc":"=a" (endlow),"=d" (endhigh));

	    /* Error: ECTCNEVERSET */
	    if (count <= 1)
		goto bad_ctc;

	    /* 64-bit subtract - gcc just messes up with long longs */
	    __asm__("subl %2,%0\n\t"
		    "sbbl %3,%1"
		    :"=a" (endlow), "=d" (endhigh)
		    :"g" (startlow), "g" (starthigh),
		     "0" (endlow), "1" (endhigh));

	    /* Error: ECPUTOOFAST */
	    if (endhigh)
		goto bad_ctc;

	    /* Error: ECPUTOOSLOW */
	    if (endlow <= CALIBRATE_TIME)
		goto bad_ctc;

	    __asm__("divl %2"
	    	    :"=a" (endlow), "=d" (endhigh)
		    :"r" (endlow), "0" (0), "1" (CALIBRATE_TIME));

    	    __asm__("divl %2"
		    :"=a" (cpu_hz), "=d" (edx)
        	    :"r" (endlow),
	    	    "0" (eax), "1" (edx));

	    return cpu_hz;
	}
    }
    /*
     * The CTC wasn't reliable: we got a hit on the very first read,
     * or the CPU was so fast/slow that the quotient wouldn't fit in
     * 32 bits..
     */
bad_ctc:
    return 0;
}

#define rdmsr(msr,val1,val2) \
       __asm__ __volatile__("rdmsr" \
			    : "=a" (val1), "=d" (val2) \
			    : "c" (msr))

#define wrmsr(msr,val1,val2) \
     __asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" (msr), "a" (val1), "d" (val2))

ULONG GetCPUSpeed()
{
    return CPUSpeed;
}

char *GetCPUName()
{
    return (char*)&CPUName;
}

static int get_amd_model_name()
{
	unsigned int n, dummy, *v;

	/* Actually we must have cpuid or we could never have
	 * figured out that this was AMD from the vendor info :-).
	 */

	cpuid(0x80000000, &n, &dummy, &dummy, &dummy);
	if (n < 4)
		return 0;
	cpuid(0x80000001, &dummy, &dummy, &dummy, (int*)0x000009ac);
	v = (unsigned int *) &CPUName;
	cpuid(0x80000002, &v[0], &v[1], &v[2], &v[3]);
	cpuid(0x80000003, &v[4], &v[5], &v[6], &v[7]);
	cpuid(0x80000004, &v[8], &v[9], &v[10], &v[11]);
	CPUName[48] = 0;
	return 1;
}

struct cpu_model_info {
	int vendor;
	int x86;
	char *model_names[16];
};

static struct cpu_model_info cpu_models[] = {
	{ CPUV_INTEL,	4,
	  { "486 DX-25/33", "486 DX-50", "486 SX", "486 DX/2", "486 SL", 
	    "486 SX/2", NULL, "486 DX/2-WB", "486 DX/4", "486 DX/4-WB", NULL, 
	    NULL, NULL, NULL, NULL, NULL }},
	{ CPUV_INTEL,	5,
	  { "Pentium 60/66 A-step", "Pentium 60/66", "Pentium 75 - 200",
	    "OverDrive PODP5V83", "Pentium MMX", NULL, NULL,
	    "Mobile Pentium 75 - 200", "Mobile Pentium MMX", NULL, NULL, NULL,
	    NULL, NULL, NULL, NULL }},
	{ CPUV_INTEL,	6,
	  { "Pentium Pro A-step", "Pentium Pro", NULL, "Pentium II (Klamath)", 
	    NULL, "Pentium II (Deschutes)", "Celeron (Mendocino)", NULL,
	    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }},
	{ CPUV_AMD,	4,
	  { NULL, NULL, NULL, "486 DX/2", NULL, NULL, NULL, "486 DX/2-WB",
	    "486 DX/4", "486 DX/4-WB", NULL, NULL, NULL, NULL, "Am5x86-WT",
	    "Am5x86-WB" }},
	{ CPUV_AMD,	5,
	  { "K5/SSA5", "K5",
	    "K5", "K5", NULL, NULL,
	    "K6", "K6", "K6-2",
	    "K6-3", NULL, NULL, NULL, NULL, NULL, NULL }},
	{ CPUV_UMC,	4,
	  { NULL, "U5D", "U5S", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	    NULL, NULL, NULL, NULL, NULL, NULL }},
	{ CPUV_CENTAUR,	5,
	  { NULL, NULL, NULL, NULL, "C6", NULL, NULL, NULL, "C6-2", NULL, NULL,
	    NULL, NULL, NULL, NULL, NULL }},
	{ CPUV_NEXGEN,	5,
	  { "Nx586", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	    NULL, NULL, NULL, NULL, NULL, NULL, NULL }},
};

void DetectCPU()
{
    char *vendor=(char*)0x000009b0;
    int i;
    char *p=NULL;
    
    CPUSpeed=CalcCPUSpeed();

    if (!strcmp(vendor, "GenuineIntel"))
	CPUVendor = CPUV_INTEL;
    else if (!strcmp(vendor, "AuthenticAMD"))
	CPUVendor = CPUV_AMD;
    else if (!strcmp(vendor, "CyrixInstead"))
	CPUVendor = CPUV_CYRIX;
    else if (!strcmp(vendor, "UMC UMC UMC "))
    	CPUVendor = CPUV_UMC;
    else if (!strcmp(vendor, "CentaurHauls"))
    	CPUVendor = CPUV_CENTAUR;
    else if (!strcmp(vendor, "NexGenDriven"))
    	CPUVendor = CPUV_NEXGEN;
    else
    	CPUVendor = CPUV_UNKNOWN;

    if (*(LONG*)0x000009a8 > 0 && CPUVendor == CPUV_INTEL)
    {
	if(*(LONG*)0x000009ac & (1<<18))
	{
	    /* Disable processor serial number on Intel Pentium III 
	       from code by Phil Karn */
	    unsigned long lo,hi;
	    rdmsr(0x119,lo,hi);
	    lo |= 0x200000;
	    wrmsr(0x119,lo,hi);
	}
    }

    if (CPUVendor == CPUV_AMD && get_amd_model_name())
	return;
    
    for (i = 0; i < sizeof(cpu_models)/sizeof(struct cpu_model_info); i++)
    {
	if (*(LONG*)0x000009a8 > 1)
	{
	    /* supports eax=2  call */
	    int edx, cache_size, dummy;
			
	    cpuid(2, &dummy, &dummy, &dummy, &edx);

	    /* We need only the LSB */
	    edx &= 0xff;

	    switch (edx)
	    {
		case 0x40:
		    cache_size = 0;
		    break;

		case 0x41:
		    cache_size = 128;
		    break;

		case 0x42:
		    cache_size = 256;
		    break;

		case 0x43:
		    cache_size = 512;
		    break;

		case 0x44:
		    cache_size = 1024;
		    break;

		case 0x45:
		    cache_size = 2048;
		    break;

		default:
		    cache_size = 0;
		    break;
	    }
	    CPU_cache_size = cache_size; 
	}

	if (cpu_models[i].vendor == CPUVendor &&
	    cpu_models[i].x86 == (int)*(BYTE*)0x000009a0)
	{
	    if (*(BYTE*)0x000009a2 <= 16)
		p = cpu_models[i].model_names[*(BYTE*)0x000009a2];

	    /* Names for the Pentium II processors */
	    if ((cpu_models[i].vendor == CPUV_INTEL)
	            && (cpu_models[i].x86 == 6) 
	            && (*(BYTE*)0x000009a2 == 5)
	            && (CPU_cache_size == 0))
	    {
		p = "Celeron (Covington)";
	    }
	}
    }

    if (p)
        strcpy((char*)&CPUName, p);
    else
	sprintf((char*)&CPUName,"80%d86",(int)*(BYTE*)0x000009a0);
}
