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

/* A successful send may transfer only a prefix of a TCP frame. */
static int
smb_send_all (int sock_fd, const unsigned char *source, int length)
{
	int sent = 0;

	while (sent < length)
	{
		int result = send (sock_fd, (void *)(source + sent), length - sent, 0);

		/* Preserve socket errors, including the Ctrl-C break indication. */
		if (result < 0)
			return -errno;
		if (result == 0)
			return -EIO;

		sent += result;
	}

	return sent;
}

/* TCP may split any part of a session frame across several reads. */
static int
smb_receive_all (int sock_fd, unsigned char *target, int length)
{
	int received = 0;

	while (received < length)
	{
		int result = recvfrom (sock_fd, target + received, length - received, 0, NULL, NULL);

		/* EINTR also reports Ctrl-C through the configured socket break mask. */
		if (result < 0)
			return -errno;

		/* An orderly close cannot complete the remaining frame. */
		if (result == 0)
			return -EIO;

		received += result;
	}

	return received;
}

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
	result = smb_receive_all (sock_fd, netbios_session_buf, 4);
	if (result < 0)
		goto out;

	netbios_session_payload_size = (int)smb_len (netbios_session_buf);

	#if defined(DUMP_SMB)
	dump_netbios_header(__FILE__,__LINE__,netbios_session_buf,NULL,0);
	#endif /* defined(DUMP_SMB) */

	/* Check the session type. */
	switch (netbios_session_buf[0])
	{
		/* 0x00 == session message */
		case 0x00:

			break;

		/* 0x85 == session keepalive */
		case 0x85:

			/* Keepalives have no payload to leave in the byte stream. */
			if (netbios_session_payload_size != 0)
			{
				result = -EIO;
				goto out;
			}
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

	already_read = smb_receive_all (sock_fd, target, len);
	if (already_read < 0)
	{
		result = already_read;
		goto out;
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

/* Transaction offsets are relative to the SMB header, not the NetBIOS
 * header. Verify each fragment before reading its words or copying its data.
 */
static int
smb_valid_trans2_response (const byte *packet, int payload_length)
{
	int packet_length = payload_length + 4;
	int word_count;
	int bytes_offset;
	int data_start;
	int data_end;
	int param_count, param_offset;
	int data_count, data_offset;

	if (packet_length < SMB_HEADER_LEN + 10 * 2 + 2 ||
	    packet[4] != 0xff || packet[5] != 'S' ||
	    packet[6] != 'M' || packet[7] != 'B' ||
	    packet[8] != SMBtrans2)
		return -EIO;

	word_count = packet[SMB_HEADER_LEN - 1];
	if (word_count < 10 || packet_length < SMB_HEADER_LEN + word_count * 2 + 2)
		return -EIO;

	bytes_offset = SMB_HEADER_LEN + word_count * 2;
	if (packet_length < bytes_offset + 2 + WVAL(packet,bytes_offset))
		return -EIO;

	data_start = bytes_offset + 2 - 4;
	data_end = data_start + WVAL(packet,bytes_offset);
	param_count = WVAL(packet,smb_prcnt);
	param_offset = WVAL(packet,smb_proff);
	data_count = WVAL(packet,smb_drcnt);
	data_offset = WVAL(packet,smb_droff);

	if ((param_count != 0 &&
	     (param_offset < data_start || param_offset > data_end ||
	      param_count > data_end - param_offset)) ||
	    (data_count != 0 &&
	     (data_offset < data_start || data_offset > data_end ||
	      data_count > data_end - data_offset)))
		return -EIO;

	return 0;
}

/* Count unique bytes, since retransmitted or overlapping fragments do not
 * fill gaps in a transaction response.
 */
static int
smb_mark_trans2_bytes (byte *covered, int offset, int count)
{
	int i, added = 0;

	for (i = 0; i < count; i++)
	{
		int position = offset + i;
		byte mask = 1 << (position & 7);

		if ((covered[position >> 3] & mask) == 0)
		{
			covered[position >> 3] |= mask;
			added++;
		}
	}

	return added;
}

static int
smb_count_trans2_bytes (const byte *covered, int length)
{
	int i, count = 0;

	for (i = 0; i < length; i++)
		if (covered[i >> 3] & (1 << (i & 7)))
			count++;

	return count;
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
	byte *data_covered = NULL;
	byte *param_covered = NULL;

	LOG (("smb_receive_trans2: enter\n"));

	(*data_len) = (*param_len) = 0;
	(*param) = (*data) = NULL;

	result = smb_receive (server, sock_fd);
	if (result < 0)
		goto fail;
	if (result < 9)
	{
		result = -EIO;
		goto fail;
	}

	if (server->rcls != 0)
		goto fail;

	if (smb_valid_trans2_response(inbuf,result) != 0)
	{
		result = -EIO;
		goto fail;
	}

	/* parse out the lengths */
	total_data = WVAL (inbuf, smb_tdrcnt);
	total_param = WVAL (inbuf, smb_tprcnt);

	if ((total_data > server->max_buffer_size) || (total_param > server->max_buffer_size))
	{
		LOG (("smb_receive_trans2: data/param too long\n"));

		result = -EIO;
		goto fail;
	}

	/* A complete first fragment needs no coverage map or per-byte work. */
	if (WVAL (inbuf, smb_drcnt) < total_data)
	{
		data_covered = malloc ((total_data + 7) / 8);
		if (data_covered == NULL)
		{
			result = -ENOMEM;
			goto fail;
		}
		memset (data_covered, 0, (total_data + 7) / 8);
	}
	if (WVAL (inbuf, smb_prcnt) < total_param)
	{
		param_covered = malloc ((total_param + 7) / 8);
		if (param_covered == NULL)
		{
			result = -ENOMEM;
			goto fail;
		}
		memset (param_covered, 0, (total_param + 7) / 8);
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

		if (param_covered != NULL)
			(*param_len) += smb_mark_trans2_bytes (param_covered,
				WVAL (inbuf, smb_prdisp), WVAL (inbuf, smb_prcnt));
		else
			(*param_len) += WVAL (inbuf, smb_prcnt);

		if (WVAL (inbuf, smb_drdisp) + WVAL (inbuf, smb_drcnt) > total_data)
		{
			LOG (("smb_receive_trans2: invalid data block\n"));
			result = -EIO;
			goto fail;
		}

		if((*data) != NULL)
			memcpy ((*data) + WVAL (inbuf, smb_drdisp), smb_base (inbuf) + WVAL (inbuf, smb_droff), WVAL (inbuf, smb_drcnt));

		if (data_covered != NULL)
			(*data_len) += smb_mark_trans2_bytes (data_covered,
				WVAL (inbuf, smb_drdisp), WVAL (inbuf, smb_drcnt));
		else
			(*data_len) += WVAL (inbuf, smb_drcnt);

		LOG (("smb_rec_trans2: drcnt/prcnt: %ld/%ld\n", WVAL (inbuf, smb_drcnt), WVAL (inbuf, smb_prcnt)));

		/* parse out the total lengths again - they can shrink! */
		if ((WVAL (inbuf, smb_tdrcnt) > total_data) || (WVAL (inbuf, smb_tprcnt) > total_param))
		{
			LOG (("smb_receive_trans2: data/params grew!\n"));
			result = -EIO;
			goto fail;
		}

		if (data_covered != NULL && WVAL (inbuf, smb_tdrcnt) < total_data)
			(*data_len) = smb_count_trans2_bytes (data_covered, WVAL (inbuf, smb_tdrcnt));
		if (param_covered != NULL && WVAL (inbuf, smb_tprcnt) < total_param)
			(*param_len) = smb_count_trans2_bytes (param_covered, WVAL (inbuf, smb_tprcnt));
		total_data = WVAL (inbuf, smb_tdrcnt);
		total_param = WVAL (inbuf, smb_tprcnt);
		if ((*data_len) > total_data)
			(*data_len) = total_data;
		if ((*param_len) > total_param)
			(*param_len) = total_param;
		if (total_data <= (*data_len) && total_param <= (*param_len))
			break;

		result = smb_receive (server, sock_fd);
		if (result < 0)
			goto fail;
		if (result < 9)
		{
			result = -EIO;
			goto fail;
		}

		if (server->rcls != 0)
		{
			/* smb_trans2_request() will check server->rcls, etc. and
			 * produce a matching error code value.
			 */
			result = -EIO;
			goto fail;
		}
		if (smb_valid_trans2_response(inbuf,result) != 0)
		{
			result = -EIO;
			goto fail;
		}
	}

	LOG (("smb_receive_trans2: normal exit\n"));
	free (data_covered);
	free (param_covered);
	return 0;

 fail:

	LOG (("smb_receive_trans2: failed exit\n"));

	if((*param) != NULL)
		free (*param);

	if((*data) != NULL)
		free (*data);
	free (data_covered);
	free (param_covered);

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

/* A NetBIOS session reply is a four-byte control frame, not an SMB reply. */
int
smb_request_session (struct smb_server *server)
{
	int sock_fd = server->mount_data.fd;
	byte *packet = server->packet;
	byte reply[4];
	int result;

	if (sock_fd < 0 || packet == NULL)
	{
		result = -EBADF;
		goto out;
	}
	if (server->state != CONN_VALID)
	{
		result = -EIO;
		goto out;
	}

	result = smb_send_all (sock_fd, packet, smb_len (packet) + 4);
	if (result < 0)
		goto out;

	result = smb_receive_all (sock_fd, reply, sizeof(reply));
	if (result < 0)
		goto out;

	#if defined(DUMP_SMB)
	dump_netbios_header(__FILE__,__LINE__,reply,NULL,0);
	#endif

	if (reply[0] != 0x82 || reply[1] != 0 || smb_len(reply) != 0)
	{
		result = -EIO;
		goto out;
	}

	memcpy(packet,reply,sizeof(reply));
	result = 0;

 out:
	if (result < 0)
	{
		server->state = CONN_INVALID;
		smb_invalidate_all_inodes(server);
	}

	return result;
}

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

	result = smb_send_all (sock_fd, buffer, len);
	if (result >= 0)
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

	result = smb_send_all (sock_fd, buffer, len);
	if (result >= 0)
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
	result = smb_send_all (sock_fd, buffer, len);

	LOG (("smb_request_read_raw: send returned %ld\n", result));

	if (result >= 0)
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

	result = smb_send_all (sock_fd, nb_header, 4);
	if (result >= 0)
		result = smb_send_all (sock_fd, source, length);

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
