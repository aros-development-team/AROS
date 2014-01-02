/*
 * $Id$
 *
 * :ts=8
 *
 * Name: smb_abstraction.c
 * Description: Smb abstraction layer.
 * Author: Christian Starkjohann <cs -at- hal -dot- kph -dot- tuwien -dot- ac -dot- at>
 * Date: 1996-12-31
 * Copyright: GNU-GPL
 *
 * Modified for use with AmigaOS by Olaf Barthel <obarthel -at- gmx -dot- net>
 */

#include "smbfs.h"

/*****************************************************************************/

#include <smb/smb_fs.h>
#include <smb/smb.h>
#include <smb/smbno.h>

#define MAXHOSTNAMELEN 256

/*****************************************************************************/

#define ATTR_CACHE_TIME     5   /* cache attributes for this time */
#define DIR_CACHE_TIME      5   /* cache directories for this time */
#define DIRCACHE_SIZE       170
#define DOS_PATHSEP         '\\'

/*****************************************************************************/

typedef struct dircache
{
  int base;
  int len;
  int eof;                      /* cache end is eof */
  long created_at;              /* for invalidation */
  struct smba_file *cache_for;  /* owner of this cache */
  int cache_size;
  struct smb_dirent cache[1];
} dircache_t;

/* opaque structures for server and files: */
struct smba_server
{
  struct smb_server server;
  struct MinList open_files;
  dircache_t * dircache;
  unsigned supports_E:1;
  unsigned supports_E_known:1;
};

struct smba_file
{
  struct MinNode node;
  struct smba_server *server;
  struct smb_dirent dirent;
  long attr_time;               /* time when dirent was read */
  dircache_t *dircache;         /* content cache for directories */
  unsigned attr_dirty:1;        /* attribute cache is dirty */
  unsigned is_valid:1;          /* server was down, entry removed, ... */
};

/*****************************************************************************/

#include "smb_abstraction.h"

/*****************************************************************************/

static int smba_connect(smba_connect_parameters_t *p, unsigned int ip_addr, int use_E, char *workgroup_name, int cache_size, smba_server_t **result);
static INLINE int make_open(smba_file_t *f, int need_fid);
static int write_attr(smba_file_t *f);
static void invalidate_dircache(struct smba_server *server, char *path);
static void close_path(smba_server_t *s, char *path);
static void smba_cleanup_dircache(struct smba_server *server);
static int smba_setup_dircache(struct smba_server *server, int cache_size);
static int extract_service (char *service, char *server, size_t server_size, char *share, size_t share_size);

/*****************************************************************************/

static int
smba_connect (smba_connect_parameters_t * p, unsigned int ip_addr, int use_E, char * workgroup_name, int cache_size, smba_server_t ** result)
{
  smba_server_t *res;
  struct smb_mount_data data;
  int errnum;
  char hostname[MAXHOSTNAMELEN], *s;
  struct servent * servent;

  (*result) = NULL;

  res = malloc (sizeof(*res));
  if(res == NULL)
  {
    ReportError("Not enough memory.");
    errnum = -ENOMEM;
    goto error_occured;
  }

  memset (res, 0, sizeof(*res));
  memset (&data, 0, sizeof (data));
  memset (hostname, 0, sizeof (hostname));

  errnum = smba_setup_dircache (res,cache_size);
  if(errnum < 0)
  {
    ReportError("Directory cache initialization failed (%ld, %s).",-errnum,amitcp_strerror(-errnum));
    goto error_occured;
  }

  strlcpy(data.workgroup_name,workgroup_name,sizeof(data.workgroup_name));

  res->server.abstraction = res;

  gethostname (hostname, MAXHOSTNAMELEN);

  if ((s = strchr (hostname, '.')) != NULL)
    (*s) = '\0';

  data.addr.sin_family = AF_INET;
  data.addr.sin_addr.s_addr = ip_addr;

  servent = getservbyname("netbios-ssn","tcp");
  if(servent != NULL)
    data.addr.sin_port = htons (servent->s_port);
  else
    data.addr.sin_port = htons (SMB_PORT);

  data.fd = socket (AF_INET, SOCK_STREAM, 0);
  if (data.fd < 0)
  {
    errnum = (-errno);
    ReportError("socket() call failed (%ld, %s).", errno, amitcp_strerror (errno));
    goto error_occured;
  }

  strlcpy (data.service, p->service, sizeof(data.service));
  StringToUpper (data.service);
  strlcpy (data.username, p->username, sizeof(data.username));
  strlcpy (data.password, p->password, sizeof(data.password));

  if (p->max_xmit > 0)
    data.max_xmit = p->max_xmit;
  else
    data.max_xmit = 65530 /*8300*/;

  strlcpy (data.server_name, p->server_name, sizeof(data.server_name));
  strlcpy (data.client_name, p->client_name, sizeof(data.client_name));

  if (data.server_name[0] == '\0')
  {
    if (strlen (p->server_ipname) > 16)
    {
      errnum = -ENAMETOOLONG;
      ReportError("Server name '%s' is too long for NetBIOS (max %ld characters).",p->server_ipname,16);
      goto error_occured;
    }

    strlcpy (data.server_name, p->server_ipname, sizeof(data.server_name));
  }

  StringToUpper (data.server_name);

  if (data.client_name[0] == '\0')
  {
    if (strlen (hostname) > 16)
    {
      errnum = -ENAMETOOLONG;
      ReportError("Local host name '%s' is too long for NetBIOS (max %ld characters).", hostname, 16);
      goto error_occured;
    }

    strlcpy (data.client_name, hostname, sizeof(data.client_name));
    StringToUpper (data.client_name);
  }

  res->server.mount_data = data;

  NewList((struct List *)&res->open_files);

  if ((errnum = smb_proc_connect (&res->server)) < 0)
  {
    ReportError("Cannot connect to server (%ld, %s).", -errnum,amitcp_strerror(-errnum));
    goto error_occured;
  }

  if (!use_E)
    res->supports_E_known = 1;

  (*result) = res;

  return 0;

 error_occured:

  if(res != NULL)
  {
    smba_cleanup_dircache (res);
    free (res);
  }

  return errnum;
}

