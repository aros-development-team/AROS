/*
 * $Id$
 *
 * :ts=8
 *
 * sock.c
 *
 * Copyright (C) 1995 by Paal-Kr. Engstad and Volker Lendecke
 * Modified by Christian Starkjohann <cs -at- hal -dot- kph -dot- tuwien -dot- ac -dot- at>
 * Modified for use with AmigaOS by Olaf Barthel <obarthel -at- gmx -dot- net>
 */

#include "smbfs.h"

/*****************************************************************************/

#include <smb/smb_fs.h>
#include <smb/smb.h>
#include <smb/smbno.h>

/*****************************************************************************/

#include "smb_abstraction.h"

/*****************************************************************************/

/* smb_receive_raw
   fs points to the correct segment, sock != NULL, target != NULL
   The smb header is only stored if want_header != 0. */
static int
smb_receive_raw (int sock_fd, unsigned char *target, int max_raw_length, int want_header)
{
  int len, result;
  int already_read;
  unsigned char peek_buf[4];

 re_recv:

  result = recvfrom (sock_fd, (void *) peek_buf, 4, 0, NULL, NULL);
  if (result < 0)
  {
    LOG (("smb_receive_raw: recv error = %ld\n", errno));
    result = (-errno);
    goto out;
  }

  if (result < 4)
  {
    LOG (("smb_receive_raw: got less than 4 bytes\n"));
    result = -EIO;
    goto out;
  }

  switch (peek_buf[0])
  {
    case 0x00:
    case 0x82:
      break;

    case 0x85:
      LOG (("smb_receive_raw: Got SESSION KEEP ALIVE\n"));
      goto re_recv;

    default:
      LOG (("smb_receive_raw: Invalid packet 0x%02lx\n", peek_buf[0]));
      result = -EIO;
      goto out;
  }

  /* The length in the RFC NB header is the raw data length */
  len = smb_len (peek_buf);
  if (len > max_raw_length)
  {
    LOG (("smb_receive_raw: Received length (%ld) > max_xmit (%ld)!\n", len, max_raw_length));
    result = -EIO;
    goto out;
  }

  if (want_header != 0)
  {
    memcpy (target, peek_buf, 4);
    target += 4;
  }

  already_read = 0;

  while (already_read < len)
  {
    result = recvfrom (sock_fd, (void *) (target + already_read), len - already_read, 0, NULL, NULL);
    if (result < 0)
    {
      LOG (("smb_receive_raw: recvfrom error = %ld\n", errno));

      result = (-errno);

      goto out;
    }

    already_read += result;
  }

  result = already_read;

 out:

  return result;
}

/* smb_receive
   fs points to the correct segment, server != NULL, sock!=NULL */
static int
smb_receive (struct smb_server *server, int sock_fd)
{
  byte * packet = server->packet;
  int result;

  result = smb_receive_raw (sock_fd, packet,
                            server->max_recv - 4,  /* max_xmit in server includes NB header */
                            1); /* We want the header */
  if (result < 0)
  {
    LOG (("smb_receive: receive error: %ld\n", result));
    goto out;
  }

  server->rcls = *((unsigned char *) (packet + 9));
  server->err = WVAL (packet, 11);

  if (server->rcls != 0)
    LOG (("smb_receive: rcls=%ld, err=%ld\n", server->rcls, server->err));

 out:

  return result;
}

