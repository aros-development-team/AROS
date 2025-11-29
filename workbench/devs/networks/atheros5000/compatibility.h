/*

Copyright (C) 2001-2024 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#ifndef COMPATIBILITY_H
#define COMPATIBILITY_H


#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/errors.h>
#include <exec/io.h>


#ifndef UPINT
#ifdef __AROS__
typedef IPTR UPINT;
typedef SIPTR PINT;
#else
typedef ULONG UPINT;
typedef LONG PINT;
#endif
#endif

#ifndef REG
#if defined(__mc68000) && !defined(__AROS__)
#define _REG(A, B) B __asm(#A)
#define REG(A, B) _REG(A, B)
#else
#define REG(A, B) B
#endif
#endif

#define _STR(A) #A
#define STR(A) _STR(A)

#ifndef BASE_REG
#define BASE_REG a6
#endif

#ifndef S2WERR_TOO_MANY_RETRIES
#define S2WERR_TOO_MANY_RETRIES S2WERR_TOO_MANY_RETIRES
#endif

#ifdef __amigaos4__

/* Macros to replace certain OS4 Exec functions, to hide differences from
   original AmigaOS */

#undef OpenLibrary
#ifdef EXPANSIONNAME
#define OpenLibrary(name, version) \
   ({ \
      struct Library *_library; \
      struct Interface *_interface = NULL; \
      const TEXT *interface_name; \
      _library = IExec->OpenLibrary((name), (version)); \
      if(_library != NULL) \
      { \
         if(name == expansion_name) \
            interface_name = "pci"; \
         else \
            interface_name = "main"; \
         _interface = GetInterface(_library, interface_name, 1, NULL); \
         if(_interface == NULL) \
            CloseLibrary(_library); \
      } \
      (struct Library *)_interface; \
   })
#else
#define OpenLibrary(name, version) \
   ({ \
      struct Library *_library; \
      struct Interface *_interface = NULL; \
      _library = IExec->OpenLibrary((name), (version)); \
      if(_library != NULL) \
      { \
         _interface = GetInterface(_library, "main", 1, NULL); \
         if(_interface == NULL) \
            CloseLibrary(_library); \
      } \
      (struct Library *)_interface; \
   })
#endif

#undef CloseLibrary
#define CloseLibrary(library) \
   ({ \
      struct Library *_library2 = (struct Library *)(library); \
      struct Interface *_interface2; \
      if(_library2->lib_Node.ln_Type != NT_LIBRARY) \
      { \
         _interface2 = (struct Interface *)(library); \
         _library2 = _interface2->Data.LibBase; \
         DropInterface(_interface2); \
      } \
      /*if(_library2->lib_Node.ln_Type == NT_LIBRARY)*/ \
      IExec->CloseLibrary(_library2); \
   })

#undef OpenDevice
#define OpenDevice(name, unit, request, flags) \
   ({ \
      struct IORequest *_request = (APTR)(request); \
      struct Library *_library; \
      struct Interface *_interface; \
      BYTE _error = IExec->OpenDevice((name), (unit), (_request), (flags)); \
      if(_error == 0) \
      { \
         _library = &(_request)->io_Device->dd_Library; \
         _interface = GetInterface(_library, "main", 1, NULL); \
         if(_interface != NULL) \
            _request->io_Device = (struct Device *)_interface; \
         else \
         { \
            CloseDevice(_library); \
            _error = IOERR_OPENFAIL; \
         } \
      } \
      _error; \
   })

#undef CloseDevice
#define CloseDevice(request) \
   ({ \
      struct IORequest *_request2 = (APTR)(request); \
      struct Library *_library2 = &(_request2)->io_Device->dd_Library; \
      struct Interface *_interface2; \
      if(_library2->lib_Node.ln_Type != NT_DEVICE) \
      { \
         _interface2 = (struct Interface *)(_library2); \
         _library2 = _interface2->Data.LibBase; \
         DropInterface(_interface2); \
         (_request2)->io_Device = (struct Device *)_library2; \
      } \
      IExec->CloseDevice(_request2); \
   })

#undef OpenResource
#define OpenResource(name) \
   ({ \
      APTR _resource; \
      struct Interface *_interface = NULL; \
      _resource = IExec->OpenResource((name)); \
      if(_resource != NULL) \
      { \
         _interface = GetInterface(_resource, "main", 1, NULL); \
      } \
      (APTR)_interface; \
   })

#undef CachePreDMA
#define CachePreDMA(address, length, flags) \
   ({ \
      struct DMAEntry _dma_entry = {0}; \
      if(StartDMA((address), *(length), (flags)) == 1) \
         GetDMAList((address), *(length), (flags), &_dma_entry); \
      *(length) = _dma_entry.BlockLength; \
      (APTR)((ULONG)_dma_entry.PhysicalAddress | (ULONG)address & 0xfff); \
   })

#undef CachePostDMA
#define CachePostDMA(address, length, flags) \
   EndDMA(address, *(length), flags);

/* Remap OS4 interface pointers to library pointer fields, to avoid the
   requirement for OS4-specific fields in our base structure */

#define IExec ((struct ExecIFace *)SysBase)
#define IIntuition ((struct IntuitionIFace *)IntuitionBase)
#define IUtility ((struct UtilityIFace *)UtilityBase)
#define IDataTypes ((struct DataTypesIFace *)DataTypesBase)
#define IGraphics ((struct GraphicsIFace *)GfxBase)
#define IDOS ((struct DOSIFace *)DOSBase)
#define IPCI ((struct PCIIFace *)ExpansionBase)
#define IPCCard ((struct PCCardIFace *)PCCardBase)
#define ICard ((struct CardIFace *)CardResource)
#define ITimer ((struct TimerIFace *)TimerBase)

#else

#define DeleteLibrary(library) \
   ({ \
      UWORD neg_size = ((struct Library *)base)->lib_NegSize; \
      UWORD pos_size = ((struct Library *)base)->lib_PosSize; \
      FreeMem((UBYTE *)base - neg_size, pos_size + neg_size); \
   })

#endif


#endif
