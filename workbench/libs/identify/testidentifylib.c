#include <proto/identify.h>
#include <libraries/identify.h>

#include <stdio.h>

static void idhardwarenum(void)
{
    puts("\nIdHardwareNum");
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
}

static void idhardware(void)
{
    puts("\nIdHardware");
    puts("----------");
    printf("IDHW_SYSTEM         %s\n", IdHardware(IDHW_SYSTEM, NULL));
    printf("IDHW_CPU            %s\n", IdHardware(IDHW_CPU, NULL));
    printf("IDHW_FPU            %s\n", IdHardware(IDHW_FPU, NULL));
    printf("IDHW_MMU            %s\n", IdHardware(IDHW_MMU, NULL));
    printf("IDHW_OSVER          %s\n", IdHardwareTags(IDHW_OSVER, TAG_DONE)); // variadic
    printf("IDHW_EXECVER        %s\n", IdHardware(IDHW_EXECVER, NULL));
    printf("IDHW_WBVER          %s\n", IdHardware(IDHW_WBVER, NULL));
    printf("IDHW_ROMSIZE        %s\n", IdHardware(IDHW_ROMSIZE, NULL));
    printf("IDHW_CHIPSET        %s\n", IdHardware(IDHW_CHIPSET, NULL));
    printf("IDHW_GFXSYS         %s\n", IdHardware(IDHW_GFXSYS, NULL));
    printf("IDHW_CHIPRAM        %s\n", IdHardware(IDHW_CHIPRAM, NULL));
    printf("IDHW_FASTRAM        %s\n", IdHardware(IDHW_FASTRAM, NULL));
    printf("IDHW_RAM            %s\n", IdHardware(IDHW_RAM, NULL));
    printf("IDHW_SETPATCHVER    %s\n", IdHardware(IDHW_SETPATCHVER, NULL));
    printf("IDHW_AUDIOSYS       %s\n", IdHardware(IDHW_AUDIOSYS, NULL));
    printf("IDHW_OSNR           %s\n", IdHardware(IDHW_OSNR, NULL));
    printf("IDHW_VMMCHIPRAM     %s\n", IdHardware(IDHW_VMMCHIPRAM, NULL));
    printf("IDHW_VMMFASTRAM     %s\n", IdHardware(IDHW_VMMFASTRAM, NULL));
    printf("IDHW_VMMRAM         %s\n", IdHardware(IDHW_VMMRAM, NULL));
    printf("IDHW_PLNCHIPRAM     %s\n", IdHardware(IDHW_PLNCHIPRAM, NULL));
    printf("IDHW_PLNFASTRAM     %s\n", IdHardware(IDHW_PLNFASTRAM, NULL));
    printf("IDHW_PLNRAM         %s\n", IdHardware(IDHW_PLNRAM, NULL));
    printf("IDHW_VBR            %s\n", IdHardware(IDHW_VBR, NULL));
    printf("IDHW_LASTALERT      %s\n", IdHardware(IDHW_LASTALERT, NULL));
    printf("IDHW_VBLANKFREQ     %s\n", IdHardware(IDHW_VBLANKFREQ, NULL));
    printf("IDHW_POWERFREQ      %s\n", IdHardware(IDHW_POWERFREQ, NULL));
    printf("IDHW_ECLOCK         %s\n", IdHardware(IDHW_ECLOCK, NULL));
    printf("IDHW_SLOWRAM        %s\n", IdHardware(IDHW_SLOWRAM, NULL));
    printf("IDHW_GARY           %s\n", IdHardware(IDHW_GARY, NULL));
    printf("IDHW_RAMSEY         %s\n", IdHardware(IDHW_RAMSEY, NULL));
    printf("IDHW_BATTCLOCK      %s\n", IdHardware(IDHW_BATTCLOCK, NULL));
    printf("IDHW_CHUNKYPLANAR   %s\n", IdHardware(IDHW_CHUNKYPLANAR, NULL));
    printf("IDHW_POWERPC        %s\n", IdHardware(IDHW_POWERPC, NULL));
    printf("IDHW_PPCCLOCK       %s\n", IdHardware(IDHW_PPCCLOCK, NULL));
    printf("IDHW_CPUREV         %s\n", IdHardware(IDHW_CPUREV, NULL));
    printf("IDHW_CPUCLOCK       %s\n", IdHardware(IDHW_CPUCLOCK, NULL));
    printf("IDHW_FPUCLOCK       %s\n", IdHardware(IDHW_FPUCLOCK, NULL));
    printf("IDHW_RAMACCESS      %s\n", IdHardware(IDHW_RAMACCESS, NULL));
    printf("IDHW_RAMWIDTH       %s\n", IdHardware(IDHW_RAMWIDTH, NULL));
    printf("IDHW_RAMCAS         %s\n", IdHardware(IDHW_RAMCAS, NULL));
    printf("IDHW_RAMBANDWIDT    %s\n", IdHardware(IDHW_RAMBANDWIDTH, NULL));
    printf("IDHW_TCPIP          %s\n", IdHardware(IDHW_TCPIP, NULL));
    printf("IDHW_PPCOS          %s\n", IdHardware(IDHW_PPCOS, NULL));
    printf("IDHW_AGNUS          %s\n", IdHardware(IDHW_AGNUS, NULL));
    printf("IDHW_AGNUSMODE      %s\n", IdHardware(IDHW_AGNUSMODE, NULL));
    printf("IDHW_DENISE         %s\n", IdHardware(IDHW_DENISE, NULL));
    printf("IDHW_DENISEREV      %s\n", IdHardware(IDHW_DENISEREV, NULL));
}

