#include <proto/identify.h>
#include <libraries/identify.h>

#include <stdio.h>

static void idhardwarenum(void)
{
    puts("\nIdHardwareNum");
    puts("-------------");
    printf("System              %u\n", (unsigned int)IdHardwareNum(IDHW_SYSTEM, NULL));
    printf("CPU                 %u\n", (unsigned int)IdHardwareNum(IDHW_CPU, NULL));
    printf("FPU                 %u\n", (unsigned int)IdHardwareNum(IDHW_FPU, NULL));
    printf("MMU                 %u\n", (unsigned int)IdHardwareNum(IDHW_MMU, NULL));
    printf("OSVER               %u\n", (unsigned int)IdHardwareNum(IDHW_OSVER, NULL));
    printf("EXECVER             %u\n", (unsigned int)IdHardwareNum(IDHW_EXECVER, NULL));
    printf("WBVER               %u\n", (unsigned int)IdHardwareNum(IDHW_WBVER, NULL));
    printf("ROMSIZE             %u\n", (unsigned int)IdHardwareNum(IDHW_ROMSIZE, NULL));
    printf("CHIPSET             %u\n", (unsigned int)IdHardwareNum(IDHW_CHIPSET, NULL));
    printf("GFXSYS              %u\n", (unsigned int)IdHardwareNum(IDHW_GFXSYS, NULL));
    printf("CHIPRAM             %u\n", (unsigned int)IdHardwareNum(IDHW_CHIPRAM, NULL));
    printf("FASTRAM             %u\n", (unsigned int)IdHardwareNum(IDHW_FASTRAM, NULL));
    printf("RAM                 %u\n", (unsigned int)IdHardwareNum(IDHW_RAM, NULL));
    printf("SETPATCHVER         %u\n", (unsigned int)IdHardwareNum(IDHW_SETPATCHVER, NULL));
    printf("AUDIOSYS            %u\n", (unsigned int)IdHardwareNum(IDHW_AUDIOSYS, NULL));
    printf("OSNR                %u\n", (unsigned int)IdHardwareNum(IDHW_OSNR, NULL));
    printf("VMMCHIPRAM          %u\n", (unsigned int)IdHardwareNum(IDHW_VMMCHIPRAM, NULL));
    printf("VMMFASTRAM          %u\n", (unsigned int)IdHardwareNum(IDHW_VMMFASTRAM, NULL));
    printf("VMMRAM              %u\n", (unsigned int)IdHardwareNum(IDHW_VMMRAM, NULL));
    printf("PLNCHIPRAM          %u\n", (unsigned int)IdHardwareNum(IDHW_PLNCHIPRAM, NULL));
    printf("PLNFASTRAM          %u\n", (unsigned int)IdHardwareNum(IDHW_PLNFASTRAM, NULL));
    printf("PLNRAM              %u\n", (unsigned int)IdHardwareNum(IDHW_PLNRAM, NULL));
    printf("VBR                 %u\n", (unsigned int)IdHardwareNum(IDHW_VBR, NULL));
    printf("LASTALERT           %u\n", (unsigned int)IdHardwareNum(IDHW_LASTALERT, NULL));
    printf("VBLANKFREQ          %u\n", (unsigned int)IdHardwareNum(IDHW_VBLANKFREQ, NULL));
    printf("POWERFREQ           %u\n", (unsigned int)IdHardwareNum(IDHW_POWERFREQ, NULL));
    printf("ECLOCK              %u\n", (unsigned int)IdHardwareNum(IDHW_ECLOCK, NULL));
    printf("SLOWRAM             %u\n", (unsigned int)IdHardwareNum(IDHW_SLOWRAM, NULL));
    printf("GARY                %u\n", (unsigned int)IdHardwareNum(IDHW_GARY, NULL));
    printf("RAMSEY              %u\n", (unsigned int)IdHardwareNum(IDHW_RAMSEY, NULL));
    printf("BATTCLOCK           %u\n", (unsigned int)IdHardwareNum(IDHW_BATTCLOCK, NULL));
    printf("CHUNKYPLANAR        %u\n", (unsigned int)IdHardwareNum(IDHW_CHUNKYPLANAR, NULL));
    printf("POWERPC             %u\n", (unsigned int)IdHardwareNum(IDHW_POWERPC, NULL));
    printf("PPCCLOCK            %u\n", (unsigned int)IdHardwareNum(IDHW_PPCCLOCK, NULL));
    printf("CPUREV              %u\n", (unsigned int)IdHardwareNum(IDHW_CPUREV, NULL));
    printf("CPUCLOCK            %u\n", (unsigned int)IdHardwareNum(IDHW_CPUCLOCK, NULL));
    printf("FPUCLOCK            %u\n", (unsigned int)IdHardwareNum(IDHW_FPUCLOCK, NULL));
    printf("RAMACCESS           %u\n", (unsigned int)IdHardwareNum(IDHW_RAMACCESS, NULL));
    printf("RAMWIDTH            %u\n", (unsigned int)IdHardwareNum(IDHW_RAMWIDTH, NULL));
    printf("RAMCAS              %u\n", (unsigned int)IdHardwareNum(IDHW_RAMCAS, NULL));
    printf("RAMBANDWIDTH        %u\n", (unsigned int)IdHardwareNum(IDHW_RAMBANDWIDTH, NULL));
    printf("TCPIP               %u\n", (unsigned int)IdHardwareNum(IDHW_TCPIP, NULL));
    printf("PPCOS               %u\n", (unsigned int)IdHardwareNum(IDHW_PPCOS, NULL));
    printf("AGNUS               %u\n", (unsigned int)IdHardwareNum(IDHW_AGNUS, NULL));
    printf("AGNUSMODE           %u\n", (unsigned int)IdHardwareNum(IDHW_AGNUSMODE, NULL));
    printf("DENISE              %u\n", (unsigned int)IdHardwareNum(IDHW_DENISE, NULL));
    printf("DENISEREV           %u\n", (unsigned int)IdHardwareNum(IDHW_DENISEREV, NULL));
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
    printf("result %d string %s\n", (int)result, buffer);
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
    printf("result %d\n", (int)IdEstimateFormatSize("abcdef", NULL));
    printf("result %d\n", (int)IdEstimateFormatSize("abc$$def", NULL));
    printf("result %d\n", (int)IdEstimateFormatSize("abc$AUDIOSYS$def", NULL));
    printf("result %d\n", (int)IdEstimateFormatSize("abc$AUDYS$def", NULL));
}

