#ifndef MODULE_H
#define MODULE_H

/*
    Copyright (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang:
*/

#ifndef TOOLLIB_TOOLLIB_H
#include <toollib/toollib.h>
#endif

enum ModuleTypes {
    MT_LINKLIB,		/* A linker library */
    MT_LIBRARY,		/* A standard Amiga .library */
    MT_DEVICE,		/* A standard Amiga .device */
    MT_RESOURCE,	/* A standard Amiga .resource */
    MT_FILESYS,		/* A new AROS .filesystem */
    MT_HANDLER,		/* A new AROS -handler */
    MT_BOOPSI,		/* A BOOPSI class */
    MT_OOP,		/* An oop.library class */
    MT_GADGET,		/* An intuition.library gadget */
    MT_DATATYPE,	/* A datatypes.library datatype */
    MT_IMAGE,		/* An intuition.library image */
    MT_HIDD,		/* A hidd.library HIDD */
    MT_FONTENGINE,	/* A diskfont.library .engine */
    MT_CUSTOM		/* Something else */
};

/*  Options for modules */
#define MO_NOEXPUNGE	0x0001  /* Never try to expunge this module */
#define MO_ROMONLY	0x0002	/* This module must live in ROM */
#define MO_HASRT	0x0004	/* This module has restracking support */
#define MO_NOAUTOINIT	0x0008	/* Don't use autoinit when generating */
#define MO_NORESIDENT	0x0010	/* Don't create the resident structure */
#define MO_NOEXTENSION	0x0020	/* Don't put the extension on */
#define MO_CLASSLIB	0x0040	/* Class wants to be in a library */

struct ModuleConfig
{
    /*
	Name of the module we are investigating. Eg exec.library.
	Basename gives the same information, but with no extension
    */
    char    *name, *basename;

    /* The version and revision of the module - if applicable. */
    int	    version, revision;

    /* The copyright information, or idstring for the Resident struct */
    char    *copyright;

    /* What type of module are we? */
    enum ModuleTypes type;

    /* XXX - Options... */
    int	    option;

    /*
	All the modules have some kind of global data structure
	associated with them. This is the name of that structure.
    */
    char    *base, *basetype, *basetypeptr;

    /* XXX - Stuff to help create the include files? */
    char    *define, *include;

    /*
	The resident type and file name extension. Normally we would
	get these from the type, but there may be a few pedantic cases
	where they want to do something interesting.
    */
    char    *nodetype, *extension;
};

/*
    ModuleData: Defines the prototypes, code and docs for an entire
    module. Nice eh?
*/
struct ModuleData
{
    /* This is a reference to the module configuration */
    struct  ModuleConfig *md_Config;
    void *	md_MemoryLink;

    /* Flags */
    int		md_Flags;

    /* This is the data required to construct the output */
    List	md_Data;

    /* The maximum number of vectors */
    int		md_Vectors;
};

/* These allow us to only do a certain number of operations at a time */
#define MD_ScanHeader	0x0001
#define MD_ScanCode	0x0002
#define MD_ScanDocs	0x0004
#define MD_DocsOnly	0x0008

struct MDNode
{
    Node	n_Link;
    int		n_Type;
    char    *	n_FileName;
    int		n_LineNo;
};

#define T_Function	1
#define T_CPP		2
#define T_Raw		3

/*
    This is data relating to either CPP commands, or raw lines
*/
struct CPPData
{
    struct MDNode   cpd_Link;
#define cpd_Data    cpd_Link.n_Link.name
};

/*
    This is the data relating to functions.
*/
struct FunctionData
{
    struct MDNode	fd_Link;
#define fd_Name		fd_Link.n_Link.name

    char		*fd_Type;

    struct ArgData	*fd_Args;
    int			fd_NumArgs;
    int			fd_Offset;
    int			fd_Options;

    char		*fd_Code;
    char		*fd_AutoDoc;
    int			 fd_CodeLine;
};

#define	FO_UseBase	0x0001	/* This function requires the base */
#define FO_HasQuad	0x0002	/* This function has 64 bits args */
#define FO_Hidden	0x0004	/* This function is not visible */
#define FO_NoOffset	0x0008	/* This function is not in the jumptable */

#define FO_Internal	0x000C	/* This is an internal function */

struct ArgData
{
    struct ArgData	*ad_Next;

    char		*ad_Type;
    char		*ad_Name;
    char		*ad_Reg1;
    char		*ad_Reg2;
};

/* Prototypes */
struct ModuleConfig *ReadModuleConfig PARAMS((char *file));
void FreeModuleConfig PARAMS((struct ModuleConfig *mc));

struct ModuleData *ModuleStart PARAMS((struct ModuleConfig *mc, int flags));
void ModuleNext PARAMS((struct ModuleData *md, char *file));
void ModuleEnd PARAMS((struct ModuleData *md));

#endif /* MODULE_H */