/*****************************************************************************/

void
smba_disconnect (smba_server_t * server)
{
  CloseSocket (server->server.mount_data.fd);

  smba_cleanup_dircache(server);

  free (server);
}

/*****************************************************************************/

static INLINE int
make_open (smba_file_t * f, int need_fid)
{
  int errnum = 0;
  smba_server_t *s;

  if (!f->is_valid || (need_fid && !f->dirent.opened))
  {
    s = f->server;

    if (!f->is_valid || f->attr_time == -1 || GetCurrentTime() - f->attr_time > ATTR_CACHE_TIME)
    {
      if ((errnum = smb_proc_getattr_core (&s->server, f->dirent.complete_path, f->dirent.len, &f->dirent)) < 0)
        goto error_occured;
    }

    if ((f->dirent.attr & aDIR) == 0) /* a regular file */
    {
      if (need_fid || !s->supports_E_known || s->supports_E)
      {
        LOG (("opening file %s\n", f->dirent.complete_path));
        if ((errnum = smb_proc_open (&s->server, f->dirent.complete_path, f->dirent.len, &f->dirent)) < 0)
          goto error_occured;

        if (s->supports_E || !s->supports_E_known)
        {
          if (smb_proc_getattrE (&s->server, &f->dirent) < 0)
          {
            if (!s->supports_E_known)
            {
              s->supports_E_known = 1;
              s->supports_E = 0;
            } /* ignore errors here */
          }
          else
          {
            s->supports_E_known = 1;
            s->supports_E = 1;
          }
        }
      }
    }
    else
    {
      /* don't open directory, initialize directory cache */
      if (f->dircache != NULL)
      {
        f->dircache->cache_for = NULL;
        f->dircache->len = 0;
        f->dircache = NULL;
      }
    }

    f->attr_time = GetCurrentTime();
    f->is_valid = 1;
  }

 error_occured:

  return errnum;
}

/*****************************************************************************/

int
smba_open (smba_server_t * s, char *name, size_t name_size, smba_file_t ** file)
{
  smba_file_t *f;
  int errnum;

  (*file) = NULL;

  f = malloc (sizeof(*f));
  if(f == NULL)
  {
    errnum = -ENOMEM;
    goto error_occured;
  }

  memset(f,0,sizeof(*f));

  f->dirent.complete_path = name;
  f->dirent.complete_path_size = name_size;
  f->dirent.len = strlen (name);
  f->server = s;

  errnum = make_open (f, 0);
  if (errnum < 0)
    goto error_occured;

  AddTail ((struct List *)&s->open_files, (struct Node *)f);

  (*file) = f;

  return 0;

 error_occured:

  if (f != NULL)
    free (f);

  return errnum;
}

/*****************************************************************************/

static int
write_attr (smba_file_t * f)
{
  int errnum;

  LOG (("file %s\n", f->dirent.complete_path));

  errnum = make_open (f, 0);
  if (errnum < 0)
    goto out;

  if (f->dirent.opened && f->server->supports_E)
    errnum = smb_proc_setattrE (&f->server->server, f->dirent.fileid, &f->dirent);
  else
    errnum = smb_proc_setattr_core (&f->server->server, f->dirent.complete_path, f->dirent.len, &f->dirent);

  if (errnum < 0)
    f->attr_time = -1;
  else
    f->attr_dirty = 0;

 out:

  return errnum;
}

/*****************************************************************************/

