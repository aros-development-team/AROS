
#include <stdio.h>
#include <proto/exec.h>

#include "elfloader.h"

int TimerBase;
int MixGeneric;
int MixPowerUp;
void* PPCObject;

void main( void )
{
  PPCObject = ELFLoadObject( "devs:ahi.elf" );

  printf( "PPCObject: 0x%08x\n", PPCObject );

  ELFUnLoadObject( PPCObject );
}

APTR
AHIAllocVec( ULONG byteSize, ULONG requirements )
{
  return AllocVec( byteSize, requirements );
}

void
AHIFreeVec( APTR memoryBlock )
{
  FreeVec( memoryBlock );
}