/* smb_receive's preconditions also apply here. */
static int
smb_receive_trans2 (struct smb_server *server, int sock_fd, int *data_len, int *param_len, char **data, char **param)
{
  unsigned char *inbuf = server->packet;
  int total_data;
  int total_param;
  int result;

  LOG (("smb_receive_trans2: enter\n"));

  (*data_len) = (*param_len) = 0;
  (*param) = (*data) = NULL;

  result = smb_receive (server, sock_fd);
  if (result < 0)
    goto fail;

  if (server->rcls != 0)
    goto fail;

  /* parse out the lengths */
  total_data = WVAL (inbuf, smb_tdrcnt);
  total_param = WVAL (inbuf, smb_tprcnt);

  if ((total_data > server->max_xmit) || (total_param > server->max_xmit))
  {
    LOG (("smb_receive_trans2: data/param too long\n"));

    result = -EIO;
    goto fail;
  }

  /* Allocate it, but only if there is something to allocate
     in the first place. ZZZ this may not be the proper approach,
     and we should return an error for 'no data'. */
  if(total_data > 0)
  {
    (*data) = malloc (total_data);
    if ((*data) == NULL)
    {
      LOG (("smb_receive_trans2: could not alloc data area\n"));

      result = -ENOMEM;
      goto fail;
    }
  }
  else
  {
    (*data) = NULL;
  }

  /* Allocate it, but only if there is something to allocate
     in the first place. ZZZ this may not be the proper approach,
     and we should return an error for 'no parameters'. */
  if(total_param > 0)
  {
    (*param) = malloc(total_param);
    if ((*param) == NULL)
    {
      LOG (("smb_receive_trans2: could not alloc param area\n"));

      result = -ENOMEM;
      goto fail;
    }
  }
  else
  {
    (*param) = NULL;
  }

  LOG (("smb_rec_trans2: total_data/param: %ld/%ld\n", total_data, total_param));

  while (1)
  {
    if (WVAL (inbuf, smb_prdisp) + WVAL (inbuf, smb_prcnt) > (unsigned int)total_param)
    {
      LOG (("smb_receive_trans2: invalid parameters\n"));
      result = -EIO;
      goto fail;
    }

    if((*param) != NULL)
      memcpy ((*param) + WVAL (inbuf, smb_prdisp), smb_base (inbuf) + WVAL (inbuf, smb_proff), WVAL (inbuf, smb_prcnt));

    (*param_len) += WVAL (inbuf, smb_prcnt);

    if (WVAL (inbuf, smb_drdisp) + WVAL (inbuf, smb_drcnt) > (unsigned int)total_data)
    {
      LOG (("smb_receive_trans2: invalid data block\n"));
      result = -EIO;
      goto fail;
    }

    if((*data) != NULL)
      memcpy ((*data) + WVAL (inbuf, smb_drdisp), smb_base (inbuf) + WVAL (inbuf, smb_droff), WVAL (inbuf, smb_drcnt));

    (*data_len) += WVAL (inbuf, smb_drcnt);

    LOG (("smb_rec_trans2: drcnt/prcnt: %ld/%ld\n", WVAL (inbuf, smb_drcnt), WVAL (inbuf, smb_prcnt)));

    /* parse out the total lengths again - they can shrink! */
    if ((WVAL (inbuf, smb_tdrcnt) > (unsigned int)total_data) || (WVAL (inbuf, smb_tprcnt) > (unsigned int)total_param))
    {
      LOG (("smb_receive_trans2: data/params grew!\n"));
      result = -EIO;
      goto fail;
    }

    total_data = WVAL (inbuf, smb_tdrcnt);
    total_param = WVAL (inbuf, smb_tprcnt);
    if (total_data <= (*data_len) && total_param <= (*param_len))
      break;

    result = smb_receive (server, sock_fd);
    if (result < 0)
      goto fail;

    if (server->rcls != 0)
    {
      result = -EIO;
      goto fail;
    }
  }

  LOG (("smb_receive_trans2: normal exit\n"));
  return 0;

 fail:

  LOG (("smb_receive_trans2: failed exit\n"));

  if((*param) != NULL)
    free (*param);

  if((*data) != NULL)
    free (*data);

  (*param) = (*data) = NULL;

  return result;
}

int
smb_release (struct smb_server *server)
{
  int result;

  if (server->mount_data.fd >= 0)
    CloseSocket (server->mount_data.fd);

  server->mount_data.fd = socket (AF_INET, SOCK_STREAM, 0);
  if (server->mount_data.fd < 0)
  {
    result = (-errno);
    goto out;
  }

  result = 0;

 out:

  return result;
}

int
smb_connect (struct smb_server *server)
{
  int sock_fd = server->mount_data.fd;
  int result;

  if (sock_fd < 0)
  {
    result = (-EBADF);
    goto out;
  }

  result = connect (sock_fd, (struct sockaddr *)&server->mount_data.addr, sizeof(struct sockaddr_in));
  if(result < 0)
    result = (-errno);

 out:

  return(result);
}

/*****************************************************************************
 *
 *  This routine was once taken from nfs, which is for udp. Here TCP does
 *  most of the ugly stuff for us (thanks, Alan!)
 *
 ****************************************************************************/

