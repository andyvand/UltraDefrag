/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _UDEFRAG_INTERNALS_H_
#define _UDEFRAG_INTERNALS_H_

#include "../zenwinx/zenwinx.h"
#include "udefrag.h"
#include "../../include/ultradfgver.h"

#define DEFAULT_COLOR SYSTEM_SPACE

/************************************************************/
/*             Constants affecting performance              */
/************************************************************/

/*
* Fragment size threshold for partial defragmentation.
*/
#define PART_DEFRAG_MAGIC_CONSTANT  (20 * 1024 * 1024)

/*
* Default file size threshold for disk optimization.
*/
#define OPTIMIZER_MAGIC_CONSTANT    (20 * 1024 * 1024)

/*
* A magic constant for cut_off_group_of_files routine.
*/
#define OPTIMIZER_MAGIC_CONSTANT_N  10

/*
* Another magic constant for
* cut_off_sorted_out_files routine.
*/
#define OPTIMIZER_MAGIC_CONSTANT_M  1

/************************************************************/
/*                Prototypes, constants etc.                */
/************************************************************/

#ifndef DebugPrint
#define DebugPrint winx_dbg_print
#endif

#define MAX_FILE_SIZE ((ULONGLONG) -1)
#define DEFAULT_FRAGMENT_SIZE_THRESHOLD (MAX_FILE_SIZE / 2)

/* flags for user_defined_flags member of filelist entries */
#define UD_FILE_EXCLUDED         0x1
#define UD_FILE_OVER_LIMIT       0x2

/* file status flags */
#define UD_FILE_LOCKED           0x4   /* file is locked by system */
#define UD_FILE_MOVING_FAILED    0x10  /* file moving completed with failure */
#define UD_FILE_IMPROPER_STATE   0x20  /* file is in improper state (chkdsk run needed) or some bug encountered */

/*
* This flag is used to skip already processed
* files in individual volume processing tasks.
*/
#define UD_FILE_CURRENTLY_EXCLUDED      0x40

/*
* This flag is used to speed things up.
* If we'll repeatedly check the file
* we'll noticeably slow down all the
* volume procesing routines.
*/
#define UD_FILE_NOT_LOCKED              0x80

/*
* This flag is used to avoid 
* repeated moves in volume optimization.
*/
#define UD_FILE_MOVED_TO_FRONT         0x100

/*
* This flag is used to mark files
* fragmented by MFT optimizer.
*/
#define UD_FILE_FRAGMENTED_BY_FILE_OPT 0x200

#define UD_FILE_EXCLUDED_BY_PATH       0x400

/*
* Some essential DOS files need to be
* at fixed locations on disk, so we're
* skipping them to keep DOS bootable.
*/
#define UD_FILE_ESSENTIAL_DOS_FILE     0x800

/*
* Auxiliary flag for move_files_to_front routine.
*/
#define UD_FILE_REGION_NOT_FOUND       0x1000

#define is_excluded(f)           ((f)->user_defined_flags & UD_FILE_EXCLUDED)
#define is_over_limit(f)         ((f)->user_defined_flags & UD_FILE_OVER_LIMIT)
#define is_locked(f)             ((f)->user_defined_flags & UD_FILE_LOCKED)
#define is_moving_failed(f)      ((f)->user_defined_flags & UD_FILE_MOVING_FAILED)
#define is_in_improper_state(f)  ((f)->user_defined_flags & UD_FILE_IMPROPER_STATE)
#define is_currently_excluded(f) ((f)->user_defined_flags & UD_FILE_CURRENTLY_EXCLUDED)
#define is_moved_to_front(f)     ((f)->user_defined_flags & UD_FILE_MOVED_TO_FRONT)
#define is_fragmented_by_file_opt(f) ((f)->user_defined_flags & UD_FILE_FRAGMENTED_BY_FILE_OPT)
#define is_excluded_by_path(f)   ((f)->user_defined_flags & UD_FILE_EXCLUDED_BY_PATH)
#define is_essential_dos_file(f) ((f)->user_defined_flags & UD_FILE_ESSENTIAL_DOS_FILE)

#define is_block_excluded(b)     ((b)->length == 0)

/*
* MSDN states that environment variables
* are limited by 32767 characters,
* including terminal zero.
*/
#define ENV_BUFFER_SIZE 32767