void
smba_close (smba_file_t * f)
{
  if(f->node.mln_Succ != NULL || f->node.mln_Pred != NULL)
    Remove((struct Node *)f);

  if(f->attr_dirty)
    write_attr(f);

  if (f->dirent.opened)
  {
    LOG (("closing file %s\n", f->dirent.complete_path));
    smb_proc_close (&f->server->server, f->dirent.fileid, f->dirent.mtime);
  }

  if (f->dircache != NULL)
  {
    f->dircache->cache_for = NULL;
    f->dircache->len = 0;
    f->dircache = NULL;
  }

  free (f);
}

/*****************************************************************************/

int
smba_read (smba_file_t * f, char *data, long len, long offset)
{
  int maxsize, count, totalcount, result;
  int bytes_read = 0;
  char *rpos;

  result = make_open (f, 1);
  if (result < 0)
    goto out;

  D(("read %ld bytes from offset %ld",len,offset));

  if (f->server->server.blkmode & 1)
  {
    SHOWVALUE(f->server->server.max_xmit);

    if(len <= 65535)
    {
      result = smb_proc_read_raw (&f->server->server, &f->dirent, offset, len, data);

      LOG (("smb_proc_read_raw(%s)->%ld\n", f->dirent.complete_path, result));
    }
    else if (len > 0)
    {
      int n;

      totalcount = 0;

      do
      {
        n = min(len,65535);

        result = smb_proc_read_raw (&f->server->server, &f->dirent, offset, n, data);
        if(result <= 0)
        {
          D(("!!! wanted to read %ld bytes, got %ld",n,result));
          totalcount = -1;
          break;
        }

        data += result;
        offset += result;
        len -= result;
        totalcount += result;

        if(result < n)
        {
          D(("read returned fewer characters than expected (%ld < %ld)",result,n));
          break;
        }
      }
      while(len > 0);

      if(totalcount != -1)
      {
        result = totalcount;
        goto out;
      }
    }
  }

  if (result <= 0)
  {
    totalcount = len;
    rpos = data;

    maxsize = f->server->server.max_xmit - SMB_HEADER_LEN - 5 * 2 - 5;

    do
    {
      count = totalcount > maxsize ? maxsize : totalcount;

      result = smb_proc_read (&f->server->server, &f->dirent, offset, count, rpos, 0);
      if (result <= 0)
        break;

      bytes_read += result;
      totalcount -= result;
      offset += result;
      rpos += result;

      if(result < count)
      {
        D(("read returned fewer characters than expected (%ld < %ld)",result,count));
        break;
      }
    }
    while (totalcount > 0);

    if(result >= 0)
      result = bytes_read;
  }

 out:

  return result;
}

/*****************************************************************************/

int
smba_write (smba_file_t * f, char *data, long len, long offset)
{
  int newlen, maxsize, totalcount, count, result;

  if ((result = make_open (f, 1)) < 0)
    return result;

  /* Calculate maximum number of bytes that could be tranfered with
     a single SMBwrite packet... */
  maxsize = f->server->server.max_xmit - (SMB_HEADER_LEN + 5 * sizeof (word) + 5) - 4;

  if (len <= maxsize)
  {
    /* Use a single SMBwrite packet whenever possible instead of a SMBwritebraw
       because that requires two packets to be send. */
    result = smb_proc_write (&f->server->server, &f->dirent, offset, len, data);
  }
  else
  {
    /* Added by Brian Willette - We were always checking bit 2 here, but
       according to the documentation I have, the newer versions of SMB put
       the SMB_RAW_WRITE AND SMB_RAW_READ capability in bit 1 */
    if ((f->server->server.protocol >= PROTOCOL_NT1
        && f->server->server.blkmode & 1)
        || (f->server->server.protocol < PROTOCOL_NT1
           && f->server->server.blkmode & 2))
    {
      long maxxmit;
      int n;

      totalcount = 0;

      /* Try to send the maximum number of bytes with the two SMBwritebraw packets. */
      maxxmit = 2 * f->server->server.max_xmit - (SMB_HEADER_LEN + 12 * sizeof (word) + 4) - 8;

      /* If the number of bytes that should be transfered exceed the number of
         bytes that could be transfered by a single call to smb_proc_write_raw,
         the data is transfered as before: Only by the second packet, this
         prevents the CPU from copying the data into the transferbuffer. */
      if (maxxmit < len)
        maxxmit = f->server->server.max_xmit - 4;

      do
      {
        n = min(len,maxxmit);

        if (n <= maxsize)
        {
          /* Use a single SMBwrite packet whenever possible instead of a
             SMBwritebraw because that requires two packets to be send. */
          result = smb_proc_write (&f->server->server, &f->dirent, offset, n, data);
        }
        else
        {
          result = smb_proc_write_raw (&f->server->server, &f->dirent, offset, n, data);
        }

        if(result <= 0)
        {
          totalcount = -1;
          break;
        }

        data += result;
        offset += result;
        len -= result;
        totalcount += result;

        if(result < n)
          break;
      }
      while(len > 0);

      if(totalcount != -1)
      {
        len = totalcount;
        goto out;
      }
    }
    if (result <= 0)
    {
      /* Failed to use the SMBwritebraw packet, fallback to SMBwrite... */
      totalcount = len;
      len = 0;

      do
      {
        count = totalcount > maxsize ? maxsize : totalcount;
        result = smb_proc_write (&f->server->server, &f->dirent, offset, count, data);
        if (result < 0)
          break;

        totalcount -= result;
        offset += result;
        data += result;
        len += result;

        if(result < count)
          break;
      }
      while (totalcount > 0);
    }
  }

 out:

  if (result < 0)
  {
    f->attr_time = -1;
    return result;
  }

  f->dirent.mtime = GetCurrentTime();

  newlen = f->dirent.size;

  if (offset + len > newlen)
    newlen = offset + len;

  f->dirent.size = newlen;
  return len;
}

