/*
**  Q&D Protracker DataType
**
**      Written by Martin Blom, based on the AIFF DataType by
**  Olaf `Olsen' Barthel <olsen@sourcery.han.de>
**  Public domain
**
*/

#include "protracker.datatype_rev.h"
#include "Data.h"

  // Mandatory library routines

struct ClassBase * LIBENT LibInit(REG(a0) BPTR LibSegment,REG(d0) struct ClassBase *ClassBase,REG(a6) struct ExecBase *ExecBase);
struct ClassBase * LIBENT LibOpen(REG(a6) struct ClassBase *Base);
BPTR LIBENT               LibExpunge(REG(a6) struct ClassBase *Base);
BPTR LIBENT               LibClose(REG(a6) struct ClassBase *Base);

  // In RomTag.a

LONG                      LibNull(VOID);

  // The only user-callable routine

Class * LIBENT            GetClassEngine(REG(a6) struct ClassBase *ClassBase);

  // In Class.c

Object * LIBENT           ClassDispatch(REG(a0) Class *class,REG(a2) Object *object,REG(a1) Msg msg);

  // Vector initialization table

APTR LibVectors[] =
{
  LibOpen,
  LibClose,
  LibExpunge,
  LibNull,

  GetClassEngine,

  (APTR)-1
};

  // Library initialization table for MakeLibrary()

struct { ULONG DataSize; APTR Table; APTR Data; struct ClassBase * (*Init)(); } __aligned LibInitTab =
{
  sizeof(struct ClassBase),
  LibVectors,
  NULL,
  LibInit
};

  /* LibraryCleanup(struct ClassBase *ClassBase):
   *
   *  Closes all the libraries opened by LibrarySetup().
   */

STATIC VOID
LibraryCleanup(struct ClassBase *ClassBase)
{
  if(AHIDevice != -1)
  {
    CloseDevice((struct IORequest *) AHIio);
    AHIDevice = -1;
  }

  DeleteIORequest((struct IORequest *) AHIio);
  AHIio = NULL;

  DeleteMsgPort(AHImp);
  AHImp = NULL;

  CloseLibrary(SuperClassBase);
  SuperClassBase = NULL;

  CloseLibrary(DataTypesBase);
  DataTypesBase = NULL;

  CloseLibrary(UtilityBase);
  UtilityBase = NULL;

  CloseLibrary(IntuitionBase);
  IntuitionBase = NULL;

  CloseLibrary(DOSBase);
  DOSBase = NULL;
}

  /* LibrarySetup(struct ClassBase *ClassBase):
   *
   *  Sets up all the libraries this class requires to work.
   */

