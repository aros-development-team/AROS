
/*
** create.c
**
** (c) 1998-2011 Guido Mersmann
*/

/*************************************************************************/

#define SOURCENAME "create.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/dos.h>
#include <proto/icon.h>

#include "cache.h"
#include "create.h"
#include "header.h"
#include "locale_strings.h"
#include "macros.h"
#include "readargs.h"
#include "sprintf.h"
#include "version.h"

/*************************************************************************/

static void DD_WriteDosDriverData( struct PartitionEntry *pe );
static void DD_CreateDosDriverIcon( struct PartitionEntry *pe, STRPTR name );

/*************************************************************************/

/* /// Create_MountFile()
**
*/

/*************************************************************************/

ULONG Create_MountFile( STRPTR name, struct PartitionEntry *pei )
{
struct PartitionEntry *pe;
BPTR handle;
ULONG i;
ULONG args[2];
    if( (handle = Open( name, MODE_NEWFILE)) ) {

        Cache_Init( handle );

		Cache_PutString("/*\n** Mount file automatically created by GiggleDisk\n**\n** GiggleDisk (c)2005-2007 Guido Mersmann\n**\n*/\n\n");

         i = 0;
         for( pe = (APTR) partitionlist.lh_Head ; pe->Node.ln_Succ ; pe = (APTR) pe->Node.ln_Succ ) {
			debug( APPLICATIONNAME ": CreateType: %08lx\n", pe->ENV.de_DosType);
            if( (!pei || pei == pe) && pe->ENV.de_DosType ) {

                if( pe->DriveName[0] ) {
                    args[0] = (ULONG) &pe->DriveName[0];
                    Cache_Printf("%s:\n", args);
                } else {
                    args[0] = readargs_array[ARG_PREFIX];
                    args[1] = i++;
                    Cache_Printf("%s%ld:\n", args);
                }

                DD_WriteDosDriverData( pe );

                Cache_PutString("#\n\n");
            }
        }

        Cache_Close();

        Close( handle);
    }
return(0L);
}
/* \\\ */
/* /// Create_DosDriver()
**
*/

/*************************************************************************/

ULONG Create_DosDriver( STRPTR dir, struct PartitionEntry *pei )
{
struct PartitionEntry *pe;
BPTR handle, lock;
ULONG i, result;
char name[20];
ULONG args[2];

    result = MSG_ERROR_UnableToLocateDrawer;
    if( (lock = Lock( dir, ACCESS_READ )) ) {
        lock = CurrentDir( lock );

        i = 0;
        for( pe = (APTR) partitionlist.lh_Head ; pe->Node.ln_Succ ; pe = (APTR) pe->Node.ln_Succ ) {

            if( (!pei || pei == pe) && pe->ENV.de_DosType ) {

                if( pe->DriveName[0] ) {
                    String_Copy( &pe->DriveName[0], name );

                } else {
                    args[0] = readargs_array[ARG_PREFIX];
                    args[1] = i++;
                    SPrintf( "%s%ld", &name[0], args);
                }
                if( (handle = Open( name, MODE_NEWFILE)) ) {

                    Cache_Init( handle );

                    Cache_PutString("/*\n** DosDriver automatically created by GiggleDisk\n**\n** GiggleDisk (c)2005 Guido Mersmann\n**\n*/\n\n");

                    DD_WriteDosDriverData( pe );
                    Cache_Close();

                    Close( handle);

                    DD_CreateDosDriverIcon( pe, name );
                }
            }
        }
        lock = CurrentDir( lock );
        UnLock( lock );
        result = MSG_ERROR_NoError;
    }
return( result );
}
/* \\\ */
/* /// DD_WriteDosDriverData()
**
*/

/*************************************************************************/

