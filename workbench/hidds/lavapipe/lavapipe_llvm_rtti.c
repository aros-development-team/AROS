/*
    Force LLVM RTTI objects into the lavapipe HIDD binary.
*/

const void *lavapipe_ObjectCache_typeinfo asm("_ZTIN4llvm11ObjectCacheE") = 0;
const void *lavapipe_RTDyldMemoryManager_typeinfo asm("_ZTIN4llvm19RTDyldMemoryManagerE") = 0;

void
Lavapipe_ForceLLVMPipeRTTI(void)
{
    (void)lavapipe_ObjectCache_typeinfo;
    (void)lavapipe_RTDyldMemoryManager_typeinfo;
}