int
smb_request (struct smb_server *server)
{
  int len, result;
  int sock_fd = server->mount_data.fd;
  unsigned char *buffer = server->packet;

  if ((sock_fd < 0) || (buffer == NULL))
  {
    LOG (("smb_request: Bad server!\n"));
    result = -EBADF;
    goto out;
  }

  if (server->state != CONN_VALID)
  {
    result = -EIO;
    goto out;
  }

  len = smb_len (buffer) + 4;

  LOG (("smb_request: len = %ld cmd = 0x%lx\n", len, buffer[8]));

  result = send (sock_fd, (void *) buffer, len, 0);
  if (result < 0)
  {
    LOG (("smb_request: send error = %ld\n", errno));

    result = (-errno);
  }
  else
  {
    result = smb_receive (server, sock_fd);
  }

 out:

  if (result < 0)
  {
    server->state = CONN_INVALID;
    smb_invalidate_all_inodes (server);
  }

  LOG (("smb_request: result = %ld\n", result));

  return (result);
}

/* This is not really a trans2 request, we assume that you only have
   one packet to send. */
int
smb_trans2_request (struct smb_server *server, int *data_len, int *param_len, char **data, char **param)
{
  int len, result;
  int sock_fd = server->mount_data.fd;
  unsigned char *buffer = server->packet;

  if (server->state != CONN_VALID)
  {
    result = -EIO;
    goto out;
  }

  len = smb_len (buffer) + 4;

  LOG (("smb_request: len = %ld cmd = 0x%02lx\n", len, buffer[8]));

  result = send (sock_fd, (void *) buffer, len, 0);
  if (result < 0)
  {
    LOG (("smb_trans2_request: send error = %ld\n", errno));

    result = (-errno);
  }
  else
  {
    result = smb_receive_trans2 (server, sock_fd, data_len, param_len, data, param);
  }

 out:

  if (result < 0)
  {
    server->state = CONN_INVALID;
    smb_invalidate_all_inodes (server);
  }

  LOG (("smb_trans2_request: result = %ld\n", result));

  return result;
}

/* target must be in user space */
int
smb_request_read_raw (struct smb_server *server, unsigned char *target, int max_len)
{
  int len, result;
  int sock_fd = server->mount_data.fd;
  unsigned char *buffer = server->packet;

  if (server->state != CONN_VALID)
  {
    result = -EIO;
    goto out;
  }

  len = smb_len (buffer) + 4;

  LOG (("smb_request_read_raw: len = %ld cmd = 0x%02lx\n", len, buffer[8]));
  LOG (("smb_request_read_raw: target=%lx, max_len=%ld\n", (unsigned int) target, max_len));
  LOG (("smb_request_read_raw: buffer=%lx, sock=%lx\n", (unsigned int) buffer, (unsigned int) sock_fd));

  result = send (sock_fd, (void *) buffer, len, 0);

  LOG (("smb_request_read_raw: send returned %ld\n", result));

  if (result < 0)
  {
    LOG (("smb_request_read_raw: send error = %ld\n", errno));

    result = (-errno);
  }
  else
  {
    result = smb_receive_raw (sock_fd, target, max_len, 0);
  }

 out:

  if (result < 0)
  {
    server->state = CONN_INVALID;
    smb_invalidate_all_inodes (server);
  }

  LOG (("smb_request_read_raw: result = %ld\n", result));

  return result;
}

/* Source must be in user space. smb_request_write_raw assumes that
   the request SMBwriteBraw has been completed successfully, so that
   we can send the raw data now. */
int
smb_request_write_raw (struct smb_server *server, unsigned const char *source, int length)
{
  int result;
  byte nb_header[4];
  int sock_fd = server->mount_data.fd;

  if (server->state != CONN_VALID)
  {
    result = -EIO;
    goto out;
  }

  smb_encode_smb_length (nb_header, length);

  result = send (sock_fd, (void *) nb_header, 4, 0);
  if (result == 4)
  {
    result = send (sock_fd, (void *) source, length, 0);
    if(result < 0)
      result = (-errno);
  }
  else
  {
    if(result < 0)
      result = (-errno);
    else
      result = -EIO;
  }

  LOG (("smb_request_write_raw: send returned %ld\n", result));

  if (result == length)
    result = smb_receive (server, sock_fd);

 out:

  if (result < 0)
  {
    server->state = CONN_INVALID;

    smb_invalidate_all_inodes (server);
  }

  if (result > 0)
    result = length;

  LOG (("smb_request_write_raw: result = %ld\n", result));

  return result;
}
