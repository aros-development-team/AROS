/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Amiga bootloader -- support structures
    Lang: C
*/

struct ilsMemList
{
    struct MinList iml_List;
    ULONG iml_Num;		/* total number of nodes on the list */
    ULONG iml_NewNum;		/* total number of new nodes since this was reset */
};

struct ilsMemNode
{
    struct MinNode imn_Node;
    APTR imn_Addr;		/* address of memory region */
    ULONG imn_Size;		/* size of memory region */
};

struct FileList
{
    struct List fl_List;	/* ln_Name field points to filename */
    ULONG fl_Num;		/* number of nodes on this List */
};

struct ModuleList
{
    struct MinList ml_List;
    ULONG ml_Num;
};

struct Module
{
    struct MinNode m_Node;
    BPTR m_SegList;		/* pointer to a module's seglist */
    struct Resident *m_Resident;/* pointer to a module's Resident structure */
};
