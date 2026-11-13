# AROS programs are relocatable ELF objects that the loader in dos.library
# links as it loads them, resolving direct relocations only: the target has
# neither a GOT nor a PLT. Position independent code emits relocations
# against both, which the loader can only refuse, so drop the flags CMake
# would otherwise add for a project that asks for PIC.
set(CMAKE_C_COMPILE_OPTIONS_PIC "")
set(CMAKE_C_COMPILE_OPTIONS_PIE "")
set(CMAKE_C_LINK_OPTIONS_PIE "")
