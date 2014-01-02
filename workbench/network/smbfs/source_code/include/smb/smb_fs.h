/*
 * $Id$
 *
 * :ts=8
 *
 * smb_fs.h
 *
 * Copyright (C) 1995 by Paal-Kr. Engstad and Volker Lendecke
 * Modified for use with AmigaOS by Olaf Barthel <olsen@sourcery.han.de>
 * Modified for supporting SMBlockingX packets by Peter Riede <Noster-Riede@T-Online.de>
 */

#ifndef _LINUX_SMB_FS_H
#define _LINUX_SMB_FS_H

#include <smb/smb.h>

#include <smb/smb_mount.h>
#include <smb/smb_fs_sb.h>

#include <netinet/in.h>

#define SMB_HEADER_LEN   37     /* includes everything up to, but not
                                   including smb_bcc */

/* This structure is used to pass the arguments to smb_proc_lockingX
 */
struct smb_lkrng
{
	off_t	offset;						/* offset to first byte to be (un)locked */
	long len;							/* bytesize of the block */
};

/* Macros to get at offsets within smb_lkrng and smb_unlkrng
   structures. We cannot define these as actual structures
   due to possible differences in structure packing
   on different machines/compilers. */

#define SMB_LPID_OFFSET(indx) (10 * (indx))
#define SMB_LKOFF_OFFSET(indx) ( 2 + (10 * (indx)))
#define SMB_LKLEN_OFFSET(indx) ( 6 + (10 * (indx)))
#define SMB_LARGE_LKOFF_OFFSET_HIGH(indx) (4 + (20 * (indx)))
#define SMB_LARGE_LKOFF_OFFSET_LOW(indx) (8 + (20 * (indx)))
#define SMB_LARGE_LKLEN_OFFSET_HIGH(indx) (12 + (20 * (indx)))
#define SMB_LARGE_LKLEN_OFFSET_LOW(indx) (16 + (20 * (indx)))

/*****************************************************************************/

/* proc.c */
byte *smb_encode_smb_length(byte *p, dword len);
dword smb_len(byte *packet);
int smb_proc_open(struct smb_server *server, const char *pathname, int len, struct smb_dirent *entry);
int smb_proc_close(struct smb_server *server, word fileid, dword mtime);
int smb_proc_read(struct smb_server *server, struct smb_dirent *finfo, off_t offset, long count, char *data, int fs);
int smb_proc_read_raw(struct smb_server *server, struct smb_dirent *finfo, off_t offset, long count, char *data);
int smb_proc_write (struct smb_server *server, struct smb_dirent *finfo, off_t offset, long count, const char *data);
int smb_proc_write_raw(struct smb_server *server, struct smb_dirent *finfo, off_t offset, long count, const char *data);
int smb_proc_lseek (struct smb_server *server, struct smb_dirent *finfo, off_t offset, int mode, off_t  * new_position_ptr);
int smb_proc_lockingX (struct smb_server *server, struct smb_dirent *finfo, struct smb_lkrng *locks, int num_entries, int mode, long timeout);
int smb_proc_create(struct smb_server *server, const char *path, int len, struct smb_dirent *entry);
int smb_proc_mv(struct smb_server *server, const char *opath, const int olen, const char *npath, const int nlen);
int smb_proc_mkdir(struct smb_server *server, const char *path, const int len);
int smb_proc_rmdir(struct smb_server *server, const char *path, const int len);
int smb_proc_unlink(struct smb_server *server, const char *path, const int len);
int smb_proc_trunc(struct smb_server *server, word fid, dword length);
int smb_proc_readdir(struct smb_server *server, char *path, int fpos, int cache_size, struct smb_dirent *entry);
int smb_proc_getattr_core(struct smb_server *server, const char *path, int len, struct smb_dirent *entry);
int smb_proc_getattrE(struct smb_server *server, struct smb_dirent *entry);
int smb_proc_setattr_core(struct smb_server *server, const char *path, int len, struct smb_dirent *new_finfo);
int smb_proc_setattrE(struct smb_server *server, word fid, struct smb_dirent *new_entry);
int smb_proc_dskattr (struct smb_server *server, struct smb_dskattr *attr);
int smb_proc_connect(struct smb_server *server);

/* sock.c */
int smb_catch_keepalive(struct smb_server *server);
int smb_dont_catch_keepalive(struct smb_server *server);
int smb_release(struct smb_server *server);
int smb_connect(struct smb_server *server);
int smb_request(struct smb_server *server);
int smb_trans2_request(struct smb_server *server, int *data_len, int *param_len, char **data, char **param);
int smb_request_read_raw(struct smb_server *server, unsigned char *target, int max_len);
int smb_request_write_raw(struct smb_server *server, unsigned const char *source, int length);

#endif /* _LINUX_SMB_FS_H */