STATIC BOOL
LibrarySetup(struct ClassBase *ClassBase)
{
  if(DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",39))
  {
    if(IntuitionBase = OpenLibrary("intuition.library",39))
    {
      if(UtilityBase = OpenLibrary("utility.library",39))
      {
        if(DataTypesBase = OpenLibrary("datatypes.library",39))
        {
          if(SuperClassBase = OpenLibrary("datatypes/sound.datatype",39))
          {
            if(AHImp=CreateMsgPort())
            {
              if(AHIio=(struct AHIRequest *)CreateIORequest(
                  AHImp,sizeof(struct AHIRequest)))
              {
                AHIio->ahir_Version=2;
                if(!(AHIDevice=OpenDevice(AHINAME,AHI_NO_UNIT,
                    (struct IORequest *) AHIio,NULL)))
                {

                  AHIBase = AHIio->ahir_Std.io_Device;
                  return(TRUE);
                }
              }
            }
          }
        }
      }
    }
  }

  LibraryCleanup(ClassBase);

  return(FALSE);
}

  /* LibInit():
   *
   *  Initialize the library.
   */

struct ClassBase * LIBENT
LibInit(REG(a0) BPTR LibrarySegment,REG(d0) struct ClassBase *ClassBase,REG(a6) struct ExecBase *ExecBase)
{
    // Set up the header data; everything that doesn't get set
    // up here will have been set up by InitResident().

  ClassBase->LibNode.lib_Revision = REVISION;

    // Remember the segment pointer

  Segment = LibrarySegment;

    // Remember the exec library base pointer

  SysBase = ExecBase;

    // Initialize the shared data access semaphore

  InitSemaphore(&LockSemaphore);

  return(ClassBase);
}

  /* LibOpen(REG(a6) struct ClassBase *ClassBase):
   *
   *  Open the library, as called via OpenLibrary()
   */

struct ClassBase * LIBENT
LibOpen(REG(a6) struct ClassBase *ClassBase)
{
  struct SignalSemaphore *LocalSemaphore;

    // Prevent delayed expunge

  ClassBase->LibNode.lib_Flags &= ~LIBF_DELEXP;

    // We are going to modify data while in multitasking,
    // so watch out

  ObtainSemaphore(LocalSemaphore = &LockSemaphore);

    // Is this the first initialization?

  if(++ClassBase->LibNode.lib_OpenCnt == 1)
  {
      // Open libraries & classes

    if(!LibrarySetup(ClassBase))
      ClassBase = NULL;
    else
    {
        // Create a new class

      if(!(SoundClass = MakeClass(ClassBase->LibNode.lib_Node.ln_Name,SOUNDDTCLASS,NULL,NULL,NULL)))
      {
        LibraryCleanup(ClassBase);
        ClassBase = NULL;
      }
      else
      {

          // Link the class dispatcher into it
          // and keep a pointer to the library
          // base

        SoundClass->cl_Dispatcher.h_Entry   = (HOOKFUNC)ClassDispatch;
        SoundClass->cl_UserData             = (ULONG)ClassBase;

          // Make the class publicly available

        AddClass(SoundClass);
      }
    }
  }

    // Release the lock

  ReleaseSemaphore(LocalSemaphore);

    // Return the library base, if any

  return(ClassBase);
}

  /* LibExpunge(REG(a6) struct ClassBase *ClassBase):
   *
   *  Expunge the library, remove it from memory
   */

BPTR LIBENT
LibExpunge(REG(a6) struct ClassBase *ClassBase)
{
  BPTR LibrarySegment;

  LibrarySegment = NULL;

    // No more callers have the library open?

  if(ClassBase->LibNode.lib_OpenCnt == 0)
  {
      // Remember the segment pointer, so it can be unloaded

    LibrarySegment = Segment;

      // Remove the library from the public list

    Remove(ClassBase);

      // Free the vector table and the library data

    FreeMem((BYTE *)ClassBase - ClassBase->LibNode.lib_NegSize,ClassBase->LibNode.lib_NegSize + ClassBase->LibNode.lib_PosSize);
  }
  else
  {
      // Expunge it later

    ClassBase->LibNode.lib_Flags |= LIBF_DELEXP;
  }

    // Return the segment pointer, if any

  return(LibrarySegment);
}

  /* LibClose(REG(a6) struct ClassBase *ClassBase):
   *
   *  Close the library, as called by CloseLibrary()
   */

BPTR LIBENT
LibClose(REG(a6) struct ClassBase *ClassBase)
{
  BPTR LibrarySegment;

  LibrarySegment = NULL;

    // Decrement usage count

  if(ClassBase->LibNode.lib_OpenCnt > 0)
  {
      // No more users?

    if(--ClassBase->LibNode.lib_OpenCnt == 0)
    {
        // We are going to modify shared data,
        // so watch out

      ObtainSemaphore(&LockSemaphore);

        // Clean up...

      if(SoundClass)
      {
        RemoveClass(SoundClass);
        FreeClass(SoundClass);
        SoundClass = NULL;
      }

      LibraryCleanup(ClassBase);

        // Release the lock

      ReleaseSemaphore(&LockSemaphore);

        // Can we remove ourselves?

      if(ClassBase->LibNode.lib_Flags & LIBF_DELEXP)
        LibrarySegment = LibExpunge(ClassBase);
    }
  }

  return(LibrarySegment);
}

  /* GetClassEngine(REG(a6) struct ClassBase *ClassBase):
   *
   *  Get access to the class this library implements.
   */

Class * LIBENT
GetClassEngine(REG(a6) struct ClassBase *ClassBase)
{
  Class *class;

    // Access shared data

  ObtainSemaphoreShared(&LockSemaphore);

    // Remember the class pointer

  class = SoundClass;

    // Release the lock

  ReleaseSemaphore(&LockSemaphore);

    // Return the pointer

  return(class);
}
