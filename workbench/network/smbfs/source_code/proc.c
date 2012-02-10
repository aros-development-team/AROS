/*
 * $Id: proc.c,v 1.9 2009/07/22 07:52:59 obarthel Exp $
 *
 * :ts=8
 *
 * proc.c
 *
 * Copyright (C) 1995 by Paal-Kr. Engstad and Volker Lendecke
 *
 * 28/06/96 - Fixed long file name support (smb_proc_readdir_long) by Yuri Per
 *
 * Modified for big endian support by Christian Starkjohann.
 * Modified for use with AmigaOS by Olaf Barthel <obarthel -at- gmx -dot- net>
 */

#include "smbfs.h"
#if !defined(__AROS__)
#include "quad_math.h"
#endif

/*****************************************************************************/

#include <smb/smb.h>
#include <smb/smbno.h>
#include <smb/smb_fs.h>

/*****************************************************************************/

#define SMB_VWV(packet)    ((packet) + SMB_HEADER_LEN)
#define SMB_CMD(packet)    ((packet)[8])
#define SMB_WCT(packet)    ((packet)[SMB_HEADER_LEN - 1])
#define SMB_BCC(packet)    smb_bcc(packet)
#define SMB_BUF(packet)    ((packet) + SMB_HEADER_LEN + SMB_WCT(packet) * 2 + 2)

#define SMB_DIRINFO_SIZE 43
#define SMB_STATUS_SIZE  21

/*****************************************************************************/

static INLINE byte * smb_encode_word(byte *p, word data);
static INLINE byte * smb_decode_word(byte *p, word *data);
static INLINE word smb_bcc(byte *packet);
static INLINE int smb_verify(byte *packet, int command, int wct, int bcc);
static byte *smb_encode_dialect(byte *p, const byte *name, int len);
static byte *smb_encode_ascii(byte *p, const byte *name, int len);
static void smb_encode_vblock(byte *p, const byte *data, word len, int unused_fs);
static byte *smb_decode_data(byte *p, byte *data, word *data_len, int fs);
static byte *smb_name_mangle(byte *p, const byte *name);
static int date_dos2unix(unsigned short time_value, unsigned short date);
static void date_unix2dos(int unix_date, unsigned short *time_value, unsigned short *date);
static INLINE int smb_valid_packet(byte *packet);
static int smb_errno(int errcls, int error);
static int smb_request_ok(struct smb_server *s, int command, int wct, int bcc);
static int smb_retry(struct smb_server *server);
static int smb_request_ok_unlock(struct smb_server *s, int command, int wct, int bcc);
static byte *smb_setup_header(struct smb_server *server, byte command, word wct, word bcc);
static byte *smb_setup_header_exclusive(struct smb_server *server, byte command, word wct, word bcc);
static int smb_proc_do_create(struct smb_server *server, const char *path, int len, struct smb_dirent *entry, word command);
static char *smb_decode_dirent(char *p, struct smb_dirent *entry);
static int smb_proc_readdir_short(struct smb_server *server, char *path, int fpos, int cache_size, struct smb_dirent *entry);
static char *smb_decode_long_dirent(char *p, struct smb_dirent *finfo, int level);
static int smb_proc_readdir_long(struct smb_server *server, char *path, int fpos, int cache_size, struct smb_dirent *entry);
static int smb_proc_reconnect(struct smb_server *server);
static void smb_printerr(int class, int num);

/*****************************************************************************/

/*****************************************************************************
 *
 *  Encoding/Decoding section
 *
 *****************************************************************************/
static INLINE byte *
smb_encode_word (byte * p, word data)
{
  p[0] = data & 0x00ffU;
  p[1] = (data & 0xff00U) >> 8;
  return &p[2];
}

static INLINE byte *
smb_decode_word (byte * p, word * data)
{
  (*data) = (word) p[0] | p[1] << 8;
  return &(p[2]);
}

byte *
smb_encode_smb_length (byte * p, dword len)
{
  p[0] = p[1] = 0;
  p[2] = (len & 0xFF00) >> 8;
  p[3] = (len & 0xFF);

  if (len > 0xFFFF)
    p[1] |= 0x01;

  return &p[4];
}

static byte *
smb_encode_dialect (byte * p, const byte * name, int len)
{
  (*p++) = 2;
  strcpy (p, name);

  return p + len + 1;
}

static byte *
smb_encode_ascii (byte * p, const byte * name, int len)
{
  (*p++) = 4;
  strcpy (p, name);

  return p + len + 1;
}

static void
smb_encode_vblock (byte * p, const byte * data, word len, int unused_fs)
{
  (*p++) = 5;
  p = smb_encode_word (p, len);
  memcpy (p, data, len);
}

static byte *
smb_decode_data (byte * p, byte * data, word * data_len, int unused_fs)
{
  word len;

  if (!((*p) == 1 || (*p) == 5))
    LOG (("Warning! Data block not starting with 1 or 5\n"));

  len = WVAL (p, 1);
  p += 3;

  memcpy (data, p, len);

  (*data_len) = len;

  return p + len;
}

static byte *
smb_name_mangle (byte * p, const byte * name)
{
  int len, pad = 0;

  len = strlen (name);

  if (len < 16)
    pad = 16 - len;

  (*p++) = 2 * (len + pad);

  while ((*name) != '\0')
  {
    (*p++) = ((*name) >> 4) + 'A';
    (*p++) = ((*name) & 0x0F) + 'A';

    name++;
  }

  while (pad-- > 0)
  {
    (*p++) = 'C';
    (*p++) = 'A';
  }

  (*p++) = '\0';

  return p;
}

/* According to the core protocol documentation times are
   expressed as seconds past January 1st, 1970, local
   time zone. */
static INLINE int
utc2local (int time_value)
{
  int result;

  if(time_value > 0)
    result = time_value - GetTimeZoneDelta();
  else
    result = time_value;

  return result;
}

static INLINE int
local2utc (int time_value)
{
  int result;

  if(time_value > 0)
    result = time_value + GetTimeZoneDelta();
  else
    result = time_value;

  return result;
}

/* Convert a MS-DOS time/date pair to a UNIX date (seconds since January 1st 1970). */
static int
date_dos2unix (unsigned short time_value, unsigned short date)
{
  time_t seconds;
  struct tm tm;

  memset(&tm,0,sizeof(tm));

  tm.tm_sec  = 2 * (time_value & 0x1F);
  tm.tm_min  = (time_value >> 5) & 0x3F;
  tm.tm_hour = (time_value >> 11) & 0x1F;
  tm.tm_mday = date & 0x1F;
  tm.tm_mon  = ((date >> 5) & 0xF) - 1;
  tm.tm_year = ((date >> 9) & 0x7F) + 80;

  seconds = MakeTime(&tm);

  return(seconds);
}

/* Convert linear UNIX date to a MS-DOS time/date pair. */
static void
date_unix2dos (int unix_date, unsigned short *time_value, unsigned short *date)
{
  struct tm tm;

  GMTime(unix_date,&tm);

  (*time_value) = (tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_sec / 2);
  (*date) = ((tm.tm_year - 80) << 9) | ((tm.tm_mon + 1) << 5) | tm.tm_mday;
}

/****************************************************************************
 *
 *  Support section.
 *
 ****************************************************************************/
dword
smb_len (byte * packet)
{
  return (dword)( (((dword)(packet[1] & 0x1)) << 16) | (((dword)packet[2]) << 8) | (packet[3]) );
}

static INLINE word
smb_bcc (byte * packet)
{
  int pos = SMB_HEADER_LEN + SMB_WCT (packet) * sizeof (word);

  return (word)(packet[pos] | packet[pos + 1] << 8);
}

/* smb_valid_packet: We check if packet fulfills the basic
   requirements of a smb packet */
static INLINE int
smb_valid_packet (byte * packet)
{
  int result;

  LOG (("len: %ld, wct: %ld, bcc: %ld -- SMB_HEADER_LEN[%ld] + SMB_WCT (packet) * 2[%ld] + SMB_BCC (packet)[%ld] + 2 = %ld\n",
	smb_len (packet),
	SMB_WCT (packet),
	SMB_BCC (packet),
	SMB_HEADER_LEN,
	SMB_WCT (packet) * 2,
	SMB_BCC (packet),
	SMB_HEADER_LEN + SMB_WCT (packet) * 2 + SMB_BCC (packet) + 2));

  /* olsen: During protocol negotiation Samba 3.2.5 may return SMB responses which contain
            more data than is called for. I'm not sure what the right approach to handle
            this would be. But for now, I've modified this test to check only if there
            is less data in a response than required. */
  result = (packet[4] == 0xff
         && packet[5] == 'S'
         && packet[6] == 'M'
         && packet[7] == 'B'
         && (smb_len (packet) + 4 >= (dword)
             (SMB_HEADER_LEN + SMB_WCT (packet) * 2 + SMB_BCC (packet) + 2))) ? 0 : (-EIO);

  return result;
}

/* smb_verify: We check if we got the answer we expected, and if we
   got enough data. If bcc == -1, we don't care. */
static INLINE int
smb_verify (byte * packet, int command, int wct, int bcc)
{
  return (SMB_CMD (packet) == command &&
          SMB_WCT (packet) >= wct &&
          (bcc == -1 || SMB_BCC (packet) >= bcc)) ? 0 : -EIO;
}

