/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: kernel.h 37932 2011-03-30 13:19:37Z sonic $

    Desc: Structures and TagItem for debug.library
    Lang: english
*/

#ifndef LIBRARIES_DEBUG_H
#define LIBRARIES_DEBUG_H

#include <exec/lists.h>
#include <exec/nodes.h>
#include <dos/elf.h>

/* Tags for DecodeLocation() */
#define DL_Dummy		(TAG_USER + 0x03e00000)
#define DL_ModuleName		(DL_Dummy + 1)
#define DL_SegmentName		(DL_Dummy + 2)
#define DL_SegmentPointer	(DL_Dummy + 3)
#define DL_SegmentNumber	(DL_Dummy + 4)
#define DL_SegmentStart		(DL_Dummy + 5)
#define DL_SegmentEnd		(DL_Dummy + 6)
#define DL_SymbolName		(DL_Dummy + 7)
#define DL_SymbolStart		(DL_Dummy + 8)
#define DL_SymbolEnd		(DL_Dummy + 9)

/* Known debug information types */
#define DEBUG_NONE              0
#define DEBUG_ELF               1
#define DEBUG_PARTHENOPE        2

/* ELF module debug information */
struct ELF_DebugInfo
{
    struct elfheader *eh;
    struct sheader *sh;
};

/* Kickstart module debug information (pointed to by KRN_DebugInfo ti_Data) */
struct ELF_ModuleInfo
{
    struct ELF_ModuleInfo *Next; /* Pointer to next module in list */
    const char		  *Name; /* Pointer to module name	   */
    unsigned short	   Type; /* DEBUG_ELF, for convenience	   */
    struct elfheader	  *eh;	 /* ELF file header		   */
    struct sheader	  *sh;	 /* ELF section header		   */
};

/* Parthenope module debug information (pointed to by KRN_DebugInfo ti_Data)
 *
 * (This structure has the same layout as Parthenope's "module_t")
 */
struct Parthenope_ModuleInfo {
    struct MinNode      m_node;
    CONST_STRPTR        m_name;
    CONST_STRPTR        m_str;
    ULONG               m_lowest;
    ULONG               m_highest;
    struct MinList      m_symbols;
};

struct Parthenope_Symbol {
    struct MinNode      s_node;
    CONST_STRPTR        s_name;
    ULONG               s_lowest;
    ULONG               s_highest;
};

#endif