static void DD_WriteDosDriverData( struct PartitionEntry *pe )
{
ULONG args[25];
ULONG *arg = args;
            *arg++ = (ULONG) &pe->FileSysName[0];
            *arg++ = pe->Device;
            *arg++ = pe->Unit;
            *arg++ = pe->ENV.de_SizeBlock * 4;  /* block size in longs */
            *arg++ = pe->ENV.de_Surfaces;
            *arg++ = pe->ENV.de_SectorPerBlock;
            *arg++ = pe->ENV.de_BlocksPerTrack;
            *arg++ = pe->ENV.de_Reserved;
            *arg++ = pe->ENV.de_PreAlloc;
            *arg++ = pe->ENV.de_Interleave;
            *arg++ = pe->ENV.de_MaxTransfer;
            *arg++ = pe->ENV.de_Mask;
            *arg++ = pe->ENV.de_LowCyl;
            *arg++ = pe->ENV.de_HighCyl;
            *arg++ = pe->ENV.de_NumBuffers;
            *arg++ = pe->ENV.de_BufMemType;
            *arg++ = pe->StackSize;
            *arg++ = pe->ENV.de_BootPri;
            *arg++ = pe->ENV.de_DosType;
            *arg++ =         ((pe->Flags & PBFF_NOMOUNT) ? 0 : 1);
            *arg++ = (ULONG) ((pe->Flags & PBFF_BOOTABLE) ? "TRUE" : "FALSE");

            Cache_Printf("FileSystem       = %s\n"
                         "Device           = %s\n"
                         "Unit             = %ld\n"
                         "BlockSize        = %ld\n"
                         "Surfaces         = %ld\n"
                         "SectorsPerBlock  = %ld\n"
                         "BlocksPerTrack   = %ld\n"
                         "Reserved         = %ld\n"
                         "PreAlloc         = %ld\n"
                         "Interleave       = %ld\n"
                         "MaxTransfer      = 0x%08lx\n"
                         "Mask             = 0x%08lx\n"
                         "LowCyl           = %ld\n"
                         "HighCyl          = %ld\n"
                         "Buffers          = %ld\n"
                         "BufMemType       = %ld\n"
                         "StackSize        = %ld\n"
                         "Priority         = %ld\n"
                         "GlobVec          = -1\n"
                         "DosType          = 0x%08lx\n"
                         "Activate         = 1\n"
#ifndef __MORPHOS__
                         "Mount            = %ld\n"
#else
                         "/* Mount            = %ld  - MorphOS is not supporting this value */\n"
#endif
                         "/* Bootable      = %s */\n", args);
}
/* \\\ */
/* /// DD_CreateDosDriverIcon()
**
*/

/*************************************************************************/

static void DD_CreateDosDriverIcon( struct PartitionEntry *pe, STRPTR name )
{
#define MAXTTSTRINGSIZE 0x40
struct DiskObject *dob;
ULONG   orgstack, orgcurrentx, orgcurrenty;
STRPTR  orgdefaulttool;
STRPTR *orgtooltypes;
STRPTR  newtooltypes[4];
char newdevice[ MAXTTSTRINGSIZE ];
char newunit[ MAXTTSTRINGSIZE ];

	if( ( dob = GetDefDiskObject( WBPROJECT ) ) ) {

/* prepare new contents */

        SPrintf("DEVICE=%s", newdevice ,&pe->Device);
        SPrintf("UNIT=%ld" , newunit   ,&pe->Unit);
        newtooltypes[0] = newdevice;
        newtooltypes[1] = newunit;
        newtooltypes[2] = "ACTIVATE=1";
        newtooltypes[3] = NULL;

/* save original icon contents */

        orgdefaulttool      = dob->do_DefaultTool;
        orgtooltypes        = dob->do_ToolTypes;
        orgstack            = dob->do_StackSize;
        orgcurrentx         = dob->do_CurrentX;
        orgcurrenty         = dob->do_CurrentY;


/* replace with new contents */

        dob->do_DefaultTool = "c:Mount";
        dob->do_ToolTypes   = newtooltypes;
        dob->do_StackSize   = 8192;
        dob->do_CurrentX    = NO_ICON_POSITION;
        dob->do_CurrentY    = NO_ICON_POSITION;
/* write icon to disk */

        PutDiskObject( name, dob );

/* restore original icon contents */

        dob->do_DefaultTool = orgdefaulttool;
        dob->do_ToolTypes   = orgtooltypes;
        dob->do_StackSize   = orgstack;
        dob->do_CurrentX    = orgcurrentx;
        dob->do_CurrentY    = orgcurrenty;

/* free icon */
        FreeDiskObject( dob );
    }

}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      








/* \\\ */


