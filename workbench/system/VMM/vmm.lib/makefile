LIBOBJS = AllocVMem.o FreeVMem.o AvailVMem.o AllocVVec.o FreeVVec.o VMM_LVOs.o

all : vmm.library test_vmmlib vmm_lib.lib

.asm.o:
  phxass NOEXE QUIET LARGE $*.asm

vmm.library : vmm_lib.o
  blink from vmm_lib.o TO vmm.library  
  copy vmm.library libs:

vmm_lib.o : vmm_lib.asm vmm_lib.i /shared_defs.i

test_vmmlib : test_vmmlib.c
  sc link test_vmmlib.c

vmm_lib.lib : $(LIBOBJS)
  rm -f vmm_lib.lib
  oml vmm_lib.lib A $(LIBOBJS)

clean :
  delete $(LIBOBJS) quiet
  delete vmm_lib.o vmm_lib.lib vmm.library test_vmmlib test_vmmlib.o quiet

AllocVMem.o : AllocVMem.asm
FreeVMem.o  : FreeVMem.asm
AvailVMem.o : AvailVMem.asm
AllocVVec.o : AllocVVec.asm
FreeVVec.o  : FreeVVec.asm
VMM_LVOs.o   : VMM_LVOs.asm
