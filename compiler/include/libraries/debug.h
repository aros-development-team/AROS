/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

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
#define DL_FirstSegment		(DL_Dummy + 10)

/* Known debug information types */
#define DEBUG_NONE              0
#define DEBUG_ELF               1
#define DEBUG_PARTHENOPE        2
#define DEBUG_HUNK              3

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

/* Structure received as message of EnumerateSymbols hook */
struct SymbolInfo
{
    LONG            si_Size;        /* Size of the structure */
    CONST_STRPTR    si_ModuleName;
    CONST_STRPTR    si_SymbolName;
    APTR            si_SymbolStart;
    APTR            si_SymbolEnd;
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

/* HUNK module debug information */
struct HUNK_DebugInfo
{
    APTR dummy;
};

#endif