static void idformatstringtst(STRPTR format, ULONG len)
{
    TEXT buffer[50] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    LONG result = IdFormatString(format, buffer, len, NULL);
    printf("result %d string %s\n", result, buffer);
}

static void idformatstring(void)
{
    puts("\nIdFormatString");
    puts("--------------");
    idformatstringtst("abcdef", 50);
    idformatstringtst("abcdef", 3); // shortened
    idformatstringtst("ab$$cd$$ef", 50); // $$ -> $
    idformatstringtst("ab$$cd$$ef", 4);
    idformatstringtst("abc$AUDIOSYS$def", 50);
    idformatstringtst("abc$AUDIOSYS$def", 5); // shortened
    idformatstringtst("abc$AUDYS$def", 50); // mis-spelled command
}

static void idestimateformatsize(void)
{
    puts("\nIdEstimateFormatSize");
    puts("--------------------");
    printf("result %d\n", IdEstimateFormatSize("abcdef", NULL));
    printf("result %d\n", IdEstimateFormatSize("abc$$def", NULL));
    printf("result %d\n", IdEstimateFormatSize("abc$AUDIOSYS$def", NULL));
    printf("result %d\n", IdEstimateFormatSize("abc$AUDYS$def", NULL));
}

static void idalert(void)
{
    puts("\nIdAlert");
    puts("-------");
    TEXT buffer1[30];
    TEXT buffer2[30];
    printf("result %d\n", IdAlertTags(42, IDTAG_DeadStr, buffer1, IDTAG_SpecStr, buffer2, IDTAG_StrLength, 30, TAG_DONE));
    printf("DeadStr %s SpecStr %s\n", buffer1, buffer2);
}

static void idexpansion(void)
{
    puts("\nIdExpansion");
    puts("-----------");
    TEXT buffer[30];
    printf("result %d\n", IdExpansionTags(IDTAG_ManufStr, buffer, IDTAG_StrLength, 30, TAG_DONE));
    printf("ManufStr %s\n", buffer);
}

static void idfunction(void)
{
    puts("\nIdFunction");
    puts("-----------");
    TEXT buffer[30];
    printf("result %d\n", IdFunctionTags("identify", 30, IDTAG_FuncNameStr, buffer, IDTAG_StrLength, 30, TAG_DONE));
    printf("FuncNameStr %s\n", buffer);
    printf("result %d\n", IdFunctionTags("identify", 36, IDTAG_FuncNameStr, buffer, IDTAG_StrLength, 30, TAG_DONE));
    printf("FuncNameStr %s\n", buffer);
}

int main(void)
{
    idhardwarenum();
    idhardware();
    idformatstring();
    idestimateformatsize();
    idalert();
    idexpansion();
    idfunction();

    return 0;
}
