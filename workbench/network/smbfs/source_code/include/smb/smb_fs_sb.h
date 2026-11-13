/*
 * $Id$
 *
 * :ts=4
 *
 * smb_fs_sb.h
 *
 * Copyright (C) 1995 by Paal-Kr. Engstad and Volker Lendecke
 * Modified for use with AmigaOS by Olaf Barthel <obarthel -at- gmx -dot- net>
 */

#ifndef _SMB_FS_SB
#define _SMB_FS_SB

#include <smb/smb.h>
#include <smb/smb_mount.h>

struct smb_server
{
	enum smb_protocol protocol;		/* The protocol this
									   connection accepts. */

	dword max_buffer_size;			/* Maximum SMB message size, including
									   message header, parameter and
									   data blocks */
	dword max_raw_size;				/* Maximum SMB_COM_WRITE_RAW and
									   SMB_COM_READ_RAW data. */
	int max_recv;		/* added by CS */
	word server_uid;
	word tid;

	struct smb_mount_data mount_data;	/* We store the complete information here
										   to be able to reconnect. */

	unsigned short rcls;				/* The error codes we received */
	unsigned short err;
	unsigned char *packet;

	int security_mode;
	int crypt_key_length;
	unsigned char crypt_key[8];

	struct smba_server * abstraction;

	enum smb_conn_state state;

	/* The following are LANMAN 1.0 options transferred to us in SMBnegprot */
	dword capabilities;

	/* olsen (2012-12-10): raw SMB over TCP instead of NBT transport? */
	int raw_smb;
};

#define NEGOTIATE_USER_SECURITY 0x01	/* If set, the server supports
										   only user level access control.
										   If clear, the server supports
										   only share level access
										   control. */

#define NEGOTIATE_ENCRYPT_PASSWORDS 0x02	/* If set, the server supports
											   challenge/response
											   authentication. If clear,
											   the server supports only
											   plaintext password
											   authentication. */

#define CAP_RAW_MODE 0x00000001	/* The server supports SMB_COM_WRITE_RAW
								   and SMB_COM_READ_RAW requests. */

#endif
