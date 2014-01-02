/*
 * $Id$
 *
 * :ts=8
 *
 * Name: smb_abstraction.h
 * Description: Interface to the smb abstraction layer.
 * Author: Christian Starkjohann <cs -at- hal -dot- kph -dot- tuwien -dot- ac -dot- at>
 * Date: 1996-12-31
 * Copyright: GNU-GPL
 *
 * Modified for use with AmigaOS by Olaf Barthel <obarthel -at- gmx -dot- net>
 * Modified to support record locking by Peter Riede <Noster-Riede -at- T-Online -dot- de>
 */

#ifndef _SMB_ABSTRACTION_H
#define _SMB_ABSTRACTION_H 1

/****************************************************************************/

/* Forward declaration to keep the compiler happy. */
#ifndef _SMB_FS_SB
struct smb_server;
#endif /* _SMB_FS_SB */

/****************************************************************************/

typedef struct smba_connect_parameters
{
  char server_ipname[64];
  char service[64];
  char *server_name;
  char *client_name;
  char *username;
  char *password;
  int max_xmit;
} smba_connect_parameters_t;

typedef struct smba_stat
{
  unsigned is_dir:1;
  unsigned is_wp:1;
  unsigned is_hidden:1;
  unsigned is_system:1;
  unsigned is_archive:1;
  int size;
  long atime;
  long ctime;
  long mtime;
} smba_stat_t;

/****************************************************************************/

typedef struct smba_server smba_server_t;
typedef struct smba_file smba_file_t;

/****************************************************************************/

typedef int (*smba_callback_t) (void *d, int fpos, int nextpos, char *name, int eof, smba_stat_t * st);

/****************************************************************************/

int smba_open(smba_server_t *s, char *name, size_t name_size, smba_file_t **file);
void smba_close(smba_file_t *f);
int smba_read(smba_file_t *f, char *data, long len, long offset);
int smba_write(smba_file_t *f, char *data, long len, long offset);
long smba_seek (smba_file_t *f, long offset, long mode, off_t * new_position_ptr);
int smba_lockrec (smba_file_t *f, long offset, long len, long mode, int unlocked, long timeout);
int smba_getattr(smba_file_t *f, smba_stat_t *data);
int smba_setattr(smba_file_t *f, smba_stat_t *data);
int smba_readdir(smba_file_t *f, long offs, void *d, smba_callback_t callback);
int smba_create(smba_file_t *dir, const char *name, smba_stat_t *attr);
int smba_mkdir(smba_file_t *dir, const char *name);
int smba_remove(smba_server_t *s, char *path);
int smba_rmdir(smba_server_t *s, char *path);
int smba_rename(smba_server_t *s, char *from, char *to);
int smba_statfs(smba_server_t *s, long *bsize, long *blocks, long *bfree);
void smb_invalidate_all_inodes(struct smb_server *server);
int smba_start(char *service, char *opt_workgroup, char *opt_username, char *opt_password, char *opt_clientname, char *opt_servername, int opt_cachesize, smba_server_t **result);
void smba_disconnect(smba_server_t *server);
int smba_get_dircache_size(struct smba_server * server);
int smba_change_dircache_size(struct smba_server * server,int cache_size);

/****************************************************************************/

#endif /* _SMB_ABSTRACTION_H */
