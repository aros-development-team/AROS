
#ifndef ICONOBSERVERCLASS_H
#    define ICONOBSERVERCLASS_H

#    include "observer.h"

#    define IO_Base TAG_USER+2100

#    define IOM_Execute   IO_Base+1
#    define IOA_Selected  IO_Base+2
#    define IOA_Name      IO_Base+3
#    define IOA_Directory IO_Base+4
/*
   when this is changed, it updates the presentation's copy, also installs a
   notify on the presentation's copy 
 */
#    define IOA_Comment      IO_Base+5
#    define IOA_Script       IO_Base+6
#    define IOA_Pure         IO_Base+7
#    define IOA_Archived     IO_Base+8
#    define IOA_Readable     IO_Base+9
#    define IOA_Writeable    IO_Base+10
#    define IOA_Executable   IO_Base+11
#    define IOA_Deleteable   IO_Base+12

struct IconObserverClassData
{
    BOOL            selected;
    UBYTE          *name,
                   *directory;
    UBYTE          *comment;
    BOOL            script,
                    pure,
                    archived,
                    readable,
                    writeable,
                    executable,
                    deleteable;
};

struct __dummyIconObsData__
{
    struct MUI_NotifyData mnd;
    struct ObserverClassData ocd;
    struct IconObserverClassData icd;
};

#    define iconObsData(obj) (&(((struct __dummyIconObsData__ *)(obj))->icd))

#    define _name(obj)         (iconObsData(obj)->name)
#    define _directory(obj)    (iconObsData(obj)->directory)
#    define _iocomment(obj)    (iconObsData(obj)->comment)

#endif
