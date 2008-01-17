#ifndef SLB_H_
#define SLB_H_

#include <exec/types.h>

#define CALLBACK_VERSION 4

typedef void * SCAN_HANDLE; /* Change it! */
typedef LONG size_t;
typedef void cmd_tbl_t;
typedef void block_dev_desc_t;
typedef void disk_partition_t;

#define SLB_NO_ERROR             0
#define SLB_ERROR_NO_MEMORY     -1
#define SLB_ERROR_OLD_SLB       -2
#define SLB_ERROR_OLD_UBOOT     -3
#define SLB_ERROR_NO_CONFIG     -4
#define SLB_ERROR_LOAD_ERROR    -5


typedef struct sbl_context {
    ULONG ssc_version;
    void (*ssc_printf_like)(const char *fmtstring,...);
    int (* ssc_getc_like)(void);

    void * ssc_scan_list; 
    struct MinList * ssc_devices_list;
    SCAN_HANDLE ssc_curr_device;

    SCAN_HANDLE (* ssc_start_unit_scan)(const void * scan_list, ULONG * const blocksize);
    SCAN_HANDLE (* ssc_next_unit_scan)(SCAN_HANDLE h, ULONG * const blocksize);
    BOOL (* ssc_open_specific_unit)(const SCAN_HANDLE h);
    void (* ssc_end_unit_scan)(SCAN_HANDLE h);
    void (* ssc_end_global_scan)(void);
    BOOL (* ssc_loadsector)(const ULONG sectn, const ULONG sect_size,
                            const ULONG numb_sects, void * const dest_buf);

    int (* ssc_my_netloop)(char * filename, void * dump_here);

    char * (* ssc_getenv)(unsigned char *);
    void (* ssc_setenv)(char *, char *);

    void * (* ssc_alloc_mem_for_iobuffers)(const size_t size);
    void * (* ssc_alloc_mem_for_kickmodule)(const size_t size);
    void * (* ssc_alloc_mem_for_execNG)(const size_t size);
    void * (* ssc_alloc_mem_for_anythingelse)(const size_t size);
    void * (* ssc_alloc_mem_for_bootloader)(const size_t size);
    void (* ssc_free_mem)(void * const loc);

    void * (* ssc_get_board_info)(void);
    
    int (* ssc_BZ2_bzBuffToBuffDecompress) (
            char*         dest,
            unsigned int* destLen,
            char*         source,
            unsigned int  sourceLen,
            int           small,
            int           verbosity
            );

    void (* ssc_video_clear)(void);
    void (* ssc_video_draw_box)(int style, int attr, char *title, int separate, int x, int y, int w, int h);

    void (* ssc_video_draw_text)(int x, int y, int attr, char *text, int field);

    void (* ssc_video_repeat_char)(int x, int y, int repcnt, int repchar, int attr);

    unsigned short (* ssc_set_partial_scroll_limits)(const short start, const short end);
    void (* ssc_get_partial_scroll_limits)(short * const start, short * const end);
    int (* ssc_video_get_key)(void);

    int (* ssc_do_bootm)(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
    void * (* ssc_memmove)(void * dest,const void *src,size_t count);
    void (* ssc_set_load_addr)(void * const la);

    int (* ssc_tstc)(void);
    void (* ssc_udelay)(unsigned long);
    int (* ssc_sprintf)(char * buf, const char *fmt, ...);

    int (* ssc_ext2fs_set_blk_dev_full)(block_dev_desc_t * const rbdd, disk_partition_t * const p);
    int (* ssc_ext2fs_open)(char *filename);
    int (* ssc_ext2fs_read)(char *buf, unsigned len);
    int (* ssc_ext2fs_mount)(unsigned part_length);
    int (* ssc_ext2fs_close)(void);

} sbl_context_t;

#endif /*SLB_H_*/
