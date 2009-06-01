#ifndef LIBRARIES_H
#define LIBRARIES_H

#include <intuition/intuitionbase.h>
#include <graphics/gfxbase.h>

extern struct Library *Libraries[];
extern struct ExecBase *SysBase;

#define DOSBase         ( Libraries[0] )
#define IntuitionBase   (( struct IntuitionBase * )Libraries[1] )
#define GfxBase         (( struct GfxBase * )Libraries[2] )
#define IconBase        ( Libraries[3] )
#define CxBase          ( Libraries[4] )
#define GarshnelibBase  ( Libraries[5] )
#define UtilityBase     ( Libraries[6] )

LONG OpenLibraries( VOID );
VOID CloseLibraries( VOID );

#endif /* LIBRARIES_H */
