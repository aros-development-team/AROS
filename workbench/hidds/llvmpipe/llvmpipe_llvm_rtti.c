/*
    Force LLVM RTTI objects into the llvmpipe HIDD binary.
*/

const void *llvmpipe_ObjectCache_typeinfo asm("_ZTIN4llvm11ObjectCacheE") = 0;
const void *llvmpipe_RTDyldMemoryManager_typeinfo asm("_ZTIN4llvm19RTDyldMemoryManagerE") = 0;

void
Llvmpipe_ForceLLVMPipeRTTI(void)
{
    (void)llvmpipe_ObjectCache_typeinfo;
    (void)llvmpipe_RTDyldMemoryManager_typeinfo;
}