static int
smb_errno (int errcls, int error)
{
  int result,i;

  if (errcls)
    smb_printerr (errcls, error);

  if (errcls == ERRDOS)
  {
    static const int map[][2] =
    {
      { ERRbadfunc,EINVAL },
      { ERRbadfile,ENOENT },
      { ERRbadpath,ENOENT },
      { ERRnofids,EMFILE },
      { ERRnoaccess,EACCES },
      { ERRbadfid,EBADF },
      { ERRbadmcb,EIO },
      { ERRnomem,ENOMEM },
      { ERRbadmem,EFAULT },
      { ERRbadenv,EIO },
      { ERRbadformat,EIO },
      { ERRbadaccess,EACCES },
      { ERRbaddata,E2BIG },
      { ERRbaddrive,ENXIO },
      { ERRremcd,EIO },
      { ERRdiffdevice,EXDEV },
      { ERRnofiles,0 },
      { ERRbadshare,ETXTBSY },
      { ERRlock,EDEADLK },
      { ERRfilexists,EEXIST },
      { 87,0 },/* Unknown error! */
      { 183,EEXIST },/* This next error seems to occur on an mv when
                        the destination exists */
      { -1,-1 }
    };

    result = EIO;

    for(i = 0 ; map[i][0] != -1 ; i++)
    {
      if(map[i][0] == error)
      {
        result = map[i][1];
        break;
      }
    }
  }
  else if (errcls == ERRSRV)
  {
    static const int map[][2] =
    {
      { ERRerror,ENFILE },
      { ERRbadpw,EINVAL },
      { ERRbadtype,EIO },
      { ERRaccess,EACCES },
      { -1, -1 }
    };

    result = EIO;

    for(i = 0 ; map[i][0] != -1 ; i++)
    {
      if(map[i][0] == error)
      {
        result = map[i][1];
        break;
      }
    }
  }
  else if (errcls == ERRHRD)
  {
    static const int map[][2] =
    {
      { ERRnowrite,EROFS },
      { ERRbadunit,ENODEV },
      { ERRnotready,EBUSY },
      { ERRbadcmd,EIO },
      { ERRdata,EIO },
      { ERRbadreq,ERANGE },
      { ERRbadshare,ETXTBSY },
      { ERRlock,EDEADLK },
      { -1, -1 }
    };

    result = EIO;

    for(i = 0 ; map[i][0] != -1 ; i++)
    {
      if(map[i][0] == error)
      {
        result = map[i][1];
        break;
      }
    }
  }
  else if (errcls == ERRCMD)
  {
    result = EIO;
  }
  else
  {
    result = 0;
  }

  return(result);
}

#if DEBUG

static char
print_char (char c)
{
  if ((c < ' ') || (c > '~'))
    return '.';

  return c;
}

static void
smb_dump_packet (byte * packet)
{
  int i, j, len;
  int errcls, error;

  errcls = (int) packet[9];
  error = (int) (int) (packet[11] | packet[12] << 8);

  LOG (("smb_len = %ld  valid = %ld    \n", len = smb_len (packet), smb_valid_packet (packet)));
  LOG (("smb_cmd = %ld  smb_wct = %ld  smb_bcc = %ld\n", packet[8], SMB_WCT (packet), SMB_BCC (packet)));
  LOG (("smb_rcls = %ld smb_err = %ld\n", errcls, error));

  if (errcls)
    smb_printerr (errcls, error);

  if (len > 100)
    len = 100;

  PRINTHEADER();

  for (i = 0; i < len; i += 10)
  {
    PRINTF (("%03ld:", i));

    for (j = i; j < i + 10; j++)
    {
      if (j < len)
        PRINTF (("%02lx ", packet[j]));
      else
        PRINTF (("   "));
    }

    PRINTF ((": "));

    for (j = i; j < i + 10; j++)
    {
      if (j < len)
        PRINTF (("%lc", print_char (packet[j])));
    }

    PRINTF (("\n"));
  }
}

#endif /* DEBUG */

/* smb_request_ok: We expect the server to be locked. Then we do the
   request and check the answer completely. When smb_request_ok
   returns 0, you can be quite sure that everything went well. When
   the answer is <=0, the returned number is a valid unix errno. */
static int
smb_request_ok (struct smb_server *s, int command, int wct, int bcc)
{
  int result;
  int error;

  s->rcls = 0;
  s->err = 0;

  result = smb_request (s);
  if (result < 0)
  {
    LOG (("smb_request failed\n"));
  }
  else if ((error = smb_valid_packet (s->packet)) != 0)
  {
    LOG (("not a valid packet!\n"));
    result = error;
  }
  else if (s->rcls != 0)
  {
    result = -smb_errno (s->rcls, s->err);
  }
  else if ((error = smb_verify (s->packet, command, wct, bcc)) != 0)
  {
    LOG (("smb_verify failed\n"));
    result = error;
  }

  return(result);
}

/* smb_retry: This function should be called when smb_request_ok has
   indicated an error. If the error was indicated because the
   connection was killed, we try to reconnect. If smb_retry returns 0,
   the error was indicated for another reason, so a retry would not be
   of any use. */
static int
smb_retry (struct smb_server *server)
{
  int result = 0;

  if (server->state == CONN_VALID)
    goto out;

  if (smb_release (server) < 0)
  {
    LOG (("smb_retry: smb_release failed\n"));
    server->state = CONN_RETRIED;
    goto out;
  }

  if (smb_proc_reconnect (server) < 0)
  {
    LOG (("smb_proc_reconnect failed\n"));
    server->state = CONN_RETRIED;
    goto out;
  }

  server->state = CONN_VALID;
  result = 1;

 out:

  return result;
}

static int
smb_request_ok_unlock (struct smb_server *s, int command, int wct, int bcc)
{
  int result;

  result = smb_request_ok (s, command, wct, bcc);

  return result;
}

/* smb_setup_header: We completely set up the packet. You only have to
   insert the command-specific fields */
static byte *
smb_setup_header (struct smb_server *server, byte command, word wct, word bcc)
{
  dword xmit_len = SMB_HEADER_LEN + wct * sizeof (word) + bcc + 2;
  byte *p = server->packet;
  byte *buf = server->packet;

  /* olsen: we subtract four bytes because smb_encode_smb_length() adds
            four bytes which are not supposed to be included in the total
            number of bytes to be sent */
  p = smb_encode_smb_length (p, xmit_len - 4);
  /* p = smb_encode_smb_length (p, xmit_len); */

  BSET (p, 0, 0xff);
  BSET (p, 1, 'S');
  BSET (p, 2, 'M');
  BSET (p, 3, 'B');
  BSET (p, 4, command);

  p += 5;
  memset (p, '\0', 19);
  p += 19;
  p += 8;

  WSET (buf, smb_tid, server->tid);
  WSET (buf, smb_pid, 0); /* server->pid */
  WSET (buf, smb_uid, server->server_uid);
  WSET (buf, smb_mid, 0); /* server->mid */

  if (server->protocol > PROTOCOL_CORE)
  {
    BSET (buf, smb_flg, 0x8);
    WSET (buf, smb_flg2, 0x3);
  }

  (*p++) = wct; /* wct */
  p += 2 * wct;
  WSET (p, 0, bcc);

  return p + 2;
}

/* smb_setup_header_exclusive waits on server->lock and locks the
   server, when it's free. You have to unlock it manually when you're
   finished with server->packet! */
static byte *
smb_setup_header_exclusive (struct smb_server *server, byte command, word wct, word bcc)
{
  byte * result;

  result = smb_setup_header (server, command, wct, bcc);

  return result;
}

/*****************************************************************************
 *
 *  File operation section.
 *
 ****************************************************************************/

