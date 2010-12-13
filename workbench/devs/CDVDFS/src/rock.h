/* rock.h: */

#ifndef _ROCK_H
#define _ROCK_H

#include "cdrom.h"
#include "iso9660.h"

t_bool Uses_Rock_Ridge_Protocol(VOLUME *p_volume, int *p_skip);
int Get_RR_File_Name(VOLUME *p_volume, directory_record *p_dir, char *p_buf, int p_buf_len);
int Get_RR_File_Comment(VOLUME *p_volume, directory_record *p_dir, uint32_t *p_prot, char *p_buf, int p_buf_len);
int Is_A_Symbolic_Link(VOLUME *p_volume, directory_record *p_dir, uint32_t *amiga_mode);
t_bool Get_Link_Name(CDROM_OBJ *p_obj, char *p_buf, int p_buf_len);
int Has_System_Use_Field(VOLUME *p_volume, directory_record *p_dir, char *p_name);
LONG RR_Child_Link(VOLUME *p_volume, directory_record *p_dir);
LONG RR_Parent_Link(VOLUME *p_volume, directory_record *p_dir);

#endif /* _ROCK_H */
