/*
 * $Id$
 *
 * :ts=4
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
#include "dump_smb.h"

/*****************************************************************************/

/* smb_receive_raw
   fs points to the correct segment, sock != NULL, target != NULL
   The smb header is only stored if want_header != 0. */
static int
smb_receive_raw (const struct smb_server *server, int sock_fd, unsigned char *target, int max_raw_length, int want_header)
{
	int len, result;
	int already_read;
	unsigned char netbios_session_buf[256];
	int netbios_session_payload_size;

 re_recv:

	/* Read the NetBIOS session header (rfc-1002, section 4.3.1) */
	result = recvfrom (sock_fd, netbios_session_buf, 4, 0, NULL, NULL);
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

	netbios_session_payload_size = (int)smb_len (netbios_session_buf);

	#if defined(DUMP_SMB)
	{
		if(netbios_session_buf[0] != 0x00 && netbios_session_payload_size > 0)
		{
			if(netbios_session_payload_size > 256 - 4)
				netbios_session_payload_size = 256 - 4;

			result = recvfrom (sock_fd, &netbios_session_buf[4], netbios_session_payload_size - 4, 0, NULL, NULL);
			if (result < 0)
			{
				LOG (("smb_receive_raw: recv error = %ld\n", errno));
				result = (-errno);
				goto out;
			}

			if(result < netbios_session_payload_size - 4)
			{
				result = -EIO;
				goto out;
			}

			dump_netbios_header(__FILE__,__LINE__,netbios_session_buf,&netbios_session_buf[4],netbios_session_payload_size);
		}
		else
		{
			dump_netbios_header(__FILE__,__LINE__,netbios_session_buf,NULL,0);
		}
	}
	#endif /* defined(DUMP_SMB) */

	/* Check the session type. */
	switch (netbios_session_buf[0])
	{
		/* 0x00 == session message */
		case 0x00:

			break;

		/* 0x85 == session keepalive */
		case 0x85:

			LOG (("smb_receive_raw: Got SESSION KEEP ALIVE\n"));
			goto re_recv;

		/* 0x81 == session request */
		/* 0x82 == positive session response */
		/* 0x83 == negative session response */
		/* 0x84 == retarget session response */
		default:

			LOG (("smb_receive_raw: Invalid session header type 0x%02lx\n", netbios_session_buf[0]));
			result = -EIO;
			goto out;
	}

	/* The length in the RFC NB header is the raw data length (17 bits) */
	len = netbios_session_payload_size;
	if (len > max_raw_length)
	{
		LOG (("smb_receive_raw: Received length (%ld) > max_xmit (%ld)!\n", len, max_raw_length));
		result = -EIO;
		goto out;
	}

	/* Prepend the NetBIOS header to what is read? */
	if (want_header)
	{
		/* Check for buffer overflow. */
		if(len + 4 > max_raw_length)
		{
			LOG (("smb_receive_raw: Received length (%ld) > max_xmit (%ld)!\n", len, max_raw_length));
			result = -EIO;
			goto out;
		}
	
		memcpy (target, netbios_session_buf, 4);
		target += 4;
	}

	for(already_read = 0 ; already_read < len ; already_read += result)
	{
		result = recvfrom (sock_fd, (void *) (target + already_read), len - already_read, 0, NULL, NULL);
		if (result < 0)
		{
			LOG (("smb_receive_raw: recvfrom error = %ld\n", errno));

			result = (-errno);

			goto out;
		}
	}

	#if defined(DUMP_SMB)
	{
		/* If want_header==0 then this is the data returned by SMB_COM_READ_RAW. */
		dump_smb(__FILE__,__LINE__,!want_header,target,already_read,smb_packet_to_consumer,server->max_recv);
	}
	#endif /* defined(DUMP_SMB) */

	result = already_read;

 out:

	return result;
}

/* smb_receive
   fs points to the correct segment, server != NULL, sock!=NULL */
