/*
 * :ts=4
 *
 * dump_smb.h
 *
 * Copyright (C) 2016 by Olaf `Olsen' Barthel <obarthel -at- gmx -dot- net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if defined(DUMP_SMB)

/*****************************************************************************/

/* The command packets are identified by the command code, but the
 * contents are interpreted differently if the packets are sent by
 * the client ("From consumer") or by the server ("To consumer").
 */
enum smb_packet_source_t
{
	smb_packet_from_consumer,
	smb_packet_to_consumer
};

/*****************************************************************************/

/* Known SMB command codes. */
enum SMB_COM_T
{
	SMB_COM_CREATE_DIRECTORY=0x00,
	SMB_COM_DELETE_DIRECTORY=0x01,
	SMB_COM_OPEN=0x02,
	SMB_COM_CREATE=0x03,
	SMB_COM_CLOSE=0x04,
	SMB_COM_FLUSH=0x05,
	SMB_COM_DELETE=0x06,
	SMB_COM_RENAME=0x07,
	SMB_COM_QUERY_INFORMATION=0x08,
	SMB_COM_SET_INFORMATION=0x09,
	SMB_COM_READ=0x0A,
	SMB_COM_WRITE=0x0B,
	SMB_COM_LOCK_BYTE_RANGE=0x0C,
	SMB_COM_UNLOCK_BYTE_RANGE=0x0D,
	SMB_COM_CREATE_TEMPORARY=0x0E,
	SMB_COM_CREATE_NEW=0x0F,
	SMB_COM_CHECK_DIRECTORY=0x10,
	SMB_COM_PROCESS_EXIT=0x11,
	SMB_COM_SEEK=0x12,
	SMB_COM_LOCK_AND_READ=0x13,
	SMB_COM_WRITE_AND_UNLOCK=0x14,
	SMB_COM_READ_RAW=0x1A,
	SMB_COM_READ_MPX=0x1B,
	SMB_COM_READ_MPX_SECONDARY=0x1C,
	SMB_COM_WRITE_RAW=0x1D,
	SMB_COM_WRITE_MPX=0x1E,
	SMB_COM_WRITE_MPX_SECONDARY=0x1F,
	SMB_COM_WRITE_COMPLETE=0x20,
	SMB_COM_QUERY_SERVER=0x21,
	SMB_COM_SET_INFORMATION2=0x22,
	SMB_COM_QUERY_INFORMATION2=0x23,
	SMB_COM_LOCKING_ANDX=0x24,
	SMB_COM_TRANSACTION=0x25,
	SMB_COM_TRANSACTION_SECONDARY=0x26,
	SMB_COM_IOCTL=0x27,
	SMB_COM_IOCTL_SECONDARY=0x28,
	SMB_COM_COPY=0x29,
	SMB_COM_MOVE=0x2A,
	SMB_COM_ECHO=0x2B,
	SMB_COM_WRITE_AND_CLOSE=0x2C,
	SMB_COM_OPEN_ANDX=0x2D,
	SMB_COM_READ_ANDX=0x2E,
	SMB_COM_WRITE_ANDX=0x2F,
	SMB_COM_NEW_FILE_SIZE=0x30,
	SMB_COM_CLOSE_AND_TREE_DISC=0x31,
	SMB_COM_TRANSACTION2=0x32,
	SMB_COM_TRANSACTION2_SECONDARY=0x33,
	SMB_COM_FIND_CLOSE2=0x34,
	SMB_COM_FIND_NOTIFY_CLOSE=0x35,
	SMB_COM_TREE_CONNECT=0x70,
	SMB_COM_TREE_DISCONNECT=0x71,
	SMB_COM_NEGOTIATE=0x72,
	SMB_COM_SESSION_SETUP_ANDX=0x73,
	SMB_COM_LOGOFF_ANDX=0x74,
	SMB_COM_TREE_CONNECT_ANDX=0x75,
	SMB_COM_SECURITY_PACKAGE_ANDX=0x7E,
	SMB_COM_QUERY_INFORMATION_DISK=0x80,
	SMB_COM_SEARCH=0x81,
	SMB_COM_FIND=0x82,
	SMB_COM_FIND_UNIQUE=0x83,
	SMB_COM_FIND_CLOSE=0x84,
	SMB_COM_NT_TRANSACT=0xA0,
	SMB_COM_NT_TRANSACT_SECONDARY=0xA1,
	SMB_COM_NT_CREATE_ANDX=0xA2,
	SMB_COM_NT_CANCEL=0xA4,
	SMB_COM_NT_RENAME=0xA5,
	SMB_COM_OPEN_PRINT_FILE=0xC0,
	SMB_COM_WRITE_PRINT_FILE=0xC1,
	SMB_COM_CLOSE_PRINT_FILE=0xC2,
	SMB_COM_GET_PRINT_QUEUE=0xC3,
	SMB_COM_READ_BULK=0xD8,
	SMB_COM_WRITE_BULK=0xD9,
	SMB_COM_WRITE_BULK_DATA=0xDA,
	SMB_COM_INVALID=0xFE,
	SMB_COM_NO_ANDX_COMMAND=0xFF
};