/*****************************************************************************/

long
smba_seek (smba_file_t *f, long offset, long mode, off_t * new_position_ptr)
{
   long result;

   D(("seek %ld bytes from position %s",offset,mode > 0 ? (mode == 2 ? "SEEK_END" : "SEEK_CUR") : "SEEK_SET"));

   if ((result = make_open (f, 1)) >= 0)
      result = smb_proc_lseek (&f->server->server, &f->dirent, offset, mode, new_position_ptr);

   return result;
}

/*****************************************************************************/

/* perform a single record-lock */
int
smba_lockrec (smba_file_t *f, long offset, long len, long mode, int unlocked, long timeout)
{
   int errnum;
   struct smb_lkrng *rec_lock;

   if ((errnum = make_open (f, 1)) >= 0)
   {
      if (unlocked)
        mode |= 2;

      rec_lock = malloc (sizeof (struct smb_lkrng));
      if (rec_lock != NULL)
      {
         rec_lock->offset = offset,
         rec_lock->len = len;

         errnum = smb_proc_lockingX (&f->server->server, &f->dirent, rec_lock, 1, mode, timeout);

         free (rec_lock);
      }
      else
      {
         errnum = -ENOMEM;
      }
   }

   return errnum;
}

/*****************************************************************************/

int
smba_getattr (smba_file_t * f, smba_stat_t * data)
{
  long now = GetCurrentTime();
  int errnum;

  errnum = make_open (f, 0);
  if (errnum < 0)
    goto out;

  if (f->attr_time == -1 || (now - f->attr_time) > ATTR_CACHE_TIME)
  {
    LOG (("file %s\n", f->dirent.complete_path));

    if (f->dirent.opened && f->server->supports_E)
      errnum = smb_proc_getattrE (&f->server->server, &f->dirent);
    else
      errnum = smb_proc_getattr_core (&f->server->server, f->dirent.complete_path, f->dirent.len, &f->dirent);

    if (errnum >= 0)
      f->attr_time = now;
  }

  data->is_dir = (f->dirent.attr & aDIR) != 0;
  data->is_wp = (f->dirent.attr & aRONLY) != 0;
  data->is_hidden = (f->dirent.attr & aHIDDEN) != 0;
  data->is_system = (f->dirent.attr & aSYSTEM) != 0;
  data->is_archive = (f->dirent.attr & aARCH) != 0;

  data->size = f->dirent.size;
  data->atime = f->dirent.atime;
  data->ctime = f->dirent.ctime;
  data->mtime = f->dirent.mtime;

 out:

  return errnum;
}

/*****************************************************************************/

int
smba_setattr (smba_file_t * f, smba_stat_t * data)
{
  int errnum;

  if (data->atime != -1)
    f->dirent.atime = data->atime;

  if (data->ctime != -1)
    f->dirent.ctime = data->ctime;

  if (data->mtime != -1)
    f->dirent.mtime = data->mtime;

  if (data->is_wp)
    f->dirent.attr |= aRONLY;
  else
    f->dirent.attr &= ~aRONLY;

  if (data->is_archive)
    f->dirent.attr |= aARCH;
  else
    f->dirent.attr &= ~aARCH;

  if (data->is_system)
    f->dirent.attr |= aSYSTEM;
  else
    f->dirent.attr &= ~aSYSTEM;

  f->attr_dirty = 1;

  if ((errnum = write_attr (f)) < 0)
    goto error_occured;

  if (data->size != -1 &&
      data->size != (int)f->dirent.size)
  {
    if ((errnum = make_open (f, 1)) < 0)
      goto error_occured;

    if ((errnum = smb_proc_trunc (&f->server->server, f->dirent.fileid, data->size)) < 0)
      goto error_occured;

    f->dirent.size = data->size;
  }

 error_occured:

  return errnum;
}

/*****************************************************************************/

