/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$
 */

#ifndef ABSTRACTICON_H
#define ABSTRACTICON_H

#include <libraries/mui.h>
#include "presentation.h"

#define AIA_BASE           TAG_USER+3400

#define AIA_Script                  AIA_BASE+1
#define AIA_Pure                    AIA_BASE+2
#define AIA_Archived                AIA_BASE+3
#define AIA_Readable                AIA_BASE+4
#define AIA_Writeable               AIA_BASE+5
#define AIA_Executable              AIA_BASE+6
#define AIA_Deleteable              AIA_BASE+7
#define AIA_Comment                 AIA_BASE+8

struct AbstractIconClassData
{
    BOOL script;
    BOOL pure;
    BOOL archived;
    BOOL readable;
    BOOL writeable;
    BOOL executable;
    BOOL deleteable;
    UBYTE *comment;
};

struct __dummyAbstractIconData__
{
    struct MUI_NotifyData mnd;
    struct MUI_AreaData mad;
    struct PresentationClassData pcd;
	struct AbstractIconClassData aicd;
};

#define abstractIconData(obj) (&(((struct __dummyAbstractIconData__ *)(obj))->aicd))

#define _comment(obj)     (abstractIconData(obj)->comment)
#define _script(obj)      (abstractIconData(obj)->script)
#define _pure(obj)        (abstractIconData(obj)->pure)
#define _archived(obj)    (abstractIconData(obj)->archived)
#define _readable(obj)    (abstractIconData(obj)->readable)
#define _writeable(obj)   (abstractIconData(obj)->writeable)
#define _executable(obj)  (abstractIconData(obj)->executable)
#define _deleteable(obj)  (abstractIconData(obj)->deleteable)

#endif


