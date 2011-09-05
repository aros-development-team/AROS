static inline void PrintString(const char *buf, struct KernelBase *KernelBase)
{
    while (*buf)
	krnPutC(*buf++, KernelBase);
}
