#include <proto/identify.h>
#include <libraries/identify.h>

#include <stdio.h>

int main(void)
{
    puts("IdHardwareNum");
    puts("-------------");
    printf("System              %u\n", IdHardwareNum(IDHW_SYSTEM, NULL));
    printf("CPU                 %u\n", IdHardwareNum(IDHW_CPU, NULL));
    printf("FPU                 %u\n", IdHardwareNum(IDHW_FPU, NULL));
    printf("MMU                 %u\n", IdHardwareNum(IDHW_MMU, NULL));
    printf("OSVER               %u\n", IdHardwareNum(IDHW_OSVER, NULL));
    printf("EXECVER             %u\n", IdHardwareNum(IDHW_EXECVER, NULL));
    printf("WBVER               %u\n", IdHardwareNum(IDHW_WBVER, NULL));
    printf("ROMSIZE             %u\n", IdHardwareNum(IDHW_ROMSIZE, NULL));
    printf("CHIPSET             %u\n", IdHardwareNum(IDHW_CHIPSET, NULL));
    printf("GFXSYS              %u\n", IdHardwareNum(IDHW_GFXSYS, NULL));
    printf("CHIPRAM             %u\n", IdHardwareNum(IDHW_CHIPRAM, NULL));
    printf("FASTRAM             %u\n", IdHardwareNum(IDHW_FASTRAM, NULL));
    printf("RAM                 %u\n", IdHardwareNum(IDHW_RAM, NULL));
    printf("SETPATCHVER         %u\n", IdHardwareNum(IDHW_SETPATCHVER, NULL));
    printf("AUDIOSYS            %u\n", IdHardwareNum(IDHW_AUDIOSYS, NULL));
    printf("OSNR                %u\n", IdHardwareNum(IDHW_OSNR, NULL));
    printf("VMMCHIPRAM          %u\n", IdHardwareNum(IDHW_VMMCHIPRAM, NULL));
    printf("VMMFASTRAM          %u\n", IdHardwareNum(IDHW_VMMFASTRAM, NULL));
    printf("VMMRAM              %u\n", IdHardwareNum(IDHW_VMMRAM, NULL));
    printf("PLNCHIPRAM          %u\n", IdHardwareNum(IDHW_PLNCHIPRAM, NULL));
    printf("PLNFASTRAM          %u\n", IdHardwareNum(IDHW_PLNFASTRAM, NULL));
    printf("PLNRAM              %u\n", IdHardwareNum(IDHW_PLNRAM, NULL));
    printf("VBR                 %u\n", IdHardwareNum(IDHW_VBR, NULL));
    printf("LASTALERT           %u\n", IdHardwareNum(IDHW_LASTALERT, NULL));
    printf("VBLANKFREQ          %u\n", IdHardwareNum(IDHW_VBLANKFREQ, NULL));
    printf("POWERFREQ           %u\n", IdHardwareNum(IDHW_POWERFREQ, NULL));
    printf("ECLOCK              %u\n", IdHardwareNum(IDHW_ECLOCK, NULL));
    printf("SLOWRAM             %u\n", IdHardwareNum(IDHW_SLOWRAM, NULL));
    printf("GARY                %u\n", IdHardwareNum(IDHW_GARY, NULL));
    printf("RAMSEY              %u\n", IdHardwareNum(IDHW_RAMSEY, NULL));
    printf("BATTCLOCK           %u\n", IdHardwareNum(IDHW_BATTCLOCK, NULL));
    printf("CHUNKYPLANAR        %u\n", IdHardwareNum(IDHW_CHUNKYPLANAR, NULL));
    printf("POWERPC             %u\n", IdHardwareNum(IDHW_POWERPC, NULL));
    printf("PPCCLOCK            %u\n", IdHardwareNum(IDHW_PPCCLOCK, NULL));
    printf("CPUREV              %u\n", IdHardwareNum(IDHW_CPUREV, NULL));
    printf("CPUCLOCK            %u\n", IdHardwareNum(IDHW_CPUCLOCK, NULL));
    printf("FPUCLOCK            %u\n", IdHardwareNum(IDHW_FPUCLOCK, NULL));
    printf("RAMACCESS           %u\n", IdHardwareNum(IDHW_RAMACCESS, NULL));
    printf("RAMWIDTH            %u\n", IdHardwareNum(IDHW_RAMWIDTH, NULL));
    printf("RAMCAS              %u\n", IdHardwareNum(IDHW_RAMCAS, NULL));
    printf("RAMBANDWIDTH        %u\n", IdHardwareNum(IDHW_RAMBANDWIDTH, NULL));
    printf("TCPIP               %u\n", IdHardwareNum(IDHW_TCPIP, NULL));
    printf("PPCOS               %u\n", IdHardwareNum(IDHW_PPCOS, NULL));
    printf("AGNUS               %u\n", IdHardwareNum(IDHW_AGNUS, NULL));
    printf("AGNUSMODE           %u\n", IdHardwareNum(IDHW_AGNUSMODE, NULL));
    printf("DENISE              %u\n", IdHardwareNum(IDHW_DENISE, NULL));
    printf("DENISEREV           %u\n", IdHardwareNum(IDHW_DENISEREV, NULL));

    puts("IdHardware");
    puts("----------");
    printf("OSVER               %s\n", IdHardware(IDHW_OSVER, NULL));

    return 0;
}
