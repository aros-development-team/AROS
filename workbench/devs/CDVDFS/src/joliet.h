typedef struct sup_vol_desc {
  unsigned char		type;
  char			id[5];
  unsigned char		version;
  char			flags;
  char			system_id[32];
  char			volume_id[32];
  char			pad2[8];
#if !AROS_BIG_ENDIAN
  uint32_t		space_size;
  uint32_t		space_size_m;
#else
  uint32_t		space_size_i;
  uint32_t		space_size;
#endif
  char			escape[32];
#if !AROS_BIG_ENDIAN
  unsigned short	set_size;
  unsigned short	set_size_m;
#else
  unsigned short	set_size_i;
  unsigned short	set_size;
#endif
#if !AROS_BIG_ENDIAN
  unsigned short	sequence;
  unsigned short	sequence_m;
#else
  unsigned short	sequence_i;
  unsigned short	sequence;
#endif
#if !AROS_BIG_ENDIAN
  unsigned short	block_size;
  unsigned short	block_size_m;
#else
  unsigned short	block_size_i;
  unsigned short	block_size;
#endif
#if !AROS_BIG_ENDIAN
  uint32_t		path_size;
  uint32_t		path_size_m;
#else
  uint32_t		path_size_i;
  uint32_t		path_size;
#endif
#if !AROS_BIG_ENDIAN
  uint32_t         table;
  uint32_t         opt_table;
  uint32_t         m_table;
  uint32_t         opt_m_table;
#else
  uint32_t         i_table;
  uint32_t         opt_i_table;
  uint32_t         table;
  uint32_t         opt_table;
#endif
  directory_record      root;
  char			volume_set_id[128];
  char			publisher_id[128];
  char			data_preparer[128];
  char			application_id[128];
  char			copyright[37];
  char			abstract_file_id[37];
  char			bibliographic_id[37];
  time_and_date		vol_creation;
  time_and_date		vol_modification;
  time_and_date		vol_expiration;
  time_and_date		vol_effective;
  unsigned char		file_structure_version;
  char			pad4;
  char			application_use[512];
  char			reserved[653];
} sup_vol_desc;

t_bool Uses_Joliet_Protocol(CDROM *p_cdrom, t_ulong offset, t_ulong *p_svdoffset);
int Get_Joliet_Name(char *from, char *to, unsigned char len);