int
smb_receive (struct smb_server *server, int sock_fd)
{
	byte * packet = server->packet;
	int result;

	result = smb_receive_raw (server, sock_fd, packet,
	                          server->max_recv - 4, /* max_xmit in server includes NB header */
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

	if ((total_data > server->max_buffer_size) || (total_param > server->max_buffer_size))
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
		if (WVAL (inbuf, smb_prdisp) + WVAL (inbuf, smb_prcnt) > total_param)
		{
			LOG (("smb_receive_trans2: invalid parameters\n"));
			result = -EIO;
			goto fail;
		}

		if((*param) != NULL)
			memcpy ((*param) + WVAL (inbuf, smb_prdisp), smb_base (inbuf) + WVAL (inbuf, smb_proff), WVAL (inbuf, smb_prcnt));

		(*param_len) += WVAL (inbuf, smb_prcnt);

		if (WVAL (inbuf, smb_drdisp) + WVAL (inbuf, smb_drcnt) > total_data)
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
		if ((WVAL (inbuf, smb_tdrcnt) > total_data) || (WVAL (inbuf, smb_tprcnt) > total_param))
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
			/* smb_trans2_request() will check server->rcls, etc. and
			 * produce a matching error code value.
			 */
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
 * This routine was once taken from nfs, which is for udp. Here TCP does
 * most of the ugly stuff for us (thanks, Alan!)
 *
 ****************************************************************************/

/* Returns number of bytes received (>= 0) or a negative value in
 * case of error.
 */
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

	/* Length includes the NetBIOS session header (4 bytes), which
	 * is prepended to the packet to be sent.
	 */
	len = smb_len (buffer) + 4;

	LOG (("smb_request: len = %ld cmd = 0x%lx\n", len, buffer[8]));

	#if defined(DUMP_SMB)
	dump_netbios_header(__FILE__,__LINE__,buffer,&buffer[4],len);
	dump_smb(__FILE__,__LINE__,0,buffer+4,len-4,smb_packet_from_consumer,server->max_recv);
	#endif /* defined(DUMP_SMB) */

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

	/* Length includes the NetBIOS session header (4 bytes), which
	 * is prepended to the packet to be sent.
	 */
	len = smb_len (buffer) + 4;

	LOG (("smb_request: len = %ld cmd = 0x%02lx\n", len, buffer[8]));

	#if defined(DUMP_SMB)
	dump_netbios_header(__FILE__,__LINE__,buffer,NULL,0);
	dump_smb(__FILE__,__LINE__,0,buffer+4,len-4,smb_packet_from_consumer,server->max_recv);
	#endif /* defined(DUMP_SMB) */

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

	/* Length includes the NetBIOS session header (4 bytes), which
	 * is prepended to the packet to be sent.
	 */
	len = smb_len (buffer) + 4;

	LOG (("smb_request_read_raw: len = %ld cmd = 0x%02lx\n", len, buffer[8]));
	LOG (("smb_request_read_raw: target=%lx, max_len=%ld\n", (unsigned int) target, max_len));
	LOG (("smb_request_read_raw: buffer=%lx, sock=%lx\n", (unsigned int) buffer, (unsigned int) sock_fd));

	#if defined(DUMP_SMB)
	dump_netbios_header(__FILE__,__LINE__,buffer,NULL,0);
	dump_smb(__FILE__,__LINE__,0,buffer+4,len-4,smb_packet_from_consumer,server->max_recv);
	#endif /* defined(DUMP_SMB) */

	/* Request that data should be read in raw mode. */
	result = send (sock_fd, (void *) buffer, len, 0);

	LOG (("smb_request_read_raw: send returned %ld\n", result));

	if (result < 0)
	{
		LOG (("smb_request_read_raw: send error = %ld\n", errno));

		result = (-errno);
	}
	else
	{
		/* Wait for the raw data to be sent by the server. */
		result = smb_receive_raw (server, sock_fd, target, max_len, 0);
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

	/* Send the NetBIOS header. */
	smb_encode_smb_length (nb_header, length);

	result = send (sock_fd, (void *) nb_header, 4, 0);
	if (result == 4)
	{
		/* Now send the data to be written. */
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

	/* If the write operation succeeded, wait for the
	 * server to confirm it.
	 */
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
