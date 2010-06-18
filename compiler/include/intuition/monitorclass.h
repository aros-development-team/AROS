#ifndef INTUITION_MONITORCLASS_H
#define INTUITION_MONITORCLASS_H

/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: Headerfile for BOOPSI monitorclass.
    Lang: english
*/

#include <exec/types.h>

/* Length of array returned by MA_PixelFormats */
#define MONITOR_MAXPIXELFORMATS 14

/* Attributes */
#define MA_Dummy		(TAG_USER)
#define MA_MonitorName		(MA_Dummy + 1)	/* [..G] STRPTR    Monitor name					      	    */
#define MA_Manufacturer		(MA_Dummy + 2)	/* [..G] STRPTR    Hardware manufacturer string			      	    */
#define MA_ManufacturerID	(MA_Dummy + 3)	/* [..G] ULONG							      	    */
#define MA_ProductID		(MA_Dummy + 4)	/* [..G] ULONG							      	    */
#define MA_MemorySize		(MA_Dummy + 5)	/* [..G] ULONG     Video card memory size				    */
#define MA_PixelFormats		(MA_Dummy + 6)  /* [..G] ULONG *   Pixelformat support flags				    */
#define MA_TopLeftMonitor	(MA_Dummy + 7)	/* [.SG] Object *  Monitor placed in a position relative to the current one */
#define MA_TopMiddleMonitor	(MA_Dummy + 8)  /* [.SG] Object *							    */
#define MA_TopRightMonitor	(MA_Dummy + 9)  /* [.SG] Object *							    */
#define MA_MiddleLeftMonitor	(MA_Dummy + 10) /* [.SG] Object *							    */
#define MA_MiddleRightMonitor	(MA_Dummy + 11) /* [.SG] Object *							    */
#define MA_BottomLeftMonitor	(MA_Dummy + 12) /* [.SG] Object *							    */
#define MA_BottomMiddleMonitor	(MA_Dummy + 13) /* [.SG] Object *							    */
#define MA_BottomRightMonitor	(MA_Dummy + 14) /* [.SG] Object *							    */
#define MA_GammaControl		(MA_Dummy + 15) /* [..G] BOOL      Whether gamma control is supported			    */
#define MA_PointerType		(MA_Dummy + 16) /* [..G] ULONG     Supported pointer types				    */
#define MA_DriverName		(MA_Dummy + 17) /* [..G] STRPTR    Driver name						    */
#define MA_MemoryClock		(MA_Dummy + 18) /* [..G] ULONG     Video memory clock in Hz, 0 if unknown		    */

/* Pointer type flags */
#define PointerType_3Plus1 0x0001 /* color 0 transparent, 1-3 visible				 */
#define PointerType_2Plus1 0x0002 /* color 0 transparent, 2-3 visible, 1 undefined/clear/inverse */
#define PointerType_ARGB   0x0004 /* Direct color alpha-blended bitmap pointer			 */

/* Methods */
#define MM_GetRootBitMap	 0x401	/* Reserved								  */
#define MM_Query3DSupport	 0x402	/* Ask for 3D acceleration support for given pixelformat	       	  */
#define MM_GetDefaultGammaTables 0x403	/* Get default gamma correction table				       	  */
#define MM_GetDefaultPixelFormat 0x404	/* Ask for preferred pixelformat for given depth (-1 = unsupported depth) */
#define MM_GetPointerBounds	 0x405	/* Ask for maximum supported mouse pointer size			       	  */
#define MM_RunBlanker		 0x406	/* Start screensaver for this monitor				       	  */
#define MM_EnterPowerSaveMode	 0x407	/* Start power saving mode					       	  */
#define MM_ExitBlanker		 0x408	/* Stop screensaver or power saving mode			       	  */

struct msGetRootBitMap
{
    ULONG  MethodID;
    ULONG  PixelFormat;
    struct BitMap **Store;
};

struct msQuery3DSupport
{
    ULONG  MethodID;
    ULONG  PixelFormat;
    ULONG *Store;
};

#define MSQUERY3D_UNKNOWN  (0) /* Unsupported pixelformat or other error */
#define MSQUERY3D_NODRIVER (1) /* No 3D support available		 */
#define MSQUERY3D_SWDRIVER (2) /* Software 3D support available		 */
#define MSQUERY3D_HWDRIVER (3) /* Hardware accelerated 3D available	 */

struct msGetDefaultGammaTables
{
    ULONG  MethodID;
    UBYTE *Red;		/* Optional pointers to 256-byte arrays to fill in */
    UBYTE *Green;
    UBYTE *Blue;
};

struct msGetDefaultPixelFormat
{
    ULONG  MethodID;
    ULONG  Depth;
    ULONG *Store;
};

struct msGetPointerBounds
{
    ULONG MethodID;
    ULONG PointerType;
    ULONG *Width;
    ULONG *Height;
};

#endif
