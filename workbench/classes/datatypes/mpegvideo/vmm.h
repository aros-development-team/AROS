
#ifndef VMM_H
#define VMM_H 1

/*
**
**  $VER: vmm.h 1.10 (16.10.97)
**  mpegvideo.datatype 1.10
**
**  includes for VMM support (virtual memory)
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**
*/

/* amiga prototypes */
APTR  AllocVMem( unsigned long byteSize, unsigned long requirements );
void  FreeVMem( APTR memoryBlock, unsigned long byteSize );
ULONG AvailVMem( unsigned long requirements );
APTR  AllocVVec( unsigned long byteSize, unsigned long requirements );
void  FreeVVec( APTR memoryBlock );

#if !defined(__AROS__)
/* amiga pragmas */
#pragma libcall VMMBase AllocVMem  1e 1002
#pragma libcall VMMBase FreeVMem   24 0902
#pragma libcall VMMBase AvailVMem  2a 101
#pragma libcall VMMBase AllocVVec  30 1002
#pragma libcall VMMBase FreeVVec   36 901
#endif

#endif /* !VMM_H */