typedef struct _udefrag_options {
    winx_patlist in_filter;     /* patterns for file inclusion */
    winx_patlist ex_filter;     /* patterns for file exclusion */
    ULONGLONG fragment_size_threshold;  /* fragment size threshold */
    ULONGLONG size_limit;       /* file size threshold */
    ULONGLONG optimizer_size_limit; /* file size threshold used in disk optimization */
    ULONGLONG fragments_limit;  /* file fragments threshold */
    ULONGLONG time_limit;       /* processing time limit, in seconds */
    int refresh_interval;       /* progress refresh interval, in milliseconds */
    int disable_reports;        /* nonzero value forces fragmentation reports to be disabled */
    int dbgprint_level;         /* controls amount of debugging information */
    int dry_run;                /* set %UD_DRY_RUN% variable to avoid actual data moving in tests */
    int job_flags;              /* flags triggering algorithm features */
    int algorithm_defined_fst;  /* nonzero value indicates that the fragment size
                                   threshold is set by algorithm and not by user */
} udefrag_options;

typedef struct _udefrag_fragmented_file {
    struct _udefrag_fragmented_file *next;
    struct _udefrag_fragmented_file *prev;
    winx_file_info *f;
} udefrag_fragmented_file;

typedef enum {
    FS_UNKNOWN = 0, /* ext2 and others */
    FS_FAT12,
    FS_FAT16,
    FS_FAT32,
    FS_EXFAT,
    FS_NTFS,
    FS_UDF
} file_system_type;

/*
* More details at http://www.thescripts.com/forum/thread617704.html
* ('Dynamically-allocated Multi-dimensional Arrays - C').
*/
typedef struct _cmap {
    ULONGLONG (*array)[NUM_OF_SPACE_STATES];
    ULONGLONG field_size;
    int map_size;
    int n_colors;
    ULONGLONG clusters_per_cell;
    ULONGLONG clusters_per_last_cell;
    BOOLEAN opposite_order; /* clusters < cells */
    ULONGLONG cells_per_cluster;
    ULONGLONG unused_cells;
} cmap;

struct performance_counters {
    ULONGLONG overall_time;               /* time needed for volume processing */
    ULONGLONG analysis_time;              /* time needed for volume analysis */
    ULONGLONG searching_time;             /* time needed for searching */
    ULONGLONG moving_time;                /* time needed for file moves */
    ULONGLONG temp_space_releasing_time;  /* time needed to release space temporarily allocated by system */
};

#define TINY_FILE_SIZE            0 * 1024  /* < 10 KB */
#define SMALL_FILE_SIZE          10 * 1024  /* 10 - 100 KB */
#define AVERAGE_FILE_SIZE       100 * 1024  /* 100 KB - 1 MB */
#define BIG_FILE_SIZE          1024 * 1024  /* 1 - 16 MB */
#define HUGE_FILE_SIZE    16 * 1024 * 1024  /* 16 - 128 MB */
#define GIANT_FILE_SIZE  128 * 1024 * 1024  /* > 128 MB */

struct file_counters {
    unsigned long tiny_files;
    unsigned long small_files;
    unsigned long average_files;
    unsigned long big_files;
    unsigned long huge_files;
    unsigned long giant_files;
};

typedef int  (*udefrag_termination_router)(void /*udefrag_job_parameters*/ *p);

typedef struct _udefrag_job_parameters {
    unsigned char volume_letter;                /* volume letter */
    udefrag_job_type job_type;                  /* type of requested job */
    udefrag_progress_callback cb;               /* progress update callback */
    udefrag_terminator t;                       /* termination callback */
    void *p;                                    /* pointer to user defined data to be passed to both callbacks */
    udefrag_termination_router termination_router;  /* address of procedure triggering job termination */
    ULONGLONG start_time;                       /* time of the job launch */
    ULONGLONG progress_refresh_time;            /* time of the last progress refresh */
    udefrag_options udo;                        /* job options */
    udefrag_progress_info pi;                   /* progress counters */
    winx_volume_information v_info;             /* basic volume information */
    file_system_type fs_type;                   /* type of volume file system */
    int is_fat;                                 /* nonzero value indicates that the file system is a kind of FAT */
    winx_file_info *filelist;                   /* list of files */
    udefrag_fragmented_file *fragmented_files;  /* list of fragmented files; does not contain filtered out files */
    winx_volume_region *free_regions;           /* list of free space regions */
    unsigned long free_regions_count;           /* number of free space regions */
    ULONGLONG clusters_at_once;                 /* number of clusters to be moved at once */
    cmap cluster_map;                           /* cluster map internal data */
    WINX_FILE *fVolume;                         /* handle of the volume, used by file moving routines */
    struct performance_counters p_counters;     /* performance counters */
    struct prb_table *file_blocks;              /* pointer to binary tree of all file blocks found on the volume */
    struct file_counters f_counters;            /* file counters */
    NTSTATUS last_move_status;                  /* status of the last move file operation; zero by default */
    ULONGLONG already_optimized_clusters;       /* number of clusters needing no sorting in optimization */
} udefrag_job_parameters;

