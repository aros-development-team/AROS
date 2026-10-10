/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    netprefs_module.h - Protocol module interface for the Network prefs editor.

    Each protocol-address plugin (.netprefs library) exports ModuleInit() at
    vector 5.  ModuleInit receives a pointer to the NetPrefsBase library and
    registers a NetPrefsModule whose Startup callback will later create the
    MUI window class and call RegisterProtoHandler().
*/

#ifndef _NETPREFS_MODULE_H_
#define _NETPREFS_MODULE_H_

#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/types.h>
#include <stdio.h>

struct NetPrefsBase;
struct ProtocolAddress;

/* Callback: called after all modules are loaded (create MUI classes, etc.) */
typedef void (*NETPREFS_STARTUP)(struct NetPrefsBase *);

/* Callback: write protocol config tokens to an interfaces file */
typedef void (*NETPREFS_WRITETOKENS)(FILE *f, struct ProtocolAddress *pa);

/*
 * Callback: claim and parse one token off an interface line.  If the token
 * belongs to this protocol, find (or, on its first token, allocate and AddTail)
 * this protocol's node on protoList - tagged with the plugin's assigned id in
 * ln_Type - parse the token's value into it, and return the node.  Return NULL
 * if the token is not one of this protocol's.
 */
typedef struct Node *(*NETPREFS_READTOKENS)(struct List *protoList,
                                            CONST_STRPTR token, UBYTE id);

/* Callback: format a short list-column string for one protocol object */
typedef void (*NETPREFS_DISPLAY)(struct ProtocolAddress *pa,
                                 STRPTR buf, ULONG buflen);

/*
 * NetPrefsModule - registered by each plugin during ModuleInit().
 * Startup is called after every plugin has been loaded so that modules
 * can safely use GetBase() to retrieve shared classes.
 */
struct NetPrefsModule
{
    struct Node             npm_Node;       /* ln_Name = module name, ln_Pri = sort order */
    NETPREFS_STARTUP        npm_Startup;    /* called after all modules loaded            */
    NETPREFS_STARTUP        npm_Shutdown;   /* called on app exit                         */
};

/* ------------------------------------------------------------------------
 * Filesystem (mounted share) handlers - the "Mounted Shares" page plugins.
 *
 * A share is one DOS Mountfile in the mounts directory, owned by whichever
 * handler claims it (by its Filesystem/EHandler line).  The core scans the
 * directory, pre-parses the generic Mountfile grammar, and offers each file
 * to the registered handlers; handlers parse/write only their own format.
 * ------------------------------------------------------------------------ */

/* Buffer sizes - FROZEN ABI shared between the editor and the plugins
 * (like struct ProtocolAddress).  Do not change without bumping the
 * netprefs.conf version and rebuilding every module. */
#define MOUNT_DEVBUFLEN     64  /* device/file name (Envoy "<host>-<export>") */
#define MOUNT_HOSTBUFLEN    128
#define MOUNT_FIELDBUFLEN   80  /* EFS ParseUnit fields are at most 79 chars  */

struct MountedShare
{
    struct Node ms_node;        /* ln_Type = owning handler's fsh_ID,
                                 * ln_Name -> ms_device                       */
    TEXT ms_device[MOUNT_DEVBUFLEN];    /* mountfile name, no trailing ':'    */
    TEXT ms_host[MOUNT_HOSTBUFLEN];
    TEXT ms_volume[MOUNT_FIELDBUFLEN];  /* CIFS: share;     EFS: export       */
    TEXT ms_user[MOUNT_FIELDBUFLEN];
    TEXT ms_secret[MOUNT_FIELDBUFLEN];  /* CIFS: password;  EFS: "$hash"      */
    TEXT ms_extra[MOUNT_FIELDBUFLEN];   /* CIFS: workgroup; EFS: Unit flags   */
    BOOL ms_active;
};

/*
 * Generic Mountfile fields, pre-parsed by the core so every handler does not
 * need its own copy of the full Mountfile keyword template.  Any pointer may
 * be NULL when the keyword is absent.
 */
struct NetPrefsMountInfo
{
    CONST_STRPTR nmi_FileName;      /* mountfile name = device name */
    CONST_STRPTR nmi_Handler;       /* HANDLER=    */
    CONST_STRPTR nmi_EHandler;      /* EHANDLER=   */
    CONST_STRPTR nmi_Filesystem;    /* FILESYSTEM= */
    CONST_STRPTR nmi_Control;       /* CONTROL=    */
    CONST_STRPTR nmi_Unit;          /* UNIT=       */
    CONST_STRPTR nmi_Device;        /* DEVICE=     */
};

/*
 * Callback: claim and parse one mountfile.  Return TRUE if the file belongs
 * to this handler and fill ms (the core allocates and frees it, and owns
 * ms_device / ms_active / ms_node); return FALSE to pass the file on.
 */
typedef BOOL (*NETPREFS_READMOUNT)(const struct NetPrefsMountInfo *mi,
                                   struct MountedShare *ms);

/* Callback: write one share's complete Mountfile text to f */
typedef BOOL (*NETPREFS_WRITEMOUNT)(FILE *f, const struct MountedShare *ms);

/* Callback: defaults for a freshly added share (domain = editor's domain) */
typedef void (*NETPREFS_INITSHARE)(struct MountedShare *ms,
                                   CONST_STRPTR domain);

/* Callback (optional, may be NULL): short list-column text override */
typedef void (*NETPREFS_FSDISPLAY)(struct MountedShare *ms,
                                   STRPTR buf, ULONG buflen);

#endif /* _NETPREFS_MODULE_H_ */
