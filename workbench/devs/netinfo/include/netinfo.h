#ifndef DEVICES_NETINFO_H
#define DEVICES_NETINFO_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_IO_H
#include <exec/io.h>
#endif

#define NETINFONAME "DEVS:netinfo.device"

/* Modified IOStdReq */
struct NetInfoReq {
    struct  Message     io_Message;
    struct  Device      *io_Device;     /* device node pointer  */
    struct  Unit        *io_Unit;       /* unit (driver private) */
    UWORD               io_Command;     /* device command */
    UBYTE               io_Flags;
    BYTE                io_Error;       /* error or warning num */
    ULONG               io_Actual;      /* actual number of bytes transferred */
    ULONG               io_Length;      /* requested number bytes transferred*/
    APTR                io_Data;        /* points to data area */
    ULONG               io_Offset;      /* search criteria */
};

/* NetInfo units */
enum {
    NETINFO_PASSWD_UNIT = 0,
    NETINFO_GROUP_UNIT,
    NETINFO_UNITS
};

/* Non-standard commands */
#define NI_GETBYID              (CMD_NONSTD + 0)
#define NI_GETBYNAME            (CMD_NONSTD + 1)
#define NI_MEMBERS              (CMD_NONSTD + 2)
#define NI_END                  (CMD_NONSTD + 3)

/* Non-standard error codes (matching BSD errno) */
#define NIERR_NOTFOUND          2
#define NIERR_TOOSMALL          7
#define NIERR_NOMEM             12
#define NIERR_ACCESS            13
#define NIERR_NULL_POINTER      14
#define NIERR_INVAL             22

/* The passwd structure */
struct NetInfoPasswd
{
  UBYTE *pw_name;                       /* Username */
  UBYTE *pw_passwd;                     /* Encrypted password */
  LONG  pw_uid;                         /* User ID */
  LONG  pw_gid;                         /* Group ID */
  UBYTE *pw_gecos;                      /* Real name etc */
  UBYTE *pw_dir;                        /* Home directory */
  UBYTE *pw_shell;                      /* Shell */
} __packed;

/* The group structure */
struct NetInfoGroup
{
  UBYTE *gr_name;                       /* Group name.  */
  UBYTE *gr_passwd;                     /* Password.    */
  LONG  gr_gid;                         /* Group ID.    */
  UBYTE **gr_mem;                       /* Member list. */
} __packed;

#endif /* DEVICES_NETINFO_H */
