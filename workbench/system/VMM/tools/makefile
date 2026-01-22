TARGETS = VMMStat ShowPagesize ConvVMMCfg ReadMMUConfig VMMUsage VMMUsageCLI

AFLAGS = SET "PAGE4K=1" NOEXE QUIET LARGE I "/"

.c.o:
  sc IDIR MUI:developer/C/include nolink data faronly $*.c 

all : $(TARGETS)

VMMStat : VMMStat.c /shared_defs.h
  sc link nodebug pname VMMStat VMMStat.c

VMMUsageCLI : VMMUsageCLI.c
  sc link nodebug VMMUsageCLI.c

ShowPageSize : ShowPageSize.o sv_regs30.o sv_regs40.o sv_regs60.o sv_regs851.o
  sc link data faronly nodebug pname ShowPageSize ShowPageSize.o sv_regs30.o \
          sv_regs40.o sv_regs60.o sv_regs851.o

ReadMMUConfig : ReadMMUConfig.o sv_regs30.o sv_regs40.o sv_regs60.o sv_regs851.o
  sc link ReadMMUConfig.o sv_regs30.o sv_regs40.o sv_regs60.o sv_regs851.o

ConvVMMCfg : ConvVMMCfg.c
  sc link ConvVMMCfg.c

VMMUsage : VMMUsage.o /timer.o
  sc data faronly link VMMUsage.o /timer.o

sv_regs30.o : /sv_regs30.asm
  phxass $(AFLAGS) /sv_regs30.asm to sv_regs30.o

sv_regs40.o : /sv_regs40.asm
  phxass $(AFLAGS) /sv_regs40.asm to sv_regs40.o

sv_regs60.o : /sv_regs60.asm
  phxass $(AFLAGS) /sv_regs60.asm to sv_regs60.o

sv_regs851.o : /sv_regs851.asm
  phxass $(AFLAGS) /sv_regs851.asm to sv_regs851.o

/timer.o : /timer.c
  cd /
  make timer.o
  cd tools

ReadMMUConfig.o : /protos.h

clean :
  rm -f $(TARGETS)
  rm -f *.o *.lnk