/* SMB_COM_TRANSACTION2 subcommand codes. */
enum TRANS2_T
{
	TRANS2_OPEN2=0x00,
	TRANS2_FIND_FIRST2=0x01,
	TRANS2_FIND_NEXT2=0x02,
	TRANS2_QUERY_FS_INFORMATION=0x03,
	TRANS2_QUERY_PATH_INFORMATION=0x05,
	TRANS2_SET_PATH_INFORMATION=0x06,
	TRANS2_QUERY_FILE_INFORMATION=0x07,
	TRANS2_SET_FILE_INFORMATION=0x08,
	TRANS2_FSCTL=0x09,
	TRANS2_IOCTL2=0x0A,
	TRANS2_FIND_NOTIFY_FIRST=0x0B,
	TRANS2_FIND_NOTIFY_NEXT=0x0C,
	TRANS2_CREATE_DIRECTORY=0x0D,
	TRANS2_SESSION_SETUP=0x0E
};

enum SMB_FLAGS_T
{
	SMB_FLAGS_SERVER_TO_REDIR=0x80,
	SMB_FLAGS_REQUEST_BATCH_OPLOCK=0x40,
	SMB_FLAGS_REQUEST_OPLOCK=0x20,
	SMB_FLAGS_CANONICAL_PATHNAMES=0x10,
	SMB_FLAGS_CASELESS_PATHNAMES=0x08,
	SMB_FLAGS_CLIENT_BUF_AVAIL=0x02,
	SMB_FLAGS_SUPPORT_LOCKREAD=0x01
};

enum SMB_FLAGS2_T
{
	SMB_FLAGS2_UNICODE_STRINGS=0x8000,
	SMB_FLAGS2_32BIT_STATUS=0x4000,
	SMB_FLAGS2_READ_IF_EXECUTE=0x2000,
	SMB_FLAGS2_DFS_PATHNAME=0x1000,
	SMB_FLAGS2_EXTENDED_SECURITY=0x0800,
	SMB_FLAGS2_IS_LONG_NAME=0x0040,
	SMB_FLAGS2_SECURITY_SIGNATURE=0x0004,
	SMB_FLAGS2_EAS=0x0002,
	SMB_FLAGS2_KNOWS_LONG_NAMES=0x0001
};

struct smb_header
{
	unsigned char signature[4];	// Contains 0xFF, 'SMB'	[BYTE smb_idf[4]]
	unsigned char command;		// Command code [BYTE smb_com]
	unsigned long status;		// Error code class [BYTE smb_rcls], Reserved [BYTE smb_reh], Error code [WORD smb_err], 
	unsigned char flags;
	unsigned short flags2;
	
	struct
	{
		unsigned short pid_high;
		unsigned short signature[4];
	} extra;
	
	unsigned short tid;			// Tree ID # [WORD smb_tid]
	unsigned short pid;			// Callers process id [WORD smb_pid]
	unsigned short uid;			// User ID [WORD smb_uid]
	unsigned short mid;			// Multiplex ID [WORD smb_mid]

	int parameter_offset;		// Number of bytes between start of SMB header and parameters
	int num_parameter_words;	// Count of parameter words [BYTE smb_wct]
	unsigned char * parameters;	// Variable number of parameter words [SHORT smb_wvw[..]]

	int data_offset;			// Number of bytes between start of SMB header and data
	int num_data_bytes;			// Number of data bytes following [WORD smb_bcc]
	unsigned char * data;		// Variable number of data bytes [BYTE smb_data[..]]

	int raw_packet_size;		// Number of bytes in raw SMB packet
	unsigned char * raw_packet;	// Points to raw SMB packet
};

/*****************************************************************************/

extern void dump_netbios_header(const char *file_name,int line_number,
	const unsigned char *netbios_session_header,
	const unsigned char *netbios_payload,int netbios_payload_size);

extern void dump_smb(const char *file_name,int line_number,int is_raw_data,
	const void * packet,int length,enum smb_packet_source_t smb_packet_source,
	int max_buffer_size);

extern void control_smb_dump(int enable);

/*****************************************************************************/

#endif /* DUMP_SMB */