int
smba_readdir (smba_file_t * f, long offs, void *d, smba_callback_t callback)
{
  int cache_index, rval, o, eof, count = 0;
  long now = GetCurrentTime();
  smba_stat_t data;

  rval = make_open (f, 0);
  if (rval < 0)
    goto out;

  if (f->dircache == NULL) /* get a cache */
  {
    dircache_t * dircache = f->server->dircache;

    if (dircache->cache_for != NULL)
      dircache->cache_for->dircache = NULL;  /* steal it */

    dircache->eof = dircache->len = dircache->base = 0;
    dircache->cache_for = f;

    f->dircache = dircache;

    LOG (("stealing cache\n"));
  }
  else
  {
    if ((now - f->dircache->created_at) >= DIR_CACHE_TIME)
    {
      f->dircache->eof = f->dircache->len = f->dircache->base = 0;

      LOG (("cache outdated\n"));
    }
  }

  for (cache_index = offs ; ; cache_index++)
  {
    if (cache_index < f->dircache->base  /* fill cache if necessary */
     || cache_index >= f->dircache->base + f->dircache->len)
    {
      if (cache_index >= f->dircache->base + f->dircache->len && f->dircache->eof)
        break; /* nothing more to read */

      LOG (("cachefill for %s\n", f->dirent.complete_path));
      LOG (("\tbase was: %ld, len was: %ld, newbase=%ld\n", f->dircache->base, f->dircache->len, cache_index));

      f->dircache->len = 0;
      f->dircache->base = cache_index;

      rval = smb_proc_readdir (&f->server->server, f->dirent.complete_path, cache_index, f->dircache->cache_size, f->dircache->cache);
      if (rval <= 0)
        break;

      /* Avoid some hits if restart/retry occured. Should fix the real root
         of this problem really, but I am not bored enough atm. -Piru
         ZZZ this needs further investigation. */
      if (f->dircache == NULL)
      {
        LOG (("lost dircache due to an error, bailing out!\n"));
        rval = -1;
        break;
      }

      f->dircache->len = rval;
      f->dircache->eof = (rval < f->dircache->cache_size);
      f->dircache->created_at = now;

      LOG (("cachefill with %ld entries\n", rval));
    }

    o = cache_index - f->dircache->base;
    eof = (o >= (f->dircache->len - 1) && f->dircache->eof);
    count++;

    LOG (("delivering '%s', cache_index=%ld, eof=%ld\n", f->dircache->cache[o].complete_path, cache_index, eof));

    data.is_dir = (f->dircache->cache[o].attr & aDIR) != 0;
    data.is_wp = (f->dircache->cache[o].attr & aRONLY) != 0;
    data.is_hidden = (f->dircache->cache[o].attr & aHIDDEN) != 0;
    data.is_system = (f->dircache->cache[o].attr & aSYSTEM) != 0;
    data.is_archive = (f->dircache->cache[o].attr & aARCH) != 0;
    data.size = f->dircache->cache[o].size;
    data.atime = f->dircache->cache[o].atime;
    data.ctime = f->dircache->cache[o].ctime;
    data.mtime = f->dircache->cache[o].mtime;

    if ((*callback) (d, cache_index, cache_index + 1, f->dircache->cache[o].complete_path, eof, &data))
      break;
  }

 out:

  if (rval < 0)
    return rval;
  else
    return count;
}

/*****************************************************************************/

static void
invalidate_dircache (struct smba_server * server, char * path)
{
  dircache_t * dircache = server->dircache;
  char other_path[SMB_MAXNAMELEN + 1];

  ENTER();

  if(path != NULL)
  {
    int len,i;

    strlcpy(other_path,path,sizeof(other_path));

    len = strlen(other_path);
    for(i = len-1 ; i >= 0 ; i--)
    {
      if(i == 0)
      {
        other_path[0] = DOS_PATHSEP;
        other_path[1] = '\0';
      }
      else if (other_path[i] == DOS_PATHSEP)
      {
        other_path[i] = '\0';
        break;
      }
    }
  }
  else
  {
    other_path[0] = '\0';
  }

  SHOWSTRING(other_path);

  if(dircache->cache_for != NULL)
    SHOWSTRING(dircache->cache_for->dirent.complete_path);
  else
    SHOWMSG("-- directory cache is empty --");

  if(path == NULL || (dircache->cache_for != NULL && CompareNames(other_path,dircache->cache_for->dirent.complete_path) == SAME))
  {
    SHOWMSG("clearing directory cache");

    dircache->eof = dircache->len = dircache->base = 0;
    if(dircache->cache_for != NULL)
    {
      dircache->cache_for->dircache = NULL;
      dircache->cache_for = NULL;
    }
  }

  LEAVE();
}

/*****************************************************************************/

