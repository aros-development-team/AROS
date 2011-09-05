void *krnAllocBootMem(unsigned long size);
void *krnAllocBootMemAligned(unsigned long size, unsigned int align);

extern void *BootMemPtr;
extern void *BootMemLimit;
