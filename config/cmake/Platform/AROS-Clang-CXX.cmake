# See AROS-GNU-CXX.cmake: the AROS loader resolves direct relocations only,
# so position independent code (which needs a GOT and a PLT) can never be
# built for this target.
set(CMAKE_CXX_COMPILE_OPTIONS_PIC "")
set(CMAKE_CXX_COMPILE_OPTIONS_PIE "")
set(CMAKE_CXX_LINK_OPTIONS_PIE "")
