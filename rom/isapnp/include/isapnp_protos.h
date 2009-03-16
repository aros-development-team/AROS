#ifndef CLIB_ISAPNP_PROTOS_H
#define CLIB_ISAPNP_PROTOS_H

/*
**      $VER: isapnp_protos.h 1.1 (10.5.2001)
**
**      C prototypes. For use with 32 bit integers only.
**
**      (C) Copyright 2001 Martin Blom
**      All Rights Reserved.
**
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

struct ISAPNP_Card;
struct ISAPNP_Device;
struct ISAPNP_ResourceGroup;
struct ISAPNP_Resource;


// ISA functions

void ISAC_SetMasterInt( BOOL on );
BOOL ISAC_GetMasterInt( void );

void ISAC_SetWaitState( BOOL on );
BOOL ISAC_GetWaitState( void );

BOOL ISAC_GetInterruptStatus( UBYTE interrupt );

UBYTE ISAC_GetRegByte( UWORD reg );
void ISAC_SetRegByte( UWORD reg, UBYTE value );

UWORD ISAC_GetRegWord( UWORD reg );
void ISAC_SetRegWord( UWORD reg, UWORD value );

ULONG ISAC_GetRegLong( UWORD reg );
void ISAC_SetRegLong( UWORD reg, ULONG value );

UBYTE ISAC_ReadByte( ULONG address );
void ISAC_WriteByte( ULONG address, UBYTE value );

UWORD ISAC_ReadWord( ULONG address );
void ISAC_WriteWord( ULONG address, UWORD value );

ULONG ISAC_ReadLong( ULONG address );
void ISAC_WriteLong( ULONG address, ULONG value );


// Structure allocation and deallocation (private)

struct ISAPNP_Card* ISAPNP_AllocCard( void );
void ISAPNP_FreeCard( struct ISAPNP_Card* card );

struct ISAPNP_Device* ISAPNP_AllocDevice( void );
void ISAPNP_FreeDevice( struct ISAPNP_Device* dev );

struct ISAPNP_ResourceGroup* ISAPNP_AllocResourceGroup( UBYTE pri );
void ISAPNP_FreeResourceGroup( struct ISAPNP_ResourceGroup* rg );

struct ISAPNP_Resource* ISAPNP_AllocResource( UBYTE type );
void ISAPNP_FreeResource( struct ISAPNP_Resource* r );


// PnP activation (private)

BOOL ISAPNP_ScanCards( void );
BOOL ISAPNP_ConfigureCards( void );


// Card and device handling

struct ISAPNP_Card* ISAPNP_FindCard( struct ISAPNP_Card* last_card, LONG manufacturer, WORD product, BYTE revision, LONG serial );
struct ISAPNP_Device* ISAPNP_FindDevice( struct ISAPNP_Device* last_device, LONG manufacturer, WORD product, BYTE revision );

APTR ISAPNP_LockCardsA( ULONG flags, struct ISAPNP_Card** cards );
APTR ISAPNP_LockCards( ULONG flags, struct ISAPNP_Card* card, .... );
void ISAPNP_UnlockCards( APTR card_lock_handle );

APTR ISAPNP_LockDevicesA( ULONG flags, struct ISAPNP_Device** devices );
APTR ISAPNP_LockDevices( ULONG flags, struct ISAPNP_Device* device, ... );
void ISAPNP_UnlockDevices( APTR device_lock_handle );

#endif /* CLIB_PNPISA_PROTOS_H */