int
smb_proc_open (struct smb_server *server, const char *pathname, int len, struct smb_dirent *entry)
{
  int error;
  char *p;
  char *buf = server->packet;
  const word o_attr = aSYSTEM | aHIDDEN | aDIR;

  LOG (("path=%s\n", pathname));

 retry:

  p = smb_setup_header (server, SMBopen, 2, 2 + len);
  WSET (buf, smb_vwv0, 0x42); /* read/write */
  WSET (buf, smb_vwv1, o_attr);
  smb_encode_ascii (p, pathname, len);

  if ((error = smb_request_ok (server, SMBopen, 7, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;

    if (error != -EACCES)
      goto out;

    p = smb_setup_header (server, SMBopen, 2, 2 + len);
    WSET (buf, smb_vwv0, 0x40); /* read only */
    WSET (buf, smb_vwv1, o_attr);
    smb_encode_ascii (p, pathname, len);

    if ((error = smb_request_ok (server, SMBopen, 7, 0)) < 0)
    {
      if (smb_retry (server))
        goto retry;

      goto out;
    }
  }

  /* We should now have data in vwv[0..6]. */
  entry->fileid = WVAL (buf, smb_vwv0);
  entry->attr = WVAL (buf, smb_vwv1);
  entry->ctime = entry->atime = entry->mtime = entry->wtime = local2utc (DVAL (buf, smb_vwv2));
  entry->size = DVAL (buf, smb_vwv4);
  entry->opened = 1;

  #if DEBUG
  {
    struct tm tm;

    GMTime(entry->ctime,&tm);
    LOG(("ctime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->atime,&tm);
    LOG(("atime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->mtime,&tm);
    LOG(("mtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->wtime,&tm);
    LOG(("wtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));
  }
  #endif /* DEBUG */

 out:

  return error;
}

/* smb_proc_close: in finfo->mtime we can send a modification time to
   the server */
int
smb_proc_close (struct smb_server *server, word fileid, dword mtime)
{
  char *buf = server->packet;
  int local_time;
  int result;

  if(mtime != 0 && mtime != 0xffffffff)
  {
    /* 0 and 0xffffffff mean: do not set mtime */
    local_time = utc2local (mtime);
  }
  else
  {
    local_time = mtime;
  }

  smb_setup_header_exclusive (server, SMBclose, 3, 0);
  WSET (buf, smb_vwv0, fileid);
  DSET (buf, smb_vwv1, local_time);

  result = smb_request_ok_unlock (server, SMBclose, 0, 0);

  return result;
}

/* In smb_proc_read and smb_proc_write we do not retry, because the
   file-id would not be valid after a reconnection. */

/* smb_proc_read: fs indicates if it should be copied with
   memcpy_tofs. */
int
smb_proc_read (struct smb_server *server, struct smb_dirent *finfo, off_t offset, long count, char *data, int fs)
{
  word returned_count, data_len;
  char *buf = server->packet;
  int result;
  int error;

  smb_setup_header_exclusive (server, SMBread, 5, 0);

  WSET (buf, smb_vwv0, finfo->fileid);
  WSET (buf, smb_vwv1, count);
  DSET (buf, smb_vwv2, offset);
  WSET (buf, smb_vwv4, 0);

  if ((error = smb_request_ok (server, SMBread, 5, -1)) < 0)
  {
    result = error;
    goto out;
  }

  returned_count = WVAL (buf, smb_vwv0);

  smb_decode_data (SMB_BUF (server->packet), data, &data_len, fs);

  if (returned_count != data_len)
  {
    LOG (("Warning, returned_count != data_len\n"));
    LOG (("ret_c=%ld, data_len=%ld\n", returned_count, data_len));
  }
  else
  {
    LOG (("ret_c=%ld, data_len=%ld\n", returned_count, data_len));
  }

  result = data_len;

 out:

  return result;
}

/* count must be <= 65535. No error number is returned.  A result of 0
   indicates an error, which has to be investigated by a normal read
   call. */
int
smb_proc_read_raw (struct smb_server *server, struct smb_dirent *finfo, off_t offset, long count, char *data)
{
  char *buf = server->packet;
  int result;

  smb_setup_header_exclusive (server, SMBreadbraw, 8, 0);

  WSET (buf, smb_vwv0, finfo->fileid);
  DSET (buf, smb_vwv1, offset);
  WSET (buf, smb_vwv3, count);
  WSET (buf, smb_vwv4, 0);
  DSET (buf, smb_vwv5, 0);

  result = smb_request_read_raw (server, data, count);

  return result;
}

int
smb_proc_write (struct smb_server *server, struct smb_dirent *finfo, off_t offset, long count, const char *data)
{
  int res;
  char *buf = server->packet;
  byte *p;

  p = smb_setup_header_exclusive (server, SMBwrite, 5, count + 3);
  WSET (buf, smb_vwv0, finfo->fileid);
  WSET (buf, smb_vwv1, count);
  DSET (buf, smb_vwv2, offset);
  WSET (buf, smb_vwv4, 0);

  (*p++) = 1;
  WSET (p, 0, count);
  memcpy (p + 2, data, count);

  if ((res = smb_request_ok (server, SMBwrite, 1, 0)) >= 0)
    res = WVAL (buf, smb_vwv0);

  return res;
}

/* count must be <= 65535 */
int
smb_proc_write_raw (struct smb_server *server, struct smb_dirent *finfo, off_t offset, long count, const char *data)
{
  char *buf = server->packet;
  int result;
  long len = server->max_xmit - 4;
  byte *p;

  if (len >= count)
    len = 0;
  else
    len = count - len;  /* transfer the larger part using the second packet */

  p = smb_setup_header_exclusive (server, SMBwritebraw, server->protocol > PROTOCOL_COREPLUS ? 12 : 11, len);

  WSET (buf, smb_vwv0, finfo->fileid);
  DSET (buf, smb_vwv1, count);
  DSET (buf, smb_vwv3, offset);
  DSET (buf, smb_vwv5, 0);      /* timeout */
  WSET (buf, smb_vwv7, 1);      /* send final result response */
  DSET (buf, smb_vwv8, 0);      /* reserved */

  if (server->protocol > PROTOCOL_COREPLUS)
  {
    WSET (buf, smb_vwv10, len);
    WSET (buf, smb_vwv11, p - smb_base(buf));
  }

  if (len > 0)
    memcpy (p, data, len);

  result = smb_request_ok (server, SMBwritebraw, 1, 0);

  LOG (("first request returned %ld\n", result));

  if (result < 0)
    goto out;

  result = smb_request_write_raw (server, data + len, count - len);

  LOG(("raw request returned %ld\n", result));

  if (result > 0)
  {
    int error;

    /* We have to do the checks of smb_request_ok here as well */
    if ((error = smb_valid_packet (server->packet)) != 0)
    {
      LOG (("not a valid packet!\n"));
      result = error;
    }
    else if (server->rcls != 0)
    {
      result = -smb_errno (server->rcls, server->err);
    }
    else if ((error = smb_verify (server->packet, SMBwritec, 1, 0)) != 0)
    {
      LOG (("smb_verify failed\n"));
      result = error;
    }

    /* If everything went fine, the whole block has been transfered. */
    if (result == (count - len))
      result = count;
  }

 out:

  return result;
}

int
smb_proc_lseek (struct smb_server *server, struct smb_dirent *finfo, off_t offset, int mode, off_t * new_position_ptr)
{
  char *buf = server->packet;
  int error;

 retry:

  smb_setup_header (server, SMBlseek, 4,0);

  WSET (buf, smb_vwv0, finfo->fileid);
  WSET (buf, smb_vwv1, mode);
  DSET (buf, smb_vwv2, offset);

  if ((error = smb_request_ok (server, SMBlseek, 1, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;
  }
  else
  {
    (*new_position_ptr) = DVAL(buf, smb_vwv0);
  }

  return error;
}

/* smb_proc_lockingX: We don't chain any further packets to the initial one  */
int
smb_proc_lockingX (struct smb_server *server, struct smb_dirent *finfo, struct smb_lkrng *locks, int num_entries, int mode, long timeout)
{
  int result;
  int num_locks, num_unlocks;
  char *buf = server->packet;
  char *data;
  struct smb_lkrng *p;
  int i;

  num_locks = num_unlocks = 0;

  if (mode & 2)
    num_unlocks = num_entries;
  else
    num_locks = num_entries;

 retry:

  data = smb_setup_header(server, SMBlockingX, 8, num_entries * 10);

  BSET (buf, smb_vwv0, 0xFF);
  WSET (buf, smb_vwv1, 0);
  WSET (buf, smb_vwv2, finfo->fileid);
  BSET (buf, smb_vwv3, mode & 1);
  DSET (buf, smb_vwv4, timeout);
  WSET (buf, smb_vwv6, num_unlocks);
  WSET (buf, smb_vwv7, num_locks);

  for (i = 0, p = locks; i < num_entries; i++, p++)
  {
    WSET (data, SMB_LPID_OFFSET(i), 0);  /* server->pid */
    DSET (data, SMB_LKOFF_OFFSET(i), p->offset);
    DSET (data, SMB_LKLEN_OFFSET(i), p->len);
  }

  if ((result = smb_request_ok (server, SMBlockingX, 0, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;
  }
  
  return result;
}

/* smb_proc_do_create: We expect entry->attry & entry->ctime to be set. */
static int
smb_proc_do_create (struct smb_server *server, const char *path, int len, struct smb_dirent *entry, word command)
{
  int error;
  char *p;
  char *buf = server->packet;
  int local_time;

 retry:

  p = smb_setup_header (server, command, 3, len + 2);
  WSET (buf, smb_vwv0, entry->attr);
  local_time = utc2local (entry->ctime);
  DSET (buf, smb_vwv1, local_time);
  smb_encode_ascii (p, path, len);

  if ((error = smb_request_ok (server, command, 1, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;

    goto out;
  }

  entry->opened = 1;
  entry->fileid = WVAL (buf, smb_vwv0);

  smb_proc_close (server, entry->fileid, entry->mtime);

 out:

  return error;
}

int
smb_proc_create (struct smb_server *server, const char *path, int len, struct smb_dirent *entry)
{
  return smb_proc_do_create (server, path, len, entry, SMBcreate);
}

int
smb_proc_mv (struct smb_server *server, const char *opath, const int olen, const char *npath, const int nlen)
{
  char *p;
  char *buf = server->packet;
  int result;

 retry:

  p = smb_setup_header (server, SMBmv, 1, olen + nlen + 4);

  WSET (buf, smb_vwv0, 0);

  p = smb_encode_ascii (p, opath, olen);
  smb_encode_ascii (p, npath, olen);

  if ((result = smb_request_ok (server, SMBmv, 0, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;
  }

  return result;
}

int
smb_proc_mkdir (struct smb_server *server, const char *path, const int len)
{
  char *p;
  int result;

 retry:

  p = smb_setup_header (server, SMBmkdir, 0, 2 + len);

  smb_encode_ascii (p, path, len);

  if ((result = smb_request_ok (server, SMBmkdir, 0, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;
  }

  return result;
}

int
smb_proc_rmdir (struct smb_server *server, const char *path, const int len)
{
  char *p;
  int result;

 retry:

  p = smb_setup_header (server, SMBrmdir, 0, 2 + len);

  smb_encode_ascii (p, path, len);

  if ((result = smb_request_ok (server, SMBrmdir, 0, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;
  }

  return result;
}

int
smb_proc_unlink (struct smb_server *server, const char *path, const int len)
{
  char *p;
  char *buf = server->packet;
  int result;

 retry:

  p = smb_setup_header (server, SMBunlink, 1, 2 + len);

  WSET (buf, smb_vwv0, 0);

  smb_encode_ascii (p, path, len);

  if ((result = smb_request_ok (server, SMBunlink, 0, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;
  }

  return result;
}

int
smb_proc_trunc (struct smb_server *server, word fid, dword length)
{
  char *p;
  char *buf = server->packet;
  int result;

  p = smb_setup_header (server, SMBwrite, 5, 3);
  WSET (buf, smb_vwv0, fid);
  WSET (buf, smb_vwv1, 0);
  DSET (buf, smb_vwv2, length);
  WSET (buf, smb_vwv4, 0);
  smb_encode_ascii (p, "", 0);

  result = smb_request_ok (server, SMBwrite, 1, 0);
  if (result >= 0)
    result = DVAL(buf, smb_vwv0);

  return result;
}

static char *
smb_decode_dirent (char *p, struct smb_dirent *entry)
{
  size_t name_size;

  p += SMB_STATUS_SIZE; /* reserved (search_status) */

  entry->attr = BVAL (p, 0);
  entry->mtime = entry->atime = entry->ctime = entry->wtime = date_dos2unix (WVAL (p, 1), WVAL (p, 3));
  entry->size = DVAL (p, 5);

  name_size = 13;

  if(name_size > entry->complete_path_size-1)
    name_size = entry->complete_path_size-1;

  memcpy (entry->complete_path, p + 9, name_size);

  entry->complete_path[name_size] = '\0';

  LOG (("smb_decode_dirent: path = %s\n", entry->complete_path));

  #if DEBUG
  {
    struct tm tm;

    GMTime(entry->ctime,&tm);
    LOG(("ctime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->atime,&tm);
    LOG(("atime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->mtime,&tm);
    LOG(("mtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->wtime,&tm);
    LOG(("wtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));
  }
  #endif /* DEBUG */

  return p + 22;
}

/* This routine is used to read in directory entries from the network.
   Note that it is for short directory name seeks, i.e.: protocol < PROTOCOL_LANMAN2 */
static int
smb_proc_readdir_short (struct smb_server *server, char *path, int fpos, int cache_size, struct smb_dirent *entry)
{
  char *p;
  char *buf;
  int error;
  int result = 0;
  int i;
  int first, total_count;
  struct smb_dirent *current_entry;
  word bcc;
  word count;
  char status[SMB_STATUS_SIZE];
  int entries_asked = (server->max_xmit - 100) / SMB_DIRINFO_SIZE;
  int dirlen = strlen (path);
  char * mask;

  mask = malloc(dirlen + 4 + 1);
  if (mask == NULL)
  {
     result = (-ENOMEM);
     goto out;
  }

  strcpy (mask, path);
  strcat (mask, "\\*.*");

  LOG (("SMB call  readdir %ld @ %ld\n", cache_size, fpos));
  LOG (("          mask = %s\n", mask));

  buf = server->packet;

 retry:

  first = 1;
  total_count = 0;
  current_entry = entry;

  while (1)
  {
    if (first == 1)
    {
      p = smb_setup_header (server, SMBsearch, 2, 5 + strlen (mask));
      WSET (buf, smb_vwv0, entries_asked);
      WSET (buf, smb_vwv1, aDIR);
      p = smb_encode_ascii (p, mask, strlen (mask));
      (*p++) = 5;
      (void) smb_encode_word (p, 0);
    }
    else
    {
      p = smb_setup_header (server, SMBsearch, 2, 5 + SMB_STATUS_SIZE);
      WSET (buf, smb_vwv0, entries_asked);
      WSET (buf, smb_vwv1, aDIR);
      p = smb_encode_ascii (p, "", 0);
      (void) smb_encode_vblock (p, status, SMB_STATUS_SIZE, 0);
    }

    if ((error = smb_request_ok (server, SMBsearch, 1, -1)) < 0)
    {
      if ((server->rcls == ERRDOS) && (server->err == ERRnofiles))
      {
        result = total_count - fpos;
        goto unlock_return;
      }
      else
      {
        if (smb_retry (server))
          goto retry;

        result = error;

        goto unlock_return;
      }
    }

    p = SMB_VWV (server->packet);
    p = smb_decode_word (p, &count); /* vwv[0] = count-returned */
    p = smb_decode_word (p, &bcc);

    first = 0;

    if (count <= 0)
    {
      result = total_count - fpos;

      goto unlock_return;
    }

    if (bcc != count * SMB_DIRINFO_SIZE + 3)
    {
      result = -EIO;

      goto unlock_return;
    }

    p += 3; /* Skipping VBLOCK header (5, length lo, length hi). */

    /* Read the last entry into the status field. */
    memcpy (status, SMB_BUF (server->packet) + 3 + (count - 1) * SMB_DIRINFO_SIZE, SMB_STATUS_SIZE);

    /* Now we are ready to parse smb directory entries. */

    for (i = 0; i < count; i++)
    {
      if (total_count < fpos)
      {
        p += SMB_DIRINFO_SIZE;

        LOG (("smb_proc_readdir: skipped entry; total_count = %ld, i = %ld, fpos = %ld\n", total_count, i, fpos));
      }
      else if (total_count >= fpos + cache_size)
      {
        result = total_count - fpos;

        goto unlock_return;
      }
      else
      {
        p = smb_decode_dirent (p, current_entry);

        current_entry += 1;
      }

      total_count += 1;
    }
  }

 unlock_return:

  if(mask != NULL)
    free(mask);

 out:

  return result;
}

/*****************************************************************************/

#if !defined(__AROS__)
/* Interpret an 8 byte "filetime" structure to a 'time_t'.
 * It's originally in "100ns units since jan 1st 1601".
 *
 * Unlike the Samba implementation of that date conversion
 * algorithm this one tries to perform the entire
 * calculation using integer operations only.
 */
static time_t
interpret_long_date(char * p)
{
  QUAD adjust;
  QUAD long_date;
  ULONG underflow;
  time_t result;

  /* Extract the 64 bit time value. */
  long_date.Low  = DVAL(p,0);
  long_date.High = DVAL(p,4);

  /* Divide by 10,000,000 to convert the time from 100ns
     units into seconds. */
  divide_64_by_32(&long_date,10000000,&long_date);

  /* Adjust by 369 years (11,644,473,600 seconds) to convert
     from the epoch beginning on January 1st 1601 to the one
     beginning on January 1st 1970 (the Unix epoch). */
  adjust.Low  = 0xb6109100;
  adjust.High = 0x00000002;

  underflow = subtract_64_from_64_to_64(&long_date,&adjust,&long_date);

  /* If the result did not produce an underflow or overflow,
     return the number of seconds encoded in the least
     significant word of the result. */
  if(underflow == 0 && long_date.High == 0)
    result = long_date.Low;
  else
    result = 0;

  return(result);
}
#else
static time_t
interpret_long_date(char * p)
{
    QUAD long_date;
    long_date  = ((QUAD)DVAL(p,0) << 32) | DVAL(p,4);

    long_date -= 116444736000000000ULL;
    long_date /= 10000000;
    
    return((time_t)long_date & 0xFFFFFFFF);
}
#endif

/*****************************************************************************/

static void
smb_get_dirent_name(char *p,int level,char ** name_ptr,int * len_ptr)
{
  switch (level)
  {
    case 1: /* OS/2 understands this */
      (*name_ptr) = p + 27;
      (*len_ptr) = strlen(p + 27);
      break;

    case 2: /* this is what OS/2 uses */
      (*name_ptr) = p + 31;
      (*len_ptr) = strlen(p + 31);
      break;

    case 3: /* untested */
      (*name_ptr) = p + 33;
      (*len_ptr) = strlen(p + 33);
      break;

    case 4: /* untested */
      (*name_ptr) = p + 37;
      (*len_ptr) = strlen(p + 37);
      break;

    case 260: /* NT uses this, but also accepts 2 */
      (*name_ptr) = p + 94;
      (*len_ptr) = min (DVAL (p+60, 0), SMB_MAXNAMELEN);
      break;

    default:
      (*name_ptr) = NULL;
      (*len_ptr) = 0;
      break;
  }
}

/* interpret a long filename structure - this is mostly guesses at the
   moment.  The length of the structure is returned.  The structure of
   a long filename depends on the info level. 260 is used by NT and 2
   is used by OS/2. */
static char *
smb_decode_long_dirent (char *p, struct smb_dirent *finfo, int level)
{
  char *result;

  switch (level)
  {
    case 1: /* OS/2 understands this */

      #if DEBUG
      {
        char buffer[255];

        memcpy(buffer,p + 27,sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';

        LOG(("type=%ld, name='%s'\n",level,buffer));
      }
      #endif /* DEBUG */

      if (finfo != NULL)
      {
        strlcpy (finfo->complete_path, p + 27, finfo->complete_path_size);
        finfo->len = strlen (finfo->complete_path);
        finfo->size = DVAL (p, 16);
        finfo->attr = BVAL (p, 24);
        finfo->ctime = date_dos2unix (WVAL (p, 6), WVAL (p, 4));
        finfo->atime = date_dos2unix (WVAL (p, 10), WVAL (p, 8));
        finfo->mtime = date_dos2unix (WVAL (p, 14), WVAL (p, 12));
        finfo->wtime = finfo->mtime;

        #if DEBUG
        {
          struct tm tm;

          GMTime(finfo->ctime,&tm);
          LOG(("ctime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

          GMTime(finfo->atime,&tm);
          LOG(("atime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

          GMTime(finfo->mtime,&tm);
          LOG(("mtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

          GMTime(finfo->wtime,&tm);
          LOG(("wtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));
        }
        #endif /* DEBUG */
      }

      result = p + 28 + BVAL (p, 26);

      break;

    case 2: /* this is what OS/2 uses */

      #if DEBUG
      {
        char buffer[255];

        memcpy(buffer,p + 31,sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';

        LOG(("type=%ld, name='%s'\n",level,buffer));
      }
      #endif /* DEBUG */

      if (finfo != NULL)
      {
        strlcpy (finfo->complete_path, p + 31, finfo->complete_path_size);
        finfo->len = strlen (finfo->complete_path);
        finfo->size = DVAL (p, 16);
        finfo->attr = BVAL (p, 24);
        finfo->ctime = date_dos2unix (WVAL (p, 6), WVAL (p, 4));
        finfo->atime = date_dos2unix (WVAL (p, 10), WVAL (p, 8));
        finfo->mtime = date_dos2unix (WVAL (p, 14), WVAL (p, 12));
        finfo->wtime = finfo->mtime;

        #if DEBUG
        {
          struct tm tm;

          GMTime(finfo->ctime,&tm);
          LOG(("ctime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

          GMTime(finfo->atime,&tm);
          LOG(("atime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

          GMTime(finfo->mtime,&tm);
          LOG(("mtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

          GMTime(finfo->wtime,&tm);
          LOG(("wtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));
        }
        #endif /* DEBUG */
      }

      result = p + 32 + BVAL (p, 30);

      break;

    case 260: /* NT uses this, but also accepts 2 */

      #if DEBUG
      {
        char buffer[255];
        int len;

        len = min (DVAL (p+60, 0), sizeof(buffer)-1);

        memcpy(buffer,p+94,len);
        buffer[len] = '\0';

        LOG(("type=%ld, name='%s'\n",level,buffer));
      }
      #endif /* DEBUG */

      result = p + WVAL (p, 0);

      if (finfo != NULL)
      {
        int namelen;
        time_t swap;

        p += 4;                   /* next entry offset */
        p += 4;                   /* fileindex */
        finfo->ctime = interpret_long_date(p);
        p += 8;
        finfo->atime = interpret_long_date(p);
        p += 8;
        finfo->wtime = interpret_long_date(p);
        p += 8;
        finfo->mtime = interpret_long_date(p);
        p += 8;
        finfo->size = DVAL (p, 0);
        p += 8;
        p += 8;                   /* alloc size */
        finfo->attr = BVAL (p, 0);
        p += 4;
        namelen = min (DVAL (p, 0), SMB_MAXNAMELEN);
        p += 4;
        p += 4;                   /* EA size */
        p += 2;                   /* short name len? */
        p += 24;                  /* short name? */

        if(namelen > (int)finfo->complete_path_size-1)
          namelen = finfo->complete_path_size-1;

        /* If the modification time is not set, try to
           substitute the write time for it. */
        if(finfo->mtime == 0)
          finfo->mtime = finfo->wtime;

        /* Swap last modification time and last write time. */
        swap = finfo->mtime;
        finfo->mtime = finfo->wtime;
        finfo->wtime = swap;

        memcpy (finfo->complete_path, p, namelen);
        finfo->complete_path[namelen] = '\0';
        finfo->len = namelen;

        #if DEBUG
        {
          struct tm tm;

          GMTime(finfo->ctime,&tm);
          LOG(("ctime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

          GMTime(finfo->atime,&tm);
          LOG(("atime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

          GMTime(finfo->wtime,&tm);
          LOG(("wtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

          GMTime(finfo->mtime,&tm);
          LOG(("mtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));
        }
        #endif /* DEBUG */
      }

      break;

    default:

      if (finfo != NULL)
      {
        /* I have to set times to 0 here, because I do not
           have specs about this for all info levels. */
        finfo->ctime = finfo->mtime = finfo->wtime = finfo->atime = 0;
      }

      LOG (("Unknown long filename format %ld\n", level));

      result = p + WVAL (p, 0);

      break;
  }

  return result;
}

static int
smb_proc_readdir_long (struct smb_server *server, char *path, int fpos, int cache_size, struct smb_dirent *entry)
{
  int max_matches = 512; /* this should actually be based on the max_xmit value */

  /* NT uses 260, OS/2 uses 2. Both accept 1. */
  int info_level = server->protocol < PROTOCOL_NT1 ? 1 : 260;

  char *p;
  int i;
  int first;
  int total_count = 0;
  struct smb_dirent *current_entry;

  char *resp_data;
  char *resp_param;
  int resp_data_len = 0;
  int resp_param_len = 0;

  int attribute = aSYSTEM | aHIDDEN | aDIR;
  int result;
  int error = 0;

  int ff_searchcount;
  int ff_eos = 0;
  int ff_dir_handle = 0;
  int ff_resume_key = 0;
  int loop_count = 0;

  unsigned char *outbuf = server->packet;

  int dirlen = strlen (path) + 2 + 1;
  char *mask;
  int masklen;

  ENTER();

  /* ZZZ experimental 'max_matches' adjustment */
  /*
  if(info_level == 260)
    max_matches = server->max_xmit / 360;
  else
    max_matches = server->max_xmit / 40;
  */

  SHOWVALUE(server->max_xmit);
  SHOWVALUE(max_matches);

  mask = malloc (dirlen);
  if (mask == NULL)
  {
    LOG (("Memory allocation failed\n"));
    error = (-ENOMEM);
    SHOWVALUE(error);
    goto out;
  }

  strcpy (mask, path);
  strcat (mask, "\\*");
  masklen = strlen (mask);

  LOG (("SMB call lreaddir %ld @ %ld\n", cache_size, fpos));
  LOG (("          mask = %s\n", mask));

  resp_param = NULL;
  resp_data = NULL;

 retry:

  first = 1;
  total_count = 0;
  current_entry = entry;

  while (ff_eos == 0)
  {
    loop_count++;
    if (loop_count > 200)
    {
      LOG (("smb_proc_readdir_long: Looping in FIND_NEXT???\n"));
      error = -EIO;
      SHOWVALUE(error);
      break;
    }

    memset (outbuf, 0, 39);

    smb_setup_header (server, SMBtrans2, 15, 5 + 12 + masklen + 1);

    WSET (outbuf, smb_tpscnt, 12 + masklen + 1);
    WSET (outbuf, smb_tdscnt, 0);
    WSET (outbuf, smb_mprcnt, 10);
    WSET (outbuf, smb_mdrcnt, server->max_xmit);
    WSET (outbuf, smb_msrcnt, 0);
    WSET (outbuf, smb_flags, 0);
    DSET (outbuf, smb_timeout, 0);
    WSET (outbuf, smb_pscnt, WVAL (outbuf, smb_tpscnt));
    WSET (outbuf, smb_psoff, ((SMB_BUF (outbuf) + 3) - outbuf) - 4);
    WSET (outbuf, smb_dscnt, 0);
    WSET (outbuf, smb_dsoff, 0);
    WSET (outbuf, smb_suwcnt, 1);
    WSET (outbuf, smb_setup0, first == 1 ? TRANSACT2_FINDFIRST : TRANSACT2_FINDNEXT);

    p = SMB_BUF (outbuf);
    (*p++) = 0;   /* put in a null smb_name */
    (*p++) = 'D';
    (*p++) = ' '; /* this was added because OS/2 does it */

    if (first != 0)
    {
      LOG (("first match\n"));
      WSET (p, 0, attribute);   /* attribute */
      WSET (p, 2, max_matches); /* max count */
      WSET (p, 4, 8 + 4 + 2);   /* resume required + close on end + continue */
      WSET (p, 6, info_level);
      DSET (p, 8, 0);
    }
    else
    {
      LOG (("next match; ff_dir_handle=0x%lx ff_resume_key=%ld mask='%s'\n", ff_dir_handle, ff_resume_key, mask));
      WSET (p, 0, ff_dir_handle);
      WSET (p, 2, max_matches); /* max count */
      WSET (p, 4, info_level);
      DSET (p, 6, ff_resume_key);
      WSET (p, 10, 8 + 4 + 2);  /* resume required + close on end + continue */
    }

    p += 12;

    if(masklen > 0)
      memcpy (p, mask, masklen);

    p += masklen;
    (*p++) = 0;
    (*p) = 0;

    result = smb_trans2_request (server, &resp_data_len, &resp_param_len, &resp_data, &resp_param);

    LOG (("smb_proc_readdir_long: smb_trans2_request returns %ld\n", result));

    if (result < 0)
    {
      if (smb_retry (server))
        goto retry;

      LOG (("smb_proc_readdir_long: got error from trans2_request\n"));
      error = result;
      SHOWVALUE(error);
      break;
    }

    /* Apparently, there is a bug in Windows 95 and friends which
       causes the directory read attempt to fail if you're asking
       for too much data too fast... */
    if(server->rcls == ERRSRV && server->err == ERRerror)
    {
      SHOWMSG("ouch; delaying and retrying");

      Delay(TICKS_PER_SECOND / 5);

      continue;
    }

    if (server->rcls != 0)
    {
      LOG (("server->rcls = %ld err = %ld\n",server->rcls, server->err));
      error = smb_errno (server->rcls, server->err);
      SHOWVALUE(error);
      break;
    }

    /* ZZZ bail out if this is empty. */
    if (resp_param == NULL)
      break;

    /* parse out some important return info */
    p = resp_param;
    if (first != 0)
    {
      ff_dir_handle = WVAL (p, 0);
      ff_searchcount = WVAL (p, 2);
      ff_eos = WVAL (p, 4);
    }
    else
    {
      ff_searchcount = WVAL (p, 0);
      ff_eos = WVAL (p, 2);
    }

    LOG (("received %ld entries (eos=%ld)\n",ff_searchcount, ff_eos));
    if (ff_searchcount == 0)
      break;

    /* ZZZ bail out if this is empty. */
    if (resp_data == NULL)
      break;

    /* point to the data bytes */
    p = resp_data;

    /* Now we are ready to parse smb directory entries. */
    for (i = 0; i < ff_searchcount; i++)
    {
      if(i == ff_searchcount - 1)
      {
        char * last_name;
        int len;

        ff_resume_key = DVAL(p, 0);

        smb_get_dirent_name(p,info_level,&last_name,&len);
        if(len > 0)
        {
          #if DEBUG
          {
            char buffer[SMB_MAXNAMELEN+1];

            memcpy(buffer,last_name,len);
            buffer[len] = '\0';

            LOG(("last name = '%s'\n",buffer));
          }
          #endif /* DEBUG */

          if(len + 1 > dirlen)
          {
            D(("increasing mask; old value = %ld new value = %ld",dirlen,len+1));

            if(mask != NULL)
              free (mask);

            dirlen = len + 1;
            SHOWVALUE(dirlen);

            mask = malloc (dirlen);
            if (mask == NULL)
            {
              LOG (("smb_proc_readdir_long: Memory allocation failed\n"));

              error = -ENOMEM;

              SHOWVALUE(error);

              goto fail;
            }
          }

          memcpy (mask, last_name, len);
          mask[len] = '\0';
          masklen = len;
        }
        else
        {
          masklen = 0;
        }
      }

      if (total_count < fpos)
      {
        p = smb_decode_long_dirent (p, NULL, info_level);

        LOG (("smb_proc_readdir: skipped entry; total_count = %ld, i = %ld, fpos = %ld\n",total_count, i, fpos));
      }
      else if (total_count >= fpos + cache_size)
      {
        p = smb_decode_long_dirent (p, NULL, info_level);

        LOG (("smb_proc_readdir: skipped entry; total_count = %ld, i = %ld, fpos = %ld\n",total_count, i, fpos));

        continue;
      }
      else
      {
        p = smb_decode_long_dirent (p, current_entry, info_level);

        current_entry += 1;
      }

      total_count += 1;
    }

    SHOWVALUE(ff_resume_key);

    if (resp_data != NULL)
    {
      free (resp_data);
      resp_data = NULL;
    }

    if (resp_param != NULL)
    {
      free (resp_param);
      resp_param = NULL;
    }

    first = 0;

    if (ff_searchcount > 0)
      loop_count = 0;
  }

 fail:

  /* finished: not needed any more */
  if (mask != NULL)
    free (mask);

  if (resp_data != NULL)
    free (resp_data);

  if (resp_param != NULL)
    free (resp_param);

 out:

  if(error < 0)
  {
    RETURN(error);
    return(error);
  }
  else
  {
    RETURN (total_count - fpos);
    return (total_count - fpos);
  }
}

int
smb_proc_readdir (struct smb_server *server, char *path, int fpos, int cache_size, struct smb_dirent *entry)
{
  int result;

  if (server->protocol >= PROTOCOL_LANMAN2)
    result = smb_proc_readdir_long (server, path, fpos, cache_size, entry);
  else
    result = smb_proc_readdir_short (server, path, fpos, cache_size, entry);

  return result;
}

int
smb_proc_getattr_core (struct smb_server *server, const char *path, int len, struct smb_dirent *entry)
{
  int result;
  char *p;
  char *buf = server->packet;

  LOG (("smb_proc_getattr: %s\n", path));

 retry:

  p = smb_setup_header (server, SMBgetatr, 0, 2 + len);
  smb_encode_ascii (p, path, len);

  if ((result = smb_request_ok (server, SMBgetatr, 10, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;

    goto out;
  }

  entry->attr = WVAL (buf, smb_vwv0);

  /* The server only tells us 1 time */
  entry->ctime = entry->atime = entry->mtime = entry->wtime = local2utc (DVAL (buf, smb_vwv1));

  entry->size = DVAL (buf, smb_vwv3);

  #if DEBUG
  {
    struct tm tm;

    GMTime(entry->ctime,&tm);
    LOG(("ctime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->atime,&tm);
    LOG(("atime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->mtime,&tm);
    LOG(("mtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->wtime,&tm);
    LOG(("wtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));
  }
  #endif /* DEBUG */

 out:

  return result;
}

/* smb_proc_getattrE: entry->fid must be valid */
int
smb_proc_getattrE (struct smb_server *server, struct smb_dirent *entry)
{
  char *buf = server->packet;
  int result;

  smb_setup_header_exclusive (server, SMBgetattrE, 1, 0);
  WSET (buf, smb_vwv0, entry->fileid);

  if ((result = smb_request_ok (server, SMBgetattrE, 11, 0)) < 0)
    goto out;

  entry->ctime = date_dos2unix (WVAL (buf, smb_vwv1), WVAL (buf, smb_vwv0));
  entry->atime = date_dos2unix (WVAL (buf, smb_vwv3), WVAL (buf, smb_vwv2));
  entry->mtime = date_dos2unix (WVAL (buf, smb_vwv5), WVAL (buf, smb_vwv4));
  entry->wtime = entry->mtime;
  entry->size = DVAL (buf, smb_vwv6);
  entry->attr = WVAL (buf, smb_vwv10);

  #if DEBUG
  {
    struct tm tm;

    GMTime(entry->ctime,&tm);
    LOG(("ctime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->atime,&tm);
    LOG(("atime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->mtime,&tm);
    LOG(("mtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));

    GMTime(entry->wtime,&tm);
    LOG(("wtime = %ld-%02ld-%02ld %ld:%02ld:%02ld\n",tm.tm_year + 1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec));
  }
  #endif /* DEBUG */

 out:

  return result;
}

/* In core protocol, there is only 1 time to be set, we use
   entry->mtime, to make touch work. */
int
smb_proc_setattr_core (struct smb_server *server, const char *path, int len, struct smb_dirent *new_finfo)
{
  char *p;
  char *buf = server->packet;
  int result;
  int local_time;

 retry:

  LOG (("smb_proc_setattr_core\n"));

  p = smb_setup_header (server, SMBsetatr, 8, 4 + len);
  WSET (buf, smb_vwv0, new_finfo->attr);
  local_time = utc2local (new_finfo->mtime);
  DSET (buf, smb_vwv1, local_time);
  p = smb_encode_ascii (p, path, len);
  (void) smb_encode_ascii (p, "", 0);

  if ((result = smb_request_ok (server, SMBsetatr, 0, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;
  }

  return result;
}

/* smb_proc_setattrE: we do not retry here, because we rely on fid,
   which would not be valid after a retry. */
int
smb_proc_setattrE (struct smb_server *server, word fid, struct smb_dirent *new_entry)
{
  char *buf = server->packet;
  word date, time_value;
  int result;

  LOG (("smb_proc_setattrE\n"));

  smb_setup_header_exclusive (server, SMBsetattrE, 7, 0);

  WSET (buf, smb_vwv0, fid);

  date_unix2dos (new_entry->ctime, &time_value, &date);
  WSET (buf, smb_vwv1, date);
  WSET (buf, smb_vwv2, time_value);

  date_unix2dos (new_entry->atime, &time_value, &date);
  WSET (buf, smb_vwv3, date);
  WSET (buf, smb_vwv4, time_value);

  date_unix2dos (new_entry->mtime, &time_value, &date);
  WSET (buf, smb_vwv5, date);
  WSET (buf, smb_vwv6, time_value);

  result = smb_request_ok_unlock (server, SMBsetattrE, 0, 0);

  return result;
}

int
smb_proc_dskattr (struct smb_server *server, struct smb_dskattr *attr)
{
  int error;
  char *p;

 retry:

  smb_setup_header (server, SMBdskattr, 0, 0);

  if ((error = smb_request_ok (server, SMBdskattr, 5, 0)) < 0)
  {
    if (smb_retry (server))
      goto retry;

    goto out;
  }

  p = SMB_VWV (server->packet);
  p = smb_decode_word (p, &attr->total);
  p = smb_decode_word (p, &attr->allocblocks);
  p = smb_decode_word (p, &attr->blocksize);
  (void) smb_decode_word (p, &attr->free);

 out:

  return error;
}

/*****************************************************************************
 *
 *  Mount/umount operations.
 *
 ****************************************************************************/
struct smb_prots
{
  enum smb_protocol prot;
  const char *name;
};

/* smb_proc_reconnect: We expect the server to be locked, so that you
   can call the routine from within smb_retry. The socket must be
   created, like after a user-level socket()-call. It may not be
   connected. */
static int
smb_proc_reconnect (struct smb_server *server)
{
  static const struct smb_prots prots[] =
  {
    {PROTOCOL_CORE, "PC NETWORK PROGRAM 1.0"},
    {PROTOCOL_COREPLUS, "MICROSOFT NETWORKS 1.03"},
    {PROTOCOL_LANMAN1, "MICROSOFT NETWORKS 3.0"},
    {PROTOCOL_LANMAN1, "LANMAN1.0"},
    {PROTOCOL_LANMAN2, "LM1.2X002"},
    {PROTOCOL_NT1, "NT LM 0.12"},
    {PROTOCOL_NT1, "NT LANMAN 1.0"},

    {-1, NULL}
  };

  char dev[] = "A:";
  int i, plength;
  int max_xmit = 1024; /* Space needed for first request. */
  int given_max_xmit = server->mount_data.max_xmit;
  int result;
  word any_word;
  byte *p;
  unsigned char password[24];
  int password_len;
  unsigned char nt_password[24];
  int nt_password_len;
  unsigned char full_share[SMB_MAXNAMELEN+1];
  int full_share_len;
  byte *packet;

  if (server->max_recv <= 0)
    server->max_recv = given_max_xmit > 8000 ? given_max_xmit : 8000;

  if ((result = smb_connect (server)) < 0)
  {
    LOG (("smb_proc_reconnect: could not smb_connect\n"));
    goto fail;
  }

  /* Here we assume that the connection is valid */
  server->state = CONN_VALID;

  if (server->packet != NULL)
    free (server->packet);

  server->packet = malloc (server->max_recv);

  if (server->packet == NULL)
  {
    LOG (("smb_proc_connect: No memory! Bailing out.\n"));
    result = -ENOMEM;
    goto fail;
  }

  packet = server->packet;

  server->max_xmit = max_xmit;

  /* Start with an RFC1002 session request packet. */
  p = packet + 4;

  p = smb_name_mangle (p, server->mount_data.server_name);
  p = smb_name_mangle (p, server->mount_data.client_name);

  smb_encode_smb_length (packet, (byte *) p - (byte *) (packet));

  packet[0] = 0x81; /* SESSION REQUEST */

  if ((result = smb_request (server)) < 0)
  {
    LOG (("smb_proc_connect: Failed to send SESSION REQUEST.\n"));
    goto fail;
  }

  if (packet[0] != 0x82)
  {
    LOG (("smb_proc_connect: Did not receive positive response (err = %lx)\n",packet[0]));

    #if DEBUG
    {
      smb_dump_packet (packet);
    }
    #endif /* DEBUG */

    result = -EIO;
    goto fail;
  }

  LOG (("smb_proc_connect: Passed SESSION REQUEST.\n"));

  /* Now we are ready to send a SMB Negotiate Protocol packet. */
  memset (packet, 0, SMB_HEADER_LEN);

  plength = 0;
  for (i = 0; prots[i].name != NULL; i++)
    plength += strlen (prots[i].name) + 2;

  smb_setup_header (server, SMBnegprot, 0, plength);

  p = SMB_BUF (packet);

  for (i = 0; prots[i].name != NULL; i++)
    p = smb_encode_dialect (p, prots[i].name, strlen (prots[i].name));

  LOG (("smb_proc_connect: Request SMBnegprot...\n"));
  if ((result = smb_request_ok (server, SMBnegprot, 1, -1)) < 0)
  {
    LOG (("smb_proc_connect: Failure requesting SMBnegprot\n"));
    goto fail;
  }

  LOG (("Verified!\n"));

  p = SMB_VWV (packet);
  p = smb_decode_word (p, &any_word);
  i = any_word;

  server->protocol = prots[i].prot;

  LOG (("smb_proc_connect: Server wants %s protocol.\n",prots[i].name));

  if (server->protocol > PROTOCOL_LANMAN1)
  {
    int user_len = strlen (server->mount_data.username)+1;

    LOG (("smb_proc_connect: password = %s\n",server->mount_data.password));
    LOG (("smb_proc_connect: usernam = %s\n",server->mount_data.username));
    LOG (("smb_proc_connect: blkmode = %ld\n",WVAL (packet, smb_vwv5)));

    if (server->protocol >= PROTOCOL_NT1)
    {
      server->maxxmt = DVAL (packet, smb_vwv3 + 1);
      server->blkmode = DVAL (packet, smb_vwv9 + 1);
      server->sesskey = DVAL (packet, smb_vwv7 + 1);

      server->security_mode = BVAL(packet, smb_vwv1);

      memcpy(server->crypt_key,SMB_BUF(packet),8);
    }
    else
    {
      server->maxxmt = WVAL (packet, smb_vwv2);
      server->blkmode = WVAL (packet, smb_vwv5);
      server->sesskey = DVAL (packet, smb_vwv6);

      server->security_mode = BVAL(packet, smb_vwv1);

      memcpy(server->crypt_key,SMB_BUF(packet),8);
    }

    SHOWVALUE(server->security_mode);

    if(server->security_mode & 2)
    {
      SHOWMSG("encrypted passwords required");

      memset(password,0,sizeof(password));
      strlcpy(password,server->mount_data.password,sizeof(password));

      smb_encrypt(password,server->crypt_key,password);
      password_len = 24;

      PRINTHEADER();
      PRINTF(("password: "));
      for(i = 0 ; i < 24 ; i++)
        PRINTF(("%02lx ",password[i]));
      PRINTF(("\n"));

      memset(nt_password,0,sizeof(nt_password));
      strlcpy(nt_password,server->mount_data.password,sizeof(nt_password));

      smb_nt_encrypt(nt_password,server->crypt_key,nt_password);
      nt_password_len = 24;

      PRINTHEADER();
      PRINTF(("nt_password: "));
      for(i = 0 ; i < 24 ; i++)
        PRINTF(("%02lx ",nt_password[i]));
      PRINTF(("\n"));

      PRINTHEADER();
      PRINTF(("crypt_key: "));
      for(i = 0 ; i < 8 ; i++)
        PRINTF(("%02lx ",server->crypt_key[i]));
      PRINTF(("\n"));
    }
    else
    {
      SHOWMSG("plain text passwords sufficient");

      password_len = strlen(server->mount_data.password)+1;
      nt_password_len = 0;
    }

    /* If in share level security then don't send a password now */
    if((server->security_mode & 1) == 0)
    {
      SHOWMSG("share level security; zapping passwords");
      strcpy(password,"");
      password_len = 0;

      strcpy(nt_password,"");
      nt_password_len = 0;
    }

    SHOWVALUE(password_len);
    SHOWVALUE(nt_password_len);

    LOG (("smb_proc_connect: workgroup = %s\n", server->mount_data.workgroup_name));
    if (server->protocol >= PROTOCOL_NT1)
    {
#if !defined(__AROS__)
      char *OS_id = "AmigaOS";
#else
      char *OS_id = "AROS";
#endif
      char *client_id = "smbfs";

      SHOWMSG("server->protocol >= PROTOCOL_NT1");

      smb_setup_header (server, SMBsesssetupX, 13, user_len + password_len + nt_password_len + strlen (server->mount_data.workgroup_name)+1 + strlen (OS_id)+1 + strlen (client_id)+1);

      WSET (packet, smb_vwv0, 0xff);
      WSET (packet, smb_vwv2, given_max_xmit);
      WSET (packet, smb_vwv3, 2);
      WSET (packet, smb_vwv4, 0); /* server->pid */
      DSET (packet, smb_vwv5, server->sesskey);
      WSET (packet, smb_vwv7, password_len);
      WSET (packet, smb_vwv8, nt_password_len);

      p = SMB_BUF (packet);

      if(nt_password_len != 0)
      {
        SHOWMSG("adding encrypted passwords");

        memcpy (p, password, password_len);
        p += password_len;

        memcpy (p, nt_password, nt_password_len);
        p += nt_password_len;
      }
      else
      {
        SHOWMSG("adding plain text password");

        memcpy (p, server->mount_data.password, password_len);
        p += password_len;
      }

      memcpy (p, server->mount_data.username, user_len);
      p += user_len;

      strcpy (p, server->mount_data.workgroup_name);
      p += strlen (p) + 1;

      strcpy (p, OS_id);
      p += strlen (p) + 1;

      strcpy (p, client_id);
    }
    else
    {
      smb_setup_header (server, SMBsesssetupX, 10, user_len + password_len);

      WSET (packet, smb_vwv0, 0xff);
      WSET (packet, smb_vwv1, 0);
      WSET (packet, smb_vwv2, given_max_xmit);
      WSET (packet, smb_vwv3, 2);
      WSET (packet, smb_vwv4, 0); /* server->pid */
      DSET (packet, smb_vwv5, server->sesskey);
      WSET (packet, smb_vwv7, password_len);
      WSET (packet, smb_vwv8, 0);
      WSET (packet, smb_vwv9, 0);

      p = SMB_BUF (packet);
      memcpy (p, server->mount_data.password, password_len);
      p += password_len;
      memcpy (p, server->mount_data.username, user_len);
    }

    if ((result = smb_request_ok (server, SMBsesssetupX, 3, 0)) < 0)
    {
      LOG (("smb_proc_connect: SMBsessetupX failed\n"));
      goto fail;
    }

    smb_decode_word (packet + 32, &(server->server_uid));
  }
  else
  {
    server->maxxmt = 0;
    server->blkmode = 0;
    server->sesskey = 0;

    password_len = strlen(server->mount_data.password)+1;

    nt_password_len = 0;
  }

  if(nt_password_len > 0)
  {
    strlcpy(full_share,"//",sizeof(full_share));
    strlcat(full_share,server->mount_data.server_name,sizeof(full_share));
    strlcat(full_share,"/",sizeof(full_share));
    strlcat(full_share,server->mount_data.service,sizeof(full_share));

    full_share_len = strlen(full_share);

    for(i = 0 ; i < full_share_len ; i++)
    {
      if(full_share[i] == '/')
        full_share[i] = '\\';
    }

    StringToUpper(full_share);

    SHOWSTRING(full_share);

    memset (packet, 0, SMB_HEADER_LEN);

    smb_setup_header (server, SMBtconX, 4, password_len + full_share_len+1 + strlen(dev)+1);

    WSET (packet, smb_vwv0, 0xFF);
    WSET (packet, smb_vwv3, password_len);

    p = SMB_BUF (packet);

    if(nt_password_len > 0)
      memcpy(p,password,password_len);
    else
      memcpy (p, server->mount_data.password, password_len);

    p += password_len;

    memcpy(p,full_share,full_share_len+1);
    p += full_share_len+1;

    strcpy(p,dev);

    BSET(packet,smb_rcls,1);

    if ((result = smb_request_ok (server, SMBtconX, 3, 0)) < 0)
    {
      SHOWVALUE(SMB_WCT(packet));

      LOG (("smb_proc_connect: SMBtconX not verified.\n"));
      goto fail;
    }

    SHOWVALUE(SMB_WCT(packet));

    /* Changed, max_xmit hasn't been updated if a tconX message was send instead of tcon. */
    if (server->maxxmt)
      server->max_xmit = server->maxxmt;

    server->tid = WVAL(packet,smb_tid);
  }
  else
  {
    /* Fine! We have a connection, send a tcon message. */
    smb_setup_header (server, SMBtcon, 0, 6 + strlen (server->mount_data.service) + strlen (server->mount_data.password) + strlen (dev));

    p = SMB_BUF (packet);
    p = smb_encode_ascii (p, server->mount_data.service, strlen (server->mount_data.service));
    p = smb_encode_ascii (p, server->mount_data.password, strlen (server->mount_data.password));
    (void) smb_encode_ascii (p, dev, strlen (dev));

    if ((result = smb_request_ok (server, SMBtcon, 2, 0)) < 0)
    {
      LOG (("smb_proc_connect: SMBtcon not verified.\n"));
      goto fail;
    }

    LOG (("OK! Managed to set up SMBtcon!\n"));

    p = SMB_VWV (packet);
    p = smb_decode_word (p, &server->max_xmit);

    SHOWVALUE(server->max_xmit);

    /* Added by Brian Willette - We were ignoring the server's initial
       maxbuf value */
    if (server->maxxmt != 0 && server->max_xmit > server->maxxmt)
      server->max_xmit = server->maxxmt;

    (void) smb_decode_word (p, &server->tid);
  }

  SHOWVALUE(server->max_xmit);

  /* Changed, max_xmit hasn't been updated if a tconX message was send instead of tcon. */
  if (server->max_xmit > given_max_xmit)
    server->max_xmit = given_max_xmit;

  /* Ok, everything is fine. max_xmit does not include
     the TCP-SMB header of 4 bytes. */
  if (server->max_xmit < 65535 - 4)
    server->max_xmit += 4;

  LOG (("max_xmit = %ld, tid = %ld\n", server->max_xmit, server->tid));

  LOG (("smb_proc_connect: Normal exit\n"));

  return 0;

 fail:

  server->state = CONN_INVALID;

  return result;
}

/* smb_proc_reconnect: server->packet is allocated with
   server->max_xmit bytes if and only if we return >= 0 */
int
smb_proc_connect (struct smb_server *server)
{
  int result;

  result = smb_proc_reconnect (server);

  if ((result < 0) && (server->packet != NULL))
  {
    free (server->packet);
    server->packet = NULL;
  }

  return result;
}

/* error code stuff - put together by Merik Karman
   merik -at- blackadder -dot- dsh -dot- oz -dot- au */
typedef struct
{
  char *name;
  int code;
  char *message;
} err_code_struct;

/* Dos Error Messages */
static const err_code_struct dos_msgs[] =
{
  {"ERRbadfunc", 1, "Invalid function"},
  {"ERRbadfile", 2, "File not found"},
  {"ERRbadpath", 3, "Directory invalid"},
  {"ERRnofids", 4, "No file descriptors available"},
  {"ERRnoaccess", 5, "Access denied"},
  {"ERRbadfid", 6, "Invalid file handle"},
  {"ERRbadmcb", 7, "Memory control blocks destroyed"},
  {"ERRnomem", 8, "Insufficient server memory to perform the requested function"},
  {"ERRbadmem", 9, "Invalid memory block address"},
  {"ERRbadenv", 10, "Invalid environment"},
  {"ERRbadformat", 11, "Invalid format"},
  {"ERRbadaccess", 12, "Invalid open mode"},
  {"ERRbaddata", 13, "Invalid data"},
  {"ERR", 14, "reserved"},
  {"ERRbaddrive", 15, "Invalid drive specified"},
  {"ERRremcd", 16, "A Delete Directory request attempted  to  remove  the  server's  current directory"},
  {"ERRdiffdevice", 17, "Not same device"},
  {"ERRnofiles", 18, "A File Search command can find no more files matching the specified criteria"},
  {"ERRbadshare", 32, "The sharing mode specified for an Open conflicts with existing  FIDs  on the file"},
  {"ERRlock", 33, "A Lock request conflicted with an existing lock or specified an  invalid mode,  or an Unlock requested attempted to remove a lock held by another process"},
  {"ERRfilexists", 80, "The file named in a Create Directory, Make  New  File  or  Link  request already exists"},
  {"ERRbadpipe", 230, "Pipe invalid"},
  {"ERRpipebusy", 231, "All instances of the requested pipe are busy"},
  {"ERRpipeclosing", 232, "Pipe close in progress"},
  {"ERRnotconnected", 233, "No process on other end of pipe"},
  {"ERRmoredata", 234, "There is more data to be returned"},

  {NULL, -1, NULL}
};

/* Server Error Messages */
static const err_code_struct server_msgs[] =
{
  {"ERRerror", 1, "Non-specific error code"},
  {"ERRbadpw", 2, "Bad password - name/password pair in a Tree Connect or Session Setup are invalid"},
  {"ERRbadtype", 3, "reserved"},
  {"ERRaccess", 4, "The requester does not have  the  necessary  access  rights  within  the specified  context for the requested function. The context is defined by the TID or the UID"},
  {"ERRinvnid", 5, "The tree ID (TID) specified in a command was invalid"},
  {"ERRinvnetname", 6, "Invalid network name in tree connect"},
  {"ERRinvdevice", 7, "Invalid device - printer request made to non-printer connection or  non-printer request made to printer connection"},
  {"ERRqfull", 49, "Print queue full (files) -- returned by open print file"},
  {"ERRqtoobig", 50, "Print queue full -- no space"},
  {"ERRqeof", 51, "EOF on print queue dump"},
  {"ERRinvpfid", 52, "Invalid print file FID"},
  {"ERRsmbcmd", 64, "The server did not recognize the command received"},
  {"ERRsrverror", 65, "The server encountered an internal error, e.g., system file unavailable"},
  {"ERRfilespecs", 67, "The file handle (FID) and pathname parameters contained an invalid  combination of values"},
  {"ERRreserved", 68, "reserved"},
  {"ERRbadpermits", 69, "The access permissions specified for a file or directory are not a valid combination.  The server cannot set the requested attribute"},
  {"ERRreserved", 70, "reserved"},
  {"ERRsetattrmode", 71, "The attribute mode in the Set File Attribute request is invalid"},
  {"ERRpaused", 81, "Server is paused"},
  {"ERRmsgoff", 82, "Not receiving messages"},
  {"ERRnoroom", 83, "No room to buffer message"},
  {"ERRrmuns", 87, "Too many remote user names"},
  {"ERRtimeout", 88, "Operation timed out"},
  {"ERRnoresource", 89, "No resources currently available for request"},
  {"ERRtoomanyuids", 90, "Too many UIDs active on this session"},
  {"ERRbaduid", 91, "The UID is not known as a valid ID on this session"},
  {"ERRusempx", 250, "Temp unable to support Raw, use MPX mode"},
  {"ERRusestd", 251, "Temp unable to support Raw, use standard read/write"},
  {"ERRcontmpx", 252, "Continue in MPX mode"},
  {"ERRreserved", 253, "reserved"},
  {"ERRreserved", 254, "reserved"},
  {"ERRnosupport", 0xFFFF, "Function not supported"},

  {NULL, -1, NULL}
};

/* Hard Error Messages */
static const err_code_struct hard_msgs[] =
{
  {"ERRnowrite", 19, "Attempt to write on write-protected diskette"},
  {"ERRbadunit", 20, "Unknown unit"},
  {"ERRnotready", 21, "Drive not ready"},
  {"ERRbadcmd", 22, "Unknown command"},
  {"ERRdata", 23, "Data error (CRC)"},
  {"ERRbadreq", 24, "Bad request structure length"},
  {"ERRseek", 25, "Seek error"},
  {"ERRbadmedia", 26, "Unknown media type"},
  {"ERRbadsector", 27, "Sector not found"},
  {"ERRnopaper", 28, "Printer out of paper"},
  {"ERRwrite", 29, "Write fault"},
  {"ERRread", 30, "Read fault"},
  {"ERRgeneral", 31, "General failure"},
  {"ERRbadshare", 32, "A open conflicts with an existing open"},
  {"ERRlock", 33, "A Lock request conflicted with an existing lock or specified an invalid mode, or an Unlock requested attempted to remove a lock held by another process"},
  {"ERRwrongdisk", 34, "The wrong disk was found in a drive"},
  {"ERRFCBUnavail", 35, "No FCBs are available to process request"},
  {"ERRsharebufexc", 36, "A sharing buffer has been exceeded"},

  {NULL, -1, NULL}
};

typedef struct
{
  int code;
  char *class;
  const err_code_struct *err_msgs;
} err_class_struct;

static const err_class_struct err_classes[] =
{
  { 0, "SUCCESS", NULL },
  { 0x01, "ERRDOS", dos_msgs },
  { 0x02, "ERRSRV", server_msgs },
  { 0x03, "ERRHRD", hard_msgs },
  { 0x04, "ERRXOS", NULL },
  { 0xE1, "ERRRMX1", NULL },
  { 0xE2, "ERRRMX2", NULL },
  { 0xE3, "ERRRMX3", NULL },
  { 0xFF, "ERRCMD", NULL },

  { -1, NULL, NULL }
};

static void
smb_printerr (int class, int num)
{
  int i, j;
  err_code_struct *err;

  for (i = 0; err_classes[i].class; i++)
  {
    if (err_classes[i].code != class)
      continue;

    if (!err_classes[i].err_msgs)
    {
      ReportError("%s - %ld.", err_classes[i].class, num);

      LOG (("%s - %ld\n", err_classes[i].class, num));
      return;
    }

    err = (err_code_struct *)err_classes[i].err_msgs;

    for (j = 0; err[j].name; j++)
    {
      if (num != err[j].code)
        continue;

      ReportError ("%s - %s (%s).", err_classes[i].class, err[j].name, err[j].message);

      LOG (("%s - %s (%s)\n",err_classes[i].class, err[j].name,err[j].message));
      return;
    }
  }

  ReportError ("Unknown error - (%ld, %ld).", class, num);

  LOG (("Unknown error - (%ld, %ld)\n", class, num));
}
