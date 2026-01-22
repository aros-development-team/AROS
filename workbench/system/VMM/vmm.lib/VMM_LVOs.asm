               SECTION   CODE
               XDEF      _LVOAllocVMem
               XDEF      _LVOFreeVMem
               XDEF      _LVOAvailVMem
               XDEF      _LVOAllocVVec
               XDEF      _LVOFreeVVec
               
_LVOAllocVMem  EQU       -$1E
_LVOFreeVMem   EQU       -$24
_LVOAvailVMem  EQU       -$2A
_LVOAllocVVec  EQU       -$30
_LVOFreeVVec   EQU       -$36

               end
