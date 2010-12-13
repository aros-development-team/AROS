/* generic.h: */

#ifndef _GENERIC_H_
#define _GENERIC_H_

#include "cdrom.h"

typedef uint32_t t_ulong;
typedef unsigned short t_ushort;
typedef unsigned char t_uchar;
typedef int t_bool;

typedef struct path_node *t_path_list;

typedef enum protocol {
  PRO_UNKNOWN,
  PRO_ISO,
  PRO_HIGH_SIERRA,
  PRO_ROCK,
  PRO_HFS,
  PRO_JOLIET
} t_protocol;

typedef struct VOLUME {
  CDROM		 *cd;			/* Pointer to CDROM structure	*/
  t_protocol	 protocol;		/* Protocol used		*/
  struct handler *handler;		/* Pointer to handler struct	*/
  void		 *vol_info;		/* Depends on protocol		*/
  size_t	 vol_info_size;		/* Size of vol_info structure	*/
  t_bool	 mixed_char_filenames;	/* Filenames may contain upper
  					   and lower case characters    */
  /* for use by the device handler: */
#if 0
  t_bool	 valid;			/* Is the volume valid?		*/
#endif
  int		 locks;			/* Number of locks on this vol. */
  int		 file_handles;		/* Number of file handles on    */
  					/* this volume			*/
  struct DeviceList *devlist;		/* Associated DOS device list   */
  unsigned char  buffer[256];		/* Buffer for directory record  */
} VOLUME;

typedef struct CDROM_OBJ {
  t_bool		directory_f;	/* TRUE iff object is a directory     */
  t_bool		symlink_f;	/* TRUE iff object is a symbolic link */
  uint32_t		protection;	/* Amiga protection bits              */
  VOLUME		*volume;	/* Pointer to volume node	      */
  uint32_t		pos;		/* Current position (for read & seek) */
  t_path_list		pathlist;	/* List containing full path name     */
  void			*obj_info;	/* Depends on protocol		      */
} CDROM_OBJ;

typedef struct CDROM_INFO {
  t_bool		directory_f;	/* TRUE if object is a directory     */
  t_bool		symlink_f;	/* TRUE if object is a symbolic link */
  int			name_length;	/* length of file name		     */
  char			name[256];	/* file name			     */
  uint32_t		date;		/* creation date		     */
  uint32_t		file_length;	/* length of file		     */
  uint32_t		protection;	/* Amiga protection bits	     */
  int			comment_length; /* length of file comment	     */
  char			comment[256];	/* file comment			     */
  void			*suppl_info;	/* supplementary information	     */
} CDROM_INFO;

/* Codes: M=mandatory,
 *        O=optional (may be NULL)
 */

typedef struct handler {
  /*M*/ void       (*close_vol_info)(VOLUME *);
  /*M*/ CDROM_OBJ *(*open_top_level_directory)(VOLUME *);
  /*M*/ CDROM_OBJ *(*open_obj_in_directory)(CDROM_OBJ *, char *);
  /*M*/ CDROM_OBJ *(*find_parent)(CDROM_OBJ *);
  /*M*/ void       (*close_obj)(CDROM_OBJ *);
  /*M*/ int        (*read_from_file)(CDROM_OBJ *, char *, int);
  /*M*/ t_bool     (*cdrom_info)(CDROM_OBJ *, CDROM_INFO *);
  /*M*/ t_bool     (*examine_next)(CDROM_OBJ *, CDROM_INFO *, uint32_t *);
  /*M*/ void      *(*clone_obj_info)(void *);
  /*M*/ t_bool     (*is_top_level_obj)(CDROM_OBJ *);
  /*M*/ t_bool     (*same_objects)(CDROM_OBJ *, CDROM_OBJ *);
  /*O*/	t_ulong   (*creation_date)(VOLUME *);
  /*M*/ t_ulong    (*volume_size)(VOLUME *);
  /*M*/ void       (*volume_id)(VOLUME *, char *, int);
  /*M*/ t_ulong    (*location)(CDROM_OBJ *);
  /*M*/ t_ulong    (*file_length)(CDROM_OBJ *);
  /*M*/ t_ulong    (*block_size)(VOLUME *);
} t_handler;

#define ISOERR_NO_MEMORY        1       /* out of memory                */
#define ISOERR_SCSI_ERROR       2       /* scsi command return with err */
#define ISOERR_NO_PVD           3       /* prim volume descr not found  */
#define ISOERR_ILLEGAL_NAME     4       /* illegal path name            */
#define ISOERR_NO_SUCH_RECORD   5       /* no such record in path table */
#define ISOERR_NOT_FOUND        6       /* file not found               */
#define ISOERR_OFF_BOUNDS       7       /* bad seek operation           */
#define ISOERR_BAD_ARGUMENTS    8       /* bad arguments                */
#define ISOERR_IS_SYMLINK       9       /* cannot open symbolic links   */
#define ISOERR_INTERNAL        10       /* reason unknown               */

#define SEEK_FROM_START         -1      /* values for                   */
#define SEEK_FROM_CURRENT_POS   0       /* the 'Seek_Position'          */
#define SEEK_FROM_END           1       /* function                     */

extern int iso_errno;

t_protocol Which_Protocol
	(CDROM *p_cdrom, t_bool p_use_rock_ridge, t_bool p_use_joliet,
	 int *p_skip, t_ulong *p_offset, t_ulong *p_svd_offset);

VOLUME *Open_Volume(CDROM *p_cdrom, t_bool p_use_rock_ridge, t_bool p_use_joliet);
void Close_Volume(VOLUME *p_volume);

CDROM_OBJ *Open_Top_Level_Directory(VOLUME *p_volume);
CDROM_OBJ *Open_Object(CDROM_OBJ *p_current_dir, char *p_path_name);
void Close_Object(CDROM_OBJ *p_object);

int Read_From_File(CDROM_OBJ *p_file, char *p_buffer, int p_buffer_length);

int CDROM_Info(CDROM_OBJ *p_obj, CDROM_INFO *p_info);
t_bool Examine_Next
	(CDROM_OBJ *p_dir, CDROM_INFO *p_info, uint32_t *p_offset);

CDROM_OBJ *Clone_Object(CDROM_OBJ *p_object);
CDROM_OBJ *Find_Parent(CDROM_OBJ *p_object);

t_bool Is_Top_Level_Object (CDROM_OBJ *p_object);

int Seek_Position (CDROM_OBJ *p_object, long p_offset, int p_mode);

t_bool Same_Objects (CDROM_OBJ *p_object1, CDROM_OBJ *p_object2);
t_ulong Volume_Creation_Date(VOLUME *p_volume);
t_ulong Volume_Size (VOLUME *p_volume);
t_ulong Block_Size (VOLUME *p_volume);
void Volume_ID(VOLUME *p_volume, char *p_buffer, int p_buf_length );
t_ulong Location (CDROM_OBJ *p_object);
int Full_Path_Name(CDROM_OBJ *p_obj, char *p_buf, int p_buf_length);

int Strncasecmp(char *p_str1, char *p_str2, int p_length);
#endif /* _GENERIC_H_ */
