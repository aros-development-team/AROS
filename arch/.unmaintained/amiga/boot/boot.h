/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amiga bootloader -- support structures
    Lang: C
*/

/********************************************************************
 * Structures for InternalLoadSeg()
 *******************************************************************/
struct ilsMemList
{
    struct MinList iml_List;
    ULONG  iml_Num;		/* total number of nodes on the list */
    ULONG  iml_NewNum;		/* total number of new nodes since this was reset */
};

struct ilsMemNode
{
    struct MinNode imn_Node;
    APTR           imn_Addr;	/* address of memory region */
    ULONG          imn_Size;	/* size of memory region */
};

/********************************************************************
 * Structures for config options
 *******************************************************************/

/* Anchor for all config options. */
struct BootConfig
{
    struct List bc_Modules;	/* will contain 'ModNode's */
    ULONG       bc_Num;		/* number of nodes on this List */
};

/* One ModNode for every MODULE found in the config file. */
struct ModNode
{
    struct Node mn_Node;
    struct List mn_FuncList;	/* will contain 'FuncNode's */
};

/* One FuncNode for every FUNCTION found in the config file. */
struct FuncNode
{
    struct Node fn_Node;
    UWORD       fn_Slot;	/* slot = function / -6 */
    BOOL        fn_Status;	/* on or off */
};

/********************************************************************
 * Structures for main() module handling and vector processing/loading
 *******************************************************************/
struct ModuleList
{
    struct MinList ml_List;
    ULONG  ml_Num;
};

struct Module
{
    struct MinNode   m_Node;
    BPTR             m_SegList;		/* pointer to a module's seglist */
    struct Resident *m_Resident;	/* pointer to a module's Resident structure */
};
