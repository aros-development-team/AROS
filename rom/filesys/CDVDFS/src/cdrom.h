/* cdrom.h: */

/*
 * History:
 *
 * 06-Mar-09 error     - Removed madness, fixed insanity. Cleanup started
 */

#ifndef _CDROM_H_
#define _CDROM_H_

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <devices/scsidisk.h>

#include <inttypes.h>

#define VERSION "CDROM-Handler 1.15 (03.11.94)"

#define SCSI_BUFSIZE 2048

#ifdef __GNUC__
#define __packed    __attribute__((packed))
#else
#define __packed
#endif

typedef struct CDROM {
  struct CDVDBase *global;
  unsigned char 	*buffer;
  unsigned char		**buffers;
  unsigned char 	*buffer_data;
  unsigned char		*buffer_io;
  unsigned char		sense[20];
  short			lun;
  short			buffers_cnt;
  uint32_t		t_changeint;
  uint32_t		t_changeint2;
  long 			*current_sectors;
  uint32_t		*last_used;
  uint32_t		tick;
  struct MsgPort 	*port;
  struct IOStdReq	*scsireq;
  struct SCSICmd	cmd;
  short			device_open;
  struct IOStdReq *iochangeint;
  struct Interrupt changeint;

} CDROM;

typedef struct inquiry_data {
  char		peripheral_type;
  char		modifier;
  char		version;
  char		flags1;
  char		additional_length;
  char		reserved[2];
  char		flags2;
  char		vendor[8];
  char		product[16];
  char		revision[4];
} __packed t_inquiry_data;

typedef struct toc_header {
  unsigned short length;
  unsigned char  first_track;
  unsigned char  last_track;
} __packed t_toc_header;

typedef struct toc_data {
  char		reserved1;
  unsigned char flags;
  unsigned char	track_number;
  char		reserved2;
  uint32_t address;
} __packed t_toc_data;

CDROM *Open_CDROM
	(
		struct CDVDBase *global,
		char *p_device,
		int p_scsi_id,
		uint32_t p_memory_type,
		int p_std_buffers,
		int p_file_buffers
	);

int Read_Chunk(CDROM *p_cd, long p_sector);
void Cleanup_CDROM(CDROM *p_cd);
int Test_Unit_Ready(CDROM *p_cd);
int Mode_Select(CDROM *p_cd, int p_on, int p_block_length);
int Inquire(CDROM *p_cd, t_inquiry_data *p_data);
t_toc_data *Read_TOC
	(
		CDROM *p_cd,
		uint32_t *p_toc_len
	);
int Has_Audio_Tracks(CDROM *p_cd);
int Data_Tracks(CDROM *p_cd, uint32_t** p_buf);
void block2msf (uint32_t blk, unsigned char *msf);
int Start_Play_Audio(CDROM *p_cd);
int Stop_Play_Audio(CDROM *p_cd);
void Clear_Sector_Buffers (CDROM *p_cd);
int Find_Last_Session(CDROM *p_cd, uint32_t *p_result);

enum {
  CDROMERR_OK = 0,	/* no error			*/
  CDROMERR_NO_MEMORY,	/* out of memory		*/
  CDROMERR_MSGPORT,	/* cannot create message port	*/
  CDROMERR_IOREQ,	/* cannot create I/O request	*/
  CDROMERR_DEVICE,	/* cannot open scsi device	*/
  CDROMERR_BLOCKSIZE	/* illegal blocksize		*/
};

#endif /* _CDROM_H_ */