static void idalert(void)
{
    puts("\nIdAlert");
    puts("-------");
    TEXT buffer1[30];
    TEXT buffer2[30];
    TEXT buffer3[30];
    TEXT buffer4[30];
    printf("result %d\n", (int)IdAlertTags(0x82010007, IDTAG_DeadStr, buffer1, IDTAG_SubsysStr, buffer2, IDTAG_GeneralStr, buffer3, IDTAG_SpecStr, buffer4, IDTAG_StrLength, 30, TAG_DONE));
    printf("DeadStr %s SubsysStr %s GeneralStr %s SpecStr %s\n", buffer1, buffer2, buffer3, buffer4);
    printf("result %d\n", (int)IdAlertTags(0x8400000C, IDTAG_DeadStr, buffer1, IDTAG_SubsysStr, buffer2, IDTAG_GeneralStr, buffer3, IDTAG_SpecStr, buffer4, IDTAG_StrLength, 30, TAG_DONE));
    printf("DeadStr %s SubsysStr %s GeneralStr %s SpecStr %s\n", buffer1, buffer2, buffer3, buffer4);
    printf("result %d\n", (int)IdAlertTags(0x04010003, IDTAG_DeadStr, buffer1, IDTAG_SubsysStr, buffer2, IDTAG_GeneralStr, buffer3, IDTAG_SpecStr, buffer4, IDTAG_StrLength, 30, TAG_DONE));
    printf("DeadStr %s SubsysStr %s GeneralStr %s SpecStr %s\n", buffer1, buffer2, buffer3, buffer4);
}

static void idexpansion(void)
{
    puts("\nIdExpansion");
    puts("-----------");
    TEXT buffer[30];
    printf("result %d\n", (int)IdExpansionTags(IDTAG_ManufStr, buffer, IDTAG_StrLength, 30, TAG_DONE));
    printf("ManufStr %s\n", buffer);
}

static void idfunction(void)
{
    puts("\nIdFunction");
    puts("-----------");
    TEXT buffer[30];
    printf("result %d\n", (int)IdFunctionTags("identify", 30, IDTAG_FuncNameStr, buffer, IDTAG_StrLength, 30, TAG_DONE));
    printf("FuncNameStr %s\n", buffer);
    printf("result %d\n", (int)IdFunctionTags("identify", 36, IDTAG_FuncNameStr, buffer, IDTAG_StrLength, 30, TAG_DONE));
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
