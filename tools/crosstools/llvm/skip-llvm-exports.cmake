# Loaded via CMAKE_PROJECT_INCLUDE by the linklib builds for llvm 20 to 22.
# LLVMConfig only includes LLVMExports when LLVMSupport is not yet a target,
# so defining it here skips the imported targets, as LLVM_OMIT_EXPORTS_FROM_CONFIG
# does on llvm 23. The exports of a shared-library build are SHARED imports,
# and the exports CMake >= 4.0 writes enable CMP0164, which refuses them on
# the Generic target platform.
if(NOT TARGET LLVMSupport)
  add_library(LLVMSupport INTERFACE IMPORTED)
endif()
