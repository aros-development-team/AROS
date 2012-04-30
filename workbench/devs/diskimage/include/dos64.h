#ifndef __DOS64_H
   #define __DOS64_H

   #ifdef __amigaos4__
      #define ChangeFilePosition64(file, position, mode) (IDOS->ChangeFilePosition)(file, position, mode)
      #define ChangeFileSize64(file, position, mode)     (IDOS->ChangeFileSize)(file, position, mode)
      #define GetFilePosition64(file)                    (IDOS->GetFilePosition)(file)
      #define GetFileSize64(file)                        (IDOS->GetFileSize)(file)
   #else
      #ifndef EXEC_TYPES_H
         #incude <exec/types.h>
      #endif

      LONG ChangeFilePosition64(BPTR file, QUAD position, LONG mode);
      LONG ChangeFileSize64(BPTR file, QUAD position, LONG mode);
      QUAD GetFilePosition64(BPTR file);
      QUAD GetFileSize64(BPTR file);
   #endif
#endif
