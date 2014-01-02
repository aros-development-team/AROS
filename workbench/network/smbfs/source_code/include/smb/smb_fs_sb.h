/*
 * $Id$
 *
 * :ts=8
 *
 * smb_fs_sb.h
 *
 * Copyright (C) 1995 by Paal-Kr. Engstad and Volker Lendecke
 * Modified for use with AmigaOS by Olaf Barthel <olsen@sourcery.han.de>
 */

#ifndef _SMB_FS_SB
#define _SMB_FS_SB

#include <smb/smb.h>
#include <smb/smb_mount.h>

struct smb_server
{
  enum smb_protocol protocol;   /* The protocol this
                                   connection accepts. */

  word max_xmit;
  int max_recv;                 /* added by CS */
  word server_uid;
  word tid;

  struct smb_mount_data mount_data; /* We store the complete information here
                                       to be able to reconnect.
                                     */

  unsigned short rcls;          /* The error codes we received */
  unsigned short err;
  unsigned char *packet;

  int security_mode;
  unsigned char crypt_key[8];

  struct smba_server * abstraction;

  enum smb_conn_state state;

  /* The following are LANMAN 1.0 options transferred to us in
     SMBnegprot */
  word maxxmt;
  word blkmode;
  dword sesskey;
};

#endif
