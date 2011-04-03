#ifndef READARGS_H
#define READARGS_H 1

/*************************************************************************/

enum
{
    ARG_DEVICE = 0,
    ARG_UNIT,
    ARG_TO,
    ARG_LIST,
    ARG_LOWERCYL,
    ARG_PREFIX,
    ARG_MAXTRANSFER,
    ARG_MOUNTDOS,
    ARG_MOUNTNTFS,
    ARG_MOUNTEXT,
    ARG_MOUNTSGIX,
    ARGS_SIZEOF
};

#define DEVICENAME_SIZEOF 0x100

/*
** Externals
*/

extern ULONG readargs_array[];

/*
** Prototypes
*/

BOOL ReadArgs_ReadArgs( void );
void ReadArgs_FreeArgs( void );
void ReadArgs_SetDefaults( void );


/*************************************************************************/

#endif /* READARGS_H */