int
smba_create (smba_file_t * dir, const char *name, smba_stat_t * attr)
{
  struct smb_dirent entry;
  char *path;
  int errnum;

  errnum = make_open (dir, 0);
  if (errnum < 0)
    goto out;

  memset (&entry, 0, sizeof (entry));

  if (attr->is_wp)
    entry.attr |= aRONLY;

  if (attr->is_archive)
    entry.attr |= aARCH;

  if (attr->is_system)
    entry.attr |= aSYSTEM;

  entry.atime = entry.mtime = entry.ctime = GetCurrentTime();

  path = malloc (strlen (name) + dir->dirent.len + 2);
  if(path == NULL)
  {
    errnum = -ENOMEM;
    goto out;
  }

  memcpy (path, dir->dirent.complete_path, dir->dirent.len);
  path[dir->dirent.len] = DOS_PATHSEP;
  strcpy (&path[dir->dirent.len + 1], name);

  errnum = smb_proc_create (&dir->server->server, path, strlen (path), &entry);
  if(errnum >= 0)
    invalidate_dircache (dir->server, path);

  free (path);

 out:

  return errnum;
}

/*****************************************************************************/

int
smba_mkdir (smba_file_t * dir, const char *name)
{
  char *path;
  int errnum;

  errnum = make_open (dir, 0);
  if (errnum < 0)
    goto out;

  path = malloc (strlen (name) + dir->dirent.len + 2);
  if(path == NULL)
  {
    errnum = -ENOMEM;
    goto out;
  }

  memcpy (path, dir->dirent.complete_path, dir->dirent.len);
  path[dir->dirent.len] = DOS_PATHSEP;
  strcpy (&path[dir->dirent.len + 1], name);

  errnum = smb_proc_mkdir (&dir->server->server, path, strlen (path));
  if(errnum >= 0)
    invalidate_dircache (dir->server, path);

  free (path);

 out:

  return errnum;
}

/*****************************************************************************/

static void
close_path (smba_server_t * s, char *path)
{
  smba_file_t *p;

  for (p = (smba_file_t *)s->open_files.mlh_Head;
       p->node.mln_Succ != NULL;
       p = (smba_file_t *)p->node.mln_Succ)
  {
    if (p->is_valid && CompareNames(p->dirent.complete_path, path) == SAME)
    {
      if (p->dirent.opened)
        smb_proc_close (&s->server, p->dirent.fileid, p->dirent.mtime);

      p->dirent.opened = 0;
      p->is_valid = 0;
    }
  }
}

/*****************************************************************************/

int
smba_remove (smba_server_t * s, char *path)
{
  int result;

  close_path (s, path);

  result = smb_proc_unlink (&s->server, path, strlen (path));
  if(result >= 0)
    invalidate_dircache (s, path);

  return result;
}

/*****************************************************************************/

int
smba_rmdir (smba_server_t * s, char *path)
{
  int result;

  close_path (s, path);

  result = smb_proc_rmdir (&s->server, path, strlen (path));
  if(result >= 0)
    invalidate_dircache (s, path);

  return result;
}

/*****************************************************************************/

int
smba_rename (smba_server_t * s, char *from, char *to)
{
  int result;

  close_path (s, from);

  result = smb_proc_mv (&s->server, from, strlen (from), to, strlen (to));
  if(result >= 0)
    invalidate_dircache (s, from);

  return result;
}

/*****************************************************************************/

int
smba_statfs (smba_server_t * s, long *bsize, long *blocks, long *bfree)
{
  struct smb_dskattr dskattr;
  int errnum;

  errnum = smb_proc_dskattr (&s->server, &dskattr);
  if (errnum < 0)
    goto out;

  (*bsize) = dskattr.blocksize * dskattr.allocblocks;
  (*blocks) = dskattr.total;
  (*bfree) = dskattr.free;

 out:

  return errnum;
}

/*****************************************************************************/

void
smb_invalidate_all_inodes (struct smb_server *server)
{
  smba_file_t *f;

  invalidate_dircache (server->abstraction, NULL);

  for (f = (smba_file_t *)server->abstraction->open_files.mlh_Head;
       f->node.mln_Succ != NULL;
       f = (smba_file_t *)f->node.mln_Succ)
  {
    f->dirent.opened = 0;
    f->is_valid = 0;
  }
}

/*****************************************************************************/

static void
smba_cleanup_dircache(struct smba_server * server)
{
  dircache_t * the_dircache = server->dircache;

  if(the_dircache != NULL)
  {
    int i;

    for (i = 0; i < the_dircache->cache_size; i++)
    {
      if(the_dircache->cache[i].complete_path != NULL)
        free(the_dircache->cache[i].complete_path);
    }

    free(the_dircache);
    server->dircache = NULL;
  }
}