int  get_options(udefrag_job_parameters *jp);
void release_options(udefrag_job_parameters *jp);

int save_fragmentation_report(udefrag_job_parameters *jp);
void remove_fragmentation_report(udefrag_job_parameters *jp);

void dbg_print_file_counters(udefrag_job_parameters *jp);

int allocate_map(int map_size,udefrag_job_parameters *jp);
void free_map(udefrag_job_parameters *jp);
void reset_cluster_map(udefrag_job_parameters *jp);
void colorize_map_region(udefrag_job_parameters *jp,
        ULONGLONG lcn, ULONGLONG length, int new_color, int old_color);
void colorize_file(udefrag_job_parameters *jp, winx_file_info *f, int old_color);
int get_file_color(udefrag_job_parameters *jp, winx_file_info *f);
void release_temp_space_regions(udefrag_job_parameters *jp);
void redraw_all_temporary_system_space_as_free(udefrag_job_parameters *jp);

int analyze(udefrag_job_parameters *jp);
int defragment(udefrag_job_parameters *jp);
int optimize(udefrag_job_parameters *jp);
int optimize_mft(udefrag_job_parameters *jp);
void destroy_lists(udefrag_job_parameters *jp);

ULONGLONG start_timing(char *operation_name,udefrag_job_parameters *jp);
void stop_timing(char *operation_name,ULONGLONG start_time,udefrag_job_parameters *jp);

int check_region(udefrag_job_parameters *jp,ULONGLONG lcn,ULONGLONG length);

NTSTATUS udefrag_fopen(winx_file_info *f,HANDLE *phFile);
int is_file_locked(winx_file_info *f,udefrag_job_parameters *jp);
int is_mft(winx_file_info *f,udefrag_job_parameters *jp);

int exclude_by_fragment_size(winx_file_info *f,udefrag_job_parameters *jp);
int exclude_by_fragments(winx_file_info *f,udefrag_job_parameters *jp);
int exclude_by_size(winx_file_info *f,udefrag_job_parameters *jp);
int expand_fragmented_files_list(winx_file_info *f,udefrag_job_parameters *jp);
void truncate_fragmented_files_list(winx_file_info *f,udefrag_job_parameters *jp);
winx_blockmap *build_fragments_list(winx_file_info *f,ULONGLONG *n_fragments);
void release_fragments_list(winx_blockmap **fragments);
ULONGLONG defrag_cc_routine(udefrag_job_parameters *jp);

int move_file(winx_file_info *f,
              ULONGLONG vcn,
              ULONGLONG length,
              ULONGLONG target,
              udefrag_job_parameters *jp
              );
int can_move(winx_file_info *f,udefrag_job_parameters *jp);
int can_move_entirely(winx_file_info *f,udefrag_job_parameters *jp);

/* flags for find_matching_free_region */
enum {
    FIND_MATCHING_RGN_FORWARD,
    FIND_MATCHING_RGN_BACKWARD,
    FIND_MATCHING_RGN_ANY
};
winx_volume_region *find_first_free_region(udefrag_job_parameters *jp,ULONGLONG min_lcn,ULONGLONG min_length);
winx_volume_region *find_last_free_region(udefrag_job_parameters *jp,ULONGLONG min_lcn,ULONGLONG min_length);
winx_volume_region *find_largest_free_region(udefrag_job_parameters *jp);
/*winx_volume_region *find_matching_free_region(udefrag_job_parameters *jp,
    ULONGLONG start_lcn, ULONGLONG min_length, int preferred_position);
*/

int create_file_blocks_tree(udefrag_job_parameters *jp);
int add_block_to_file_blocks_tree(udefrag_job_parameters *jp, winx_file_info *file, winx_blockmap *block);
int remove_block_from_file_blocks_tree(udefrag_job_parameters *jp, winx_blockmap *block);
void destroy_file_blocks_tree(udefrag_job_parameters *jp);
winx_blockmap *find_first_block(udefrag_job_parameters *jp,
    ULONGLONG *min_lcn, int flags, winx_file_info **first_file);

/* flags for the find_first_block routine */
enum {
    SKIP_PARTIALLY_MOVABLE_FILES = 0x1
};

#endif /* _UDEFRAG_INTERNALS_H_ */
