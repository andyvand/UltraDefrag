/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
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

/*
* Udefrag.dll interface header.
*/

#ifndef _UDEFRAG_H_
#define _UDEFRAG_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "../../include/dbg.h"

/* debug print levels */
#define DBG_NORMAL     0
#define DBG_DETAILED   1
#define DBG_PARANOID   2

/* UltraDefrag error codes */
#define UDEFRAG_UNKNOWN_ERROR     (-1)
#define UDEFRAG_W2K_4KB_CLUSTERS  (-3)
#define UDEFRAG_NO_MEM            (-4)
#define UDEFRAG_CDROM             (-5)
#define UDEFRAG_REMOTE            (-6)
#define UDEFRAG_ASSIGNED_BY_SUBST (-7)
#define UDEFRAG_REMOVABLE         (-8)
#define UDEFRAG_UDF_DEFRAG        (-9)
#define UDEFRAG_DIRTY_VOLUME      (-12)

#define DEFAULT_REFRESH_INTERVAL 100

#define MAX_DOS_DRIVES 26
#define MAXFSNAME      32  /* I think, that's enough */

int udefrag_init_library(void);
void udefrag_unload_library(void);

typedef struct _volume_info {
    char letter;
    char fsname[MAXFSNAME];
    wchar_t label[MAX_PATH + 1];
    LARGE_INTEGER total_space;
    LARGE_INTEGER free_space;
    int is_removable;
    int is_dirty;
} volume_info;

volume_info *udefrag_get_vollist(int skip_removable);
void udefrag_release_vollist(volume_info *v);
int udefrag_validate_volume(char volume_letter,int skip_removable);
int udefrag_get_volume_information(char volume_letter,volume_info *v);

int udefrag_bytes_to_hr(ULONGLONG bytes, int digits, char *buffer, int length);
ULONGLONG udefrag_hr_to_bytes(char *string);

typedef enum {
    ANALYSIS_JOB = 0,
    DEFRAGMENTATION_JOB,
    FULL_OPTIMIZATION_JOB,
    QUICK_OPTIMIZATION_JOB,
    MFT_OPTIMIZATION_JOB
} udefrag_job_type;

typedef enum {
    VOLUME_ANALYSIS = 0,     /* should be zero */
    VOLUME_DEFRAGMENTATION,
    VOLUME_OPTIMIZATION
} udefrag_operation_type;

/* flags triggering algorithm features */
#define UD_JOB_REPEAT                     0x1
/*
* 0x2, 0x4, 0x8 flags have been used 
* in the past for experimental options
*/
#define UD_JOB_CONTEXT_MENU_HANDLER       0x10

/*
* MFT_ZONE_SPACE has special meaning - 
* it is used as a marker for MFT Zone space.
*/
enum {
    UNUSED_MAP_SPACE = 0,        /* other colors have more precedence */
    FREE_SPACE,                  /* has lowest precedence */
    SYSTEM_SPACE,
    SYSTEM_OVER_LIMIT_SPACE,
    FRAGM_SPACE,
    FRAGM_OVER_LIMIT_SPACE,
    UNFRAGM_SPACE,
    UNFRAGM_OVER_LIMIT_SPACE,
    DIR_SPACE,
    DIR_OVER_LIMIT_SPACE,
    COMPRESSED_SPACE,
    COMPRESSED_OVER_LIMIT_SPACE,
    MFT_ZONE_SPACE,
    MFT_SPACE,                   /* has highest precedence */
    NUM_OF_SPACE_STATES          /* this must always be the last */
};

#define UNKNOWN_SPACE FRAGM_SPACE

typedef struct _udefrag_progress_info {
    unsigned long files;              /* number of files */
    unsigned long directories;        /* number of directories */
    unsigned long compressed;         /* number of compressed files */
    unsigned long fragmented;         /* number of fragmented files */
    ULONGLONG fragments;              /* number of fragments */
    ULONGLONG bad_fragments;          /* number of fragments which need to be joined together */
    double fragmentation;             /* fragmentation percentage */
    ULONGLONG total_space;            /* volume size, in bytes */
    ULONGLONG free_space;             /* free space amount, in bytes */
    ULONGLONG mft_size;               /* mft size, in bytes */
    udefrag_operation_type current_operation;  /* identifies currently running operation */
    unsigned long pass_number;        /* the current disk processing pass, increases 
                                         immediately after the pass completion */
    ULONGLONG clusters_to_process;    /* number of clusters to process */
    ULONGLONG processed_clusters;     /* number of already processed clusters */
    double percentage;                /* job completion percentage */
    int completion_status;            /* zero for running job, positive value for succeeded, negative for failed */
    char *cluster_map;                /* pointer to the cluster map buffer */
    int cluster_map_size;             /* size of the cluster map buffer, in bytes */
    ULONGLONG moved_clusters;         /* number of moved clusters */
    ULONGLONG total_moves;            /* number of moves by move_files_to_front/back functions */
} udefrag_progress_info;

typedef void  (*udefrag_progress_callback)(udefrag_progress_info *pi, void *p);
typedef int   (*udefrag_terminator)(void *p);

int udefrag_start_job(char volume_letter,udefrag_job_type job_type,int flags,
    int cluster_map_size,udefrag_progress_callback cb,udefrag_terminator t,void *p);

char *udefrag_get_default_formatted_results(udefrag_progress_info *pi);
void udefrag_release_default_formatted_results(char *results);

char *udefrag_get_error_description(int error_code);

/* reliable _toupper and _tolower analogs */
char udefrag_toupper(char c);
char udefrag_tolower(char c);
 /* reliable towupper and towlower analogs */
wchar_t udefrag_towupper(wchar_t c);
wchar_t udefrag_towlower(wchar_t c);
/* reliable _wcsupr and _wcslwr analogs */
wchar_t *udefrag_wcsupr(wchar_t *s);
wchar_t *udefrag_wcslwr(wchar_t *s);
/* reliable _wcsicmp analog */
int udefrag_wcsicmp(const wchar_t *s1, const wchar_t *s2);

int udefrag_set_log_file_path(void);
void udefrag_flush_dbg_log(void);

/**
 * @brief Delivers a message to the Debug View
 * program and appends it to the log file as well.
 * @param[in] flags one of the flags defined in
 * ../../include/dbg.h file.
 * If NT_STATUS_FLAG is set, the
 * last nt status value will be appended
 * to the debugging message as well as its
 * description. If LAST_ERROR_FLAG is set,
 * the same stuff will be done for the last
 * error value.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 * @note
 * - Not all system API set last status code.
 * Use strace macro defined in ../zenwinx/zenwinx.h
 * file to catch the status for sure.
 * - A few prefixes are defined for the debugging
 * messages. They're listed in ../../include/dbg.h
 * file and are intended for easier analysis of logs. To keep
 * logs clean always use one of those prefixes.
 */
void udefrag_dbg_print(int flags,const char *format, ...);

int udefrag_bootex_check(const wchar_t *command);
int udefrag_bootex_register(const wchar_t *command);
int udefrag_bootex_unregister(const wchar_t *command);

#if defined(__cplusplus)
}
#endif

#endif /* _UDEFRAG_H_ */