static int
smba_setup_dircache (struct smba_server * server,int cache_size)
{
  dircache_t * the_dircache;
  int error = (-ENOMEM);
  int i;

  the_dircache = malloc(sizeof(*the_dircache) + (cache_size-1) * sizeof(the_dircache->cache));
  if(the_dircache == NULL)
    goto out;

  memset(the_dircache,0,sizeof(*the_dircache));
  the_dircache->cache_size = cache_size;

  for (i = 0; i < the_dircache->cache_size; i++)
  {
    the_dircache->cache[i].complete_path = malloc (SMB_MAXNAMELEN + 1);
    if(the_dircache->cache[i].complete_path == NULL)
      goto out;

    the_dircache->cache[i].complete_path_size = SMB_MAXNAMELEN + 1;
  }

  server->dircache = the_dircache;

  error = 0;

 out:

  if(error < 0)
  {
    if(the_dircache != NULL)
    {
      for (i = 0; i < the_dircache->cache_size; i++)
      {
        if(the_dircache->cache[i].complete_path != NULL)
          free(the_dircache->cache[i].complete_path);
      }

      free(the_dircache);
    }
  }

  return(error);
}

/*****************************************************************************/

static int
extract_service (char *service, char *server, size_t server_size, char *share, size_t share_size)
{
  char *share_start;
  char *root_start;
  char *complete_service;
  char * service_copy;
  int result = 0;

  service_copy = malloc(strlen(service)+1);
  if(service_copy == NULL)
  {
    result = -ENOMEM;
    ReportError("Not enough memory.");
    goto out;
  }

  strcpy (service_copy, service);
  complete_service = service_copy;

  if (strlen (complete_service) < 4 || complete_service[0] != '/')
  {
    result = -EINVAL;
    ReportError("Invalid service name '%s'.",complete_service);
    goto out;
  }

  while (complete_service[0] == '/')
    complete_service += 1;

  share_start = strchr (complete_service, '/');
  if (share_start == NULL)
  {
    result = -EINVAL;
    ReportError("Invalid share name '%s'.",complete_service);
    goto out;
  }

  (*share_start++) = '\0';
  root_start = strchr (share_start, '/');

  if (root_start != NULL)
    (*root_start) = '\0';

  if ((strlen (complete_service) > 63) || (strlen (share_start) > 63))
  {
    result = -ENAMETOOLONG;
    ReportError("Server or share name is too long in '%s' (max %ld characters).",service,63);
    goto out;
  }

  strlcpy (server, complete_service, server_size);
  strlcpy (share, share_start, share_size);

 out:

  if(service_copy != NULL)
    free(service_copy);

  return(result);
}

int
smba_start(char * service,char *opt_workgroup,char *opt_username,char *opt_password,char *opt_clientname,char *opt_servername,int opt_cachesize,smba_server_t ** result)
{
  smba_connect_parameters_t par;
  smba_server_t *the_server = NULL;
  int i;
  struct hostent *h;
  int max_xmit = -1;
  int use_extended = 0;
  char server_name[17], client_name[17];
  char username[64], password[64];
  char workgroup[20];
  char server[64], share[64];
  unsigned int ipAddr;
  int error;

  (*result) = NULL;
  (*username) = (*password) = (*server_name) = (*client_name) = '\0';

  error = extract_service (service, server, sizeof(server), share, sizeof(share));
  if(error < 0)
    goto out;

  ipAddr = inet_addr (server);
  if (ipAddr == INADDR_NONE) /* name was given, not numeric */
  {
    int lookup_error;

    h = gethostbyname (server);
    lookup_error = h_errno;

    if (h != NULL)
    {
      ipAddr = ((struct in_addr *)(h->h_addr))->s_addr;
    }
    else if (BroadcastNameQuery(server,"",(UBYTE *)&ipAddr) != 0)
    {
      ReportError("Unknown host '%s' (%ld, %s).",server,lookup_error,host_strerror(lookup_error));
      error = (-ENOENT);
      goto out;
    }
  }
  else
  {
    char hostName[MAXHOSTNAMELEN+1];

    h = gethostbyaddr ((char *) &ipAddr, sizeof (ipAddr), AF_INET);
    if (h == NULL)
    {
      ReportError("Unknown host '%s' (%ld, %s).",server,h_errno,host_strerror(errno));
      error = (-ENOENT);
      goto out;
    }

    /* Brian Willette: Now we will set the server name to the dns
       hostname, hopefully this will be the same as the netbios name for
       the server.
       We do this because the user supplied no hostname, and we
       need one for netbios, this is the best guess choice we have
       NOTE: If the names are different between DNS and netbios on
       the windows side, the user MUST use the -s option. */
    for (i = 0; h->h_name[i] != '.' && h->h_name[i] != '\0' && i < 255; i++)
      hostName[i] = h->h_name[i];

    hostName[i] = '\0';

    /* Make sure the hostname is 16 characters or less (for Netbios) */
    if (strlen (hostName) > 16)
    {
      ReportError("Server host name '%s' is too long (max %ld characters).", hostName, 16);
      error = (-ENAMETOOLONG);
      goto out;
    }

    strlcpy (server_name, hostName, sizeof(server_name));
  }

  if(opt_password != NULL)
  {
    if(strlen(opt_password) >= sizeof(password))
    {
      ReportError("Password is too long (max %ld characters).", sizeof(password)-1);
      error = (-ENAMETOOLONG);
      goto out;
    }

    strlcpy(password,opt_password,sizeof(password));
  }

  if(strlen(opt_username) >= sizeof(username))
  {
    ReportError("User name '%s' is too long (max %ld characters).", username,sizeof(username)-1);
    error = (-ENAMETOOLONG);
    goto out;
  }

  strlcpy(username,opt_username,sizeof(username));
  StringToUpper(username);

  if (strlen(opt_workgroup) > 15)
  {
    ReportError("Workgroup/domain name '%s' is too long (max %ld characters).", opt_workgroup,15);
    error = (-ENAMETOOLONG);
    goto out;
  }

  strlcpy (workgroup, opt_workgroup, sizeof(workgroup));
  StringToUpper (workgroup);

  if(opt_servername != NULL)
  {
    if (strlen (opt_servername) > 16)
    {
      ReportError("Server name '%s' is too long (max %ld characters).", opt_servername,16);
      error = (-ENAMETOOLONG);
      goto out;
    }

    strlcpy (server_name, opt_servername, sizeof(server_name));
  }

  if(opt_clientname != NULL)
  {
    if (strlen (opt_clientname) > 16)
    {
      ReportError("Client name '%s' is too long (max %ld characters).", opt_clientname,16);
      error = (-ENAMETOOLONG);
      goto out;
    }

    strlcpy (client_name, opt_clientname, sizeof(client_name));
  }

  if(opt_cachesize < 1)
    opt_cachesize = DIRCACHE_SIZE;
  else if (opt_cachesize < 10)
    opt_cachesize = 10;

  strlcpy(par.server_ipname,server,sizeof(par.server_ipname));
  par.server_name = server_name;
  par.client_name = client_name;

  strlcpy(par.service,share,sizeof(par.service));
  par.username = username;
  par.password = password;
  par.max_xmit = max_xmit;

  error = smba_connect (&par, ipAddr, use_extended, workgroup, opt_cachesize, &the_server);
  if(error < 0)
  {
    ReportError("Could not connect to server (%ld, %s).",-error,amitcp_strerror(-error));
    goto out;
  }

  (*result) = the_server;

  error = 0;

 out:

  return(error);
}

/*****************************************************************************/

int
smba_get_dircache_size(struct smba_server * server)
{
  int result;

  result = server->dircache->cache_size;

  return(result);
}

/*****************************************************************************/

int
smba_change_dircache_size(struct smba_server * server,int cache_size)
{
  dircache_t * new_cache;
  dircache_t * the_dircache = server->dircache;
  int result;
  int i;

  result = the_dircache->cache_size;

  if(cache_size < 10)
    cache_size = 10;

  if(cache_size == the_dircache->cache_size)
    goto out;

  new_cache = malloc(sizeof(*new_cache) + (cache_size-1) * sizeof(new_cache->cache));
  if(new_cache == NULL)
    goto out;

  memset(new_cache,0,sizeof(*new_cache));
  new_cache->cache_size = cache_size;

  /* If the new cache is to be larger than the old one,
     allocate additional file name slots. */
  if(cache_size > the_dircache->cache_size)
  {
    for(i = the_dircache->cache_size ; i < cache_size ; i++)
    {
      new_cache->cache[i].complete_path = malloc (SMB_MAXNAMELEN + 1);
      if(new_cache->cache[i].complete_path == NULL)
      {
        int j;

        for(j = the_dircache->cache_size ; j < i ; j++)
          free(new_cache->cache[j].complete_path);

        free(new_cache);

        goto out;
      }

      new_cache->cache[i].complete_path_size = SMB_MAXNAMELEN + 1;
    }

    /* Reuse the file name buffers allocated for
       the old cache. */
    for(i = 0 ; i < the_dircache->cache_size ; i++)
    {
      new_cache->cache[i].complete_path = the_dircache->cache[i].complete_path;
      new_cache->cache[i].complete_path_size = the_dircache->cache[i].complete_path_size;

      the_dircache->cache[i].complete_path = NULL;
    }
  }
  else
  {
    /* Reuse the file name buffers allocated for
       the old cache. */
    for(i = 0 ; i < cache_size ; i++)
    {
      new_cache->cache[i].complete_path = the_dircache->cache[i].complete_path;
      new_cache->cache[i].complete_path_size = the_dircache->cache[i].complete_path_size;
      the_dircache->cache[i].complete_path = NULL;
    }
  }

  invalidate_dircache(server, NULL);

  for (i = 0; i < the_dircache->cache_size; i++)
  {
    if(the_dircache->cache[i].complete_path != NULL)
      free(the_dircache->cache[i].complete_path);
  }

  free(the_dircache);

  server->dircache = new_cache;
  result = cache_size;

 out:

  return(result);
}
