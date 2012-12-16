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

/**
 * @file analyze.c
 * @brief Volume analysis.
 * @addtogroup Analysis
 * @{
 */

/*
* Ideas by Dmitri Arkhangelski <dmitriar@gmail.com>
* and Stefan Pendl <stefanpe@users.sourceforge.net>.
*/

#include "udefrag-internals.h"

static void update_progress_counters(winx_file_info *f,udefrag_job_parameters *jp);

/**
 * @internal
 * @brief Auxiliary structure
 * used by get_volume_information.
 */
struct fs {
    char *name; /* name in uppercase */
    file_system_type type;
    /* 
    * On fat the first clusters 
    * of directories cannot be moved.
    */
    int is_fat;
};

struct fs fs_types[] = {
    {"NTFS",  FS_NTFS,  0},
    {"FAT12", FS_FAT12, 1},
    {"FAT",   FS_FAT16, 1}, /* no need to distinguish better */
    {"FAT16", FS_FAT16, 1},
    {"FAT32", FS_FAT32, 1},
    {"EXFAT", FS_EXFAT, 1},
    {"UDF",   FS_UDF,   0},
    {NULL,    0,        0}
};

/**
 * @internal
 * @brief Constant definitions for
 * adjust_move_at_once_parameter
 * routine.
 */
#define _256K                           (256LL * 1024LL)
#define _4M                      (4LL * 1024LL * 1024LL)
#define _8M                      (8LL * 1024LL * 1024LL)
#define _16M                    (16LL * 1024LL * 1024LL)
#define _32M                    (32LL * 1024LL * 1024LL)
#define _64M                    (64LL * 1024LL * 1024LL)
#define _20G           (20LL * 1024LL * 1024LL * 1024LL)
#define _100G         (100LL * 1024LL * 1024LL * 1024LL)
#define _250G         (250LL * 1024LL * 1024LL * 1024LL)
#define _1T          (1024LL * 1024LL * 1024LL * 1024LL)
#define _2T    (2LL * 1024LL * 1024LL * 1024LL * 1024LL)

/**
 * @brief Defines how many clusters to move at once in the move_file routine.
 * @details This algorithm has been suggested by Joachim Otahal:
 * http://sourceforge.net/projects/ultradefrag/forums/forum/709672/topic/4779581
 */
void adjust_move_at_once_parameter(udefrag_job_parameters *jp)
{
    ULONGLONG bytes_at_once;
    char buffer[32];
    
    /* comply with "one half second to stop defragmentation" rule */
    if(jp->v_info.device_capacity < _20G){
        bytes_at_once = _256K;
    } else if(jp->v_info.device_capacity < _100G){
        bytes_at_once = _4M;
    } else if(jp->v_info.device_capacity < _250G){
        bytes_at_once = _8M;
    } else if(jp->v_info.device_capacity < _1T){
        bytes_at_once = _16M;
    } else if(jp->v_info.device_capacity < _2T){
        bytes_at_once = _32M;
    } else {
        bytes_at_once = _64M;
    }
    /* nt4 (and maybe w2k) is able to move no more than 256 KB at once */
    if(jp->win_version < WINDOWS_XP) bytes_at_once = _256K;
    jp->clusters_at_once = bytes_at_once / jp->v_info.bytes_per_cluster;
    if(jp->clusters_at_once == 0)
        jp->clusters_at_once ++;
    winx_bytes_to_hr(bytes_at_once,0,buffer,sizeof(buffer));
    trace(I"the program will move %s (%I64u clusters) at once",
        buffer, jp->clusters_at_once);
}

/**
 * @brief Retrieves all information about the volume.
 * @return Zero for success, negative value otherwise.
 * @note Resets statistics and cluster map.
 */
int get_volume_information(udefrag_job_parameters *jp)
{
    char fs_name[MAX_FS_NAME_LENGTH + 1];
    int i;
    
    /* reset mft zone disposition */
    memset(&jp->mft_zone,0,sizeof(struct _mft_zone));

    /* reset v_info structure */
    memset(&jp->v_info,0,sizeof(winx_volume_information));
    
    /* reset statistics */
    jp->pi.files = 0;
    jp->pi.directories = 0;
    jp->pi.compressed = 0;
    jp->pi.fragmented = 0;
    jp->pi.fragments = 0;
    jp->pi.total_space = 0;
    jp->pi.free_space = 0;
    jp->pi.mft_size = 0;
    jp->pi.clusters_to_process = 0;
    jp->pi.processed_clusters = 0;
    
    jp->fs_type = FS_UNKNOWN;
    jp->is_fat = 0;
    
    /* reset file lists */
    destroy_lists(jp);
    
    /* update global variables holding drive geometry */
    if(winx_get_volume_information(jp->volume_letter,&jp->v_info) < 0)
        return (-1);
    
    /* don't touch dirty volumes */
    if(jp->v_info.is_dirty)
        return UDEFRAG_DIRTY_VOLUME;

    jp->pi.total_space = jp->v_info.total_bytes;
    jp->pi.free_space = jp->v_info.free_bytes;
    trace(I"total clusters: %I64u",jp->v_info.total_clusters);
    trace(I"cluster size: %I64u",jp->v_info.bytes_per_cluster);
    /* validate geometry */
    if(!jp->v_info.total_clusters || !jp->v_info.bytes_per_cluster){
        trace(E"wrong volume geometry detected");
        return (-1);
    }
    adjust_move_at_once_parameter(jp);
    /* check partition type */
    trace(I"%s partition detected",jp->v_info.fs_name);
    strncpy(fs_name,jp->v_info.fs_name,MAX_FS_NAME_LENGTH);
    fs_name[MAX_FS_NAME_LENGTH] = 0;
    _strupr(fs_name);
    for(i = 0; fs_types[i].name; i++){
        if(!strcmp(fs_name,fs_types[i].name)){
            jp->fs_type = fs_types[i].type;
            jp->is_fat = fs_types[i].is_fat;
            break;
        }
    }
    if(jp->fs_type == FS_UNKNOWN){
        trace(E"file system type is not recognized");
        trace(E"type independent routines will be used to defragment it");
    }
    
    jp->pi.clusters_to_process = jp->v_info.total_clusters;
    jp->pi.processed_clusters = 0;
    
    if(jp->udo.fragment_size_threshold){
        if(jp->udo.fragment_size_threshold <= jp->v_info.bytes_per_cluster){
            trace(I"fragment size threshold is below the cluster size, so it will be ignored");
            jp->udo.fragment_size_threshold = 0;
        }
    }

    /* reset cluster map */
    reset_cluster_map(jp);
    return 0;
}

/**
 * @brief get_free_space_layout helper.
 */
static int process_free_region(winx_volume_region *rgn,void *user_defined_data)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;
    
    if(jp->udo.dbgprint_level >= DBG_PARANOID)
        trace(I"Free block start: %I64u len: %I64u",rgn->lcn,rgn->length);
    colorize_map_region(jp,rgn->lcn,rgn->length,FREE_SPACE,SYSTEM_SPACE);
    jp->pi.processed_clusters += rgn->length;
    jp->free_regions_count ++;
    return 0; /* continue scan */
}

/**
 * @brief Retrieves free space layout.
 * @return Zero for success, negative value otherwise.
 */
static int get_free_space_layout(udefrag_job_parameters *jp)
{
    char buffer[32];

    jp->free_regions = winx_get_free_volume_regions(jp->volume_letter,
        WINX_GVR_ALLOW_PARTIAL_SCAN,process_free_region,(void *)jp);
    
    winx_bytes_to_hr(jp->v_info.free_bytes,1,buffer,sizeof(buffer));
    trace(I"free space amount : %s",buffer);
    trace(I"free regions count: %u",jp->free_regions_count);
    
    /* let full disks to pass the analysis successfully */
    if(jp->free_regions == NULL || jp->free_regions_count == 0)
        etrace("disk is full or some error has been encountered");
    return 0;
}

/**
 * @brief Checks whether specified 
 * region is inside processed volume.
 */
int check_region(udefrag_job_parameters *jp,ULONGLONG lcn,ULONGLONG length)
{
    if(lcn < jp->v_info.total_clusters \
      && (lcn + length) <= jp->v_info.total_clusters)
        return 1;
    
    return 0;
}

/**
 * @brief Retrieves mft zones layout.
 * @note Since we have MFT optimization routine, 
 * let's use MFT zone for files placement on XP
 * and more recent Windows editions.
 */
static void get_mft_zones_layout(udefrag_job_parameters *jp)
{
    ULONGLONG start,length,mirror_size;

    if(jp->fs_type != FS_NTFS) return;
    
    /* 
    * Don't increment progress counters,
    * because mft zones are partially
    * inside already counted free space pool.
    */
    trace(I"%-12s: %-20s: %-20s", "mft section", "start", "length");

    /* $MFT */
    start = jp->v_info.ntfs_data.MftStartLcn.QuadPart;
    if(jp->v_info.ntfs_data.BytesPerCluster)
        length = jp->v_info.ntfs_data.MftValidDataLength.QuadPart / jp->v_info.ntfs_data.BytesPerCluster;
    else
        length = 0;
    trace(I"%-12s: %-20I64u: %-20I64u", "mft", start, length);
    jp->pi.mft_size = length * jp->v_info.bytes_per_cluster;
    trace(I"mft size = %I64u bytes", jp->pi.mft_size);

    /* MFT Zone */
    start = jp->v_info.ntfs_data.MftZoneStart.QuadPart;
    length = jp->v_info.ntfs_data.MftZoneEnd.QuadPart - jp->v_info.ntfs_data.MftZoneStart.QuadPart + 1;
    trace(I"%-12s: %-20I64u: %-20I64u", "mft zone", start, length);
    if(check_region(jp,start,length)){
        /* remark space as MFT Zone */
        colorize_map_region(jp,start,length,MFT_ZONE_SPACE,0);
        if(jp->win_version < WINDOWS_XP)
            jp->free_regions = winx_sub_volume_region(jp->free_regions,start,length);
        jp->mft_zone.start = start; jp->mft_zone.length = length;
    }

    /* $MFT Mirror */
    start = jp->v_info.ntfs_data.Mft2StartLcn.QuadPart;
    length = 1;
    mirror_size = jp->v_info.ntfs_data.BytesPerFileRecordSegment * 4;
    if(jp->v_info.ntfs_data.BytesPerCluster && mirror_size > jp->v_info.ntfs_data.BytesPerCluster){
        length = mirror_size / jp->v_info.ntfs_data.BytesPerCluster;
        if(mirror_size - length * jp->v_info.ntfs_data.BytesPerCluster)
            length ++;
    }
    trace(I"%-12s: %-20I64u: %-20I64u", "mft mirror", start, length);
}

/**
 * @brief Defines whether the file must be
 * excluded from the volume processing or not
 * because of its fragments size.
 * @return Nonzero value indicates
 * that the file must be excluded.
 */
int exclude_by_fragment_size(winx_file_info *f,udefrag_job_parameters *jp)
{
    winx_blockmap *block;
    ULONGLONG fragment_size = 0;
    
    if(jp->udo.fragment_size_threshold == DEFAULT_FRAGMENT_SIZE_THRESHOLD) return 0;
    /* don't filter out files if threshold is set by algorithm */
    if(jp->udo.algorithm_defined_fst) return 0;
    
    if(f->disp.blockmap == NULL) return 0;
    
    for(block = f->disp.blockmap; block; block = block->next){
        if(block == f->disp.blockmap){
            fragment_size += block->length;
        } else if(block->lcn == block->prev->lcn + block->prev->length){
            fragment_size += block->length;
        } else {
            if(fragment_size){
                if(fragment_size * jp->v_info.bytes_per_cluster < jp->udo.fragment_size_threshold)
                    return 0; /* file contains little fragments */
            }
            fragment_size = block->length;
        }
        if(block->next == f->disp.blockmap) break;
    }
    
    if(fragment_size){
        if(fragment_size * jp->v_info.bytes_per_cluster < jp->udo.fragment_size_threshold)
            return 0; /* file contains little fragments */
    }

    return 1;
}

/**
 * @brief Defines whether the file must be
 * excluded from the volume processing or not
 * because of its number of fragments.
 * @return Nonzero value indicates
 * that the file must be excluded.
 */
int exclude_by_fragments(winx_file_info *f,udefrag_job_parameters *jp)
{
    if(jp->udo.fragments_limit == 0) return 0;
    return (f->disp.fragments < jp->udo.fragments_limit) ? 1 : 0;
}

/**
 * @brief Defines whether the file must be
 * excluded from the volume processing or not
 * because of its size.
 * @return Nonzero value indicates
 * that the file must be excluded.
 */
int exclude_by_size(winx_file_info *f,udefrag_job_parameters *jp)
{
    ULONGLONG filesize;
    
    f->user_defined_flags &= ~UD_FILE_OVER_LIMIT;
    filesize = f->disp.clusters * jp->v_info.bytes_per_cluster;
    if(filesize > jp->udo.size_limit){
        f->user_defined_flags |= UD_FILE_OVER_LIMIT;
        return 1;
    }
    return 0;
}

/**
 * @brief Defines whether the file must be
 * excluded from the volume processing or not
 * because of its path.
 * @return Nonzero value indicates
 * that the file must be excluded.
 */
int exclude_by_path(winx_file_info *f,udefrag_job_parameters *jp)
{
    /* note that paths have \??\ internal prefix while patterns haven't */
    if(wcslen(f->path) < 0x4)
        return 1; /* path is invalid */
    
    if(jp->udo.ex_filter.count){
        if(winx_patcmp(f->path + 0x4,&jp->udo.ex_filter))
            return 1;
    }
    
    if(jp->udo.cut_filter.count){
        if(!winx_patcmp(f->path + 0x4,&jp->udo.cut_filter))
            return 1;
    }

    if(jp->udo.in_filter.count == 0) return 0;
    return !winx_patcmp(f->path + 0x4,&jp->udo.in_filter);
}

/**
 * @brief find_files helper.
 * @note Optimized for speed.
 */
static int filter(winx_file_info *f,void *user_defined_data)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;
    int length;
    
    /* START OF AUX CODE */
    
    /* skip entries with empty path, as well as their children */
    if(f->path == NULL) goto skip_file_and_children;
    if(f->path[0] == 0) goto skip_file_and_children;
    
    /*
    * Skip resident streams, but not in context menu
    * handler, where they're needed for statistics.
    */
    if(f->disp.fragments == 0 && !(jp->udo.job_flags & UD_JOB_CONTEXT_MENU_HANDLER))
        return 0;
    
    /*
    * Remove trailing dot from the root
    * directory path, otherwise we'll not
    * be able to defragment it.
    */
    length = wcslen(f->path);
    if(length >= 2){
        if(f->path[length - 1] == '.' && f->path[length - 2] == '\\'){
            trace(I"root directory detected, its trailing dot will be removed");
            f->path[length - 1] = 0;
        }
    }
    
    /* skip resident streams in context menu handler, but count them */
    if(f->disp.fragments == 0){
        if(!exclude_by_path(f,jp))
            update_progress_counters(f,jp);
        return 0;
    }
    
    /* show debugging information about interesting cases */
    if(is_sparse(f))
        trace(D"sparse file found: %ws",f->path);
    if(is_reparse_point(f))
        trace(D"reparse point found: %ws",f->path);
    /* comment it out after testing to speed things up */
    /*if(winx_wcsistr(f->path,L"$BITMAP"))
        trace(D"bitmap found: %ws",f->path);
    if(winx_wcsistr(f->path,L"$ATTRIBUTE_LIST"))
        trace(D"attribute list found: %ws",f->path);
    */
    
    /* START OF FILTERING */
    
    /* skip files with invalid map */
    if(f->disp.blockmap == NULL)
        goto skip_file;

    /* skip temporary files */
    if(is_temporary(f))
        goto skip_file;

    /* filter files by their sizes */
    if(exclude_by_size(f,jp))
        goto skip_file;

    /* filter files by their number of fragments */
    if(exclude_by_fragments(f,jp))
        goto skip_file;

    /* filter files by their fragment sizes */
    if(exclude_by_fragment_size(f,jp))
        goto skip_file;
    
    /* filter files by their paths */
    if(exclude_by_path(f,jp)){
        f->user_defined_flags |= UD_FILE_EXCLUDED;
        f->user_defined_flags |= UD_FILE_EXCLUDED_BY_PATH;
    }
    /* don't skip children since their paths may match patterns */
    return 0;

skip_file:
    f->user_defined_flags |= UD_FILE_EXCLUDED;
    return 0;

skip_file_and_children:
    f->user_defined_flags |= UD_FILE_EXCLUDED;
    return 1;
}

/**
 * @internal
 */
static void update_progress_counters(winx_file_info *f,udefrag_job_parameters *jp)
{
    ULONGLONG filesize;

    jp->pi.files ++;
    if(is_directory(f)) jp->pi.directories ++;
    if(is_compressed(f)) jp->pi.compressed ++;
    jp->pi.processed_clusters += f->disp.clusters;

    filesize = f->disp.clusters * jp->v_info.bytes_per_cluster;
    if(filesize >= GIANT_FILE_SIZE)
        jp->f_counters.giant_files ++;
    else if(filesize >= HUGE_FILE_SIZE)
        jp->f_counters.huge_files ++;
    else if(filesize >= BIG_FILE_SIZE)
        jp->f_counters.big_files ++;
    else if(filesize >= AVERAGE_FILE_SIZE)
        jp->f_counters.average_files ++;
    else if(filesize >= SMALL_FILE_SIZE)
        jp->f_counters.small_files ++;
    else
        jp->f_counters.tiny_files ++;
}

/**
 * @brief find_files helper.
 */
static void progress_callback(winx_file_info *f,void *user_defined_data)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;
    
    /* don't count excluded files in context menu handler */
    if(!(jp->udo.job_flags & UD_JOB_CONTEXT_MENU_HANDLER))
        update_progress_counters(f,jp);
}

/**
 * @brief find_files helper.
 */
static int terminator(void *user_defined_data)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;

    return jp->termination_router((void *)jp);
}

/**
 * @brief Displays file counters.
 */
void dbg_print_file_counters(udefrag_job_parameters *jp)
{
    trace(I"folders total:    %u",jp->pi.directories);
    trace(I"files total:      %u",jp->pi.files);
    trace(I"fragmented files: %u",jp->pi.fragmented);
    trace(I"compressed files: %u",jp->pi.compressed);
    trace(I"tiny ...... <  10 KB: %u",jp->f_counters.tiny_files);
    trace(I"small ..... < 100 KB: %u",jp->f_counters.small_files);
    trace(I"average ... <   1 MB: %u",jp->f_counters.average_files);
    trace(I"big ....... <  16 MB: %u",jp->f_counters.big_files);
    trace(I"huge ...... < 128 MB: %u",jp->f_counters.huge_files);
    trace(I"giant ..............: %u",jp->f_counters.giant_files);
}

/**
 * @brief Searches for all files on disk.
 * @return Zero for success, negative value otherwise.
 */
static int find_files(udefrag_job_parameters *jp)
{
    int context_menu_handler = 0;
    wchar_t parent_directory[MAX_PATH + 1];
    wchar_t *p;
    wchar_t c;
    int flags = 0;
    winx_file_info *f;
    winx_blockmap *block;
    
    /* check for context menu handler */
    if(jp->udo.job_flags & UD_JOB_CONTEXT_MENU_HANDLER){
        if(jp->udo.cut_filter.count > 0){
            if(wcslen(jp->udo.cut_filter.array[0]) >= wcslen(L"C:\\"))
                context_menu_handler = 1;
        }
    }

    /* speed up the context menu handler */
    if(jp->fs_type != FS_NTFS && context_menu_handler){
        /* in case of c:\* or c:\ scan entire disk */
        c = jp->udo.cut_filter.array[0][3];
        if(c == 0 || c == '*')
            goto scan_entire_disk;
        /* in case of c:\test;c:\test\* scan parent directory recursively */
        if(jp->udo.cut_filter.count > 1)
            flags = WINX_FTW_RECURSIVE;
        /* in case of c:\test scan parent directory, not recursively */
        _snwprintf(parent_directory, MAX_PATH, L"\\??\\%ws", jp->udo.cut_filter.array[0]);
        parent_directory[MAX_PATH] = 0;
        p = wcsrchr(parent_directory,'\\');
        if(p) *p = 0;
        if(wcslen(parent_directory) <= wcslen(L"\\??\\C:\\"))
            goto scan_entire_disk;
        jp->filelist = winx_ftw(parent_directory,
            flags | WINX_FTW_DUMP_FILES | \
            WINX_FTW_ALLOW_PARTIAL_SCAN | WINX_FTW_SKIP_RESIDENT_STREAMS,
            filter,progress_callback,terminator,(void *)jp);
    } else {
    scan_entire_disk:
        jp->filelist = winx_scan_disk(jp->volume_letter,
            WINX_FTW_DUMP_FILES | WINX_FTW_ALLOW_PARTIAL_SCAN | \
            WINX_FTW_SKIP_RESIDENT_STREAMS,
            filter,progress_callback,terminator,(void *)jp);
    }
    if(jp->filelist == NULL && !jp->termination_router((void *)jp))
        return (-1);
    
    /* calculate number of fragmented files; redraw map */
    for(f = jp->filelist; f; f = f->next){
        /* skip excluded files */
        if(!is_fragmented(f) || is_excluded(f)){
            jp->pi.fragments ++;
        } else {
            jp->pi.fragmented ++;
            jp->pi.fragments += f->disp.fragments;
        }

        /* count nonresident files included by context menu handler */
        if(jp->udo.job_flags & UD_JOB_CONTEXT_MENU_HANDLER){
            if(!is_excluded_by_path(f))
                update_progress_counters(f,jp);
        }
    
        /* redraw cluster map */
        colorize_file(jp,f,SYSTEM_SPACE);
        //trace(D"%ws",f->path);
        
        /* add file blocks to the binary search tree - after winx_scan_disk! */
        for(block = f->disp.blockmap; block; block = block->next){
            if(add_block_to_file_blocks_tree(jp,f,block) < 0) break;
            if(block->next == f->disp.blockmap) break;
        }

        if(f->next == jp->filelist) break;
    }

    dbg_print_file_counters(jp);
    return 0;
}

/**
 * @brief Defines whether the file
 * is locked by system or not.
 * @return Nonzero value indicates
 * that the file is locked.
 */
int is_file_locked(winx_file_info *f,udefrag_job_parameters *jp)
{
    NTSTATUS status;
    HANDLE hFile;
    int old_color;
    
    /* check whether the file has been passed the check already */
    if(f->user_defined_flags & UD_FILE_NOT_LOCKED)
        return 0;
    if(f->user_defined_flags & UD_FILE_LOCKED)
        return 1;

    /* file status is undefined, so let's try to open it */
    status = winx_defrag_fopen(f,WINX_OPEN_FOR_MOVE,&hFile);
    if(status == STATUS_SUCCESS){
        winx_defrag_fclose(hFile);
        f->user_defined_flags |= UD_FILE_NOT_LOCKED;
        return 0;
    }

    /*strace(status,"cannot open %ws",f->path);*/
    /* redraw space */
    old_color = get_file_color(jp,f);
    f->user_defined_flags |= UD_FILE_LOCKED;
    colorize_file(jp,f,old_color);
    return 1;
}

/**
 * @brief Defines whether the file is from
 * well known locked files list or not.
 * @note Optimized for speed.
 */
static int is_well_known_locked_file(winx_file_info *f,udefrag_job_parameters *jp)
{
    /* these files are usually locked on Windows XP */
    wchar_t *locked_files[] = {
        L"$Bitmap",
        L"$Extend\\$ObjId",
        L"$Extend\\$UsnJrnl",
        L"$LogFile",
        L"$MFT::$BITMAP",
        L"$Secure",
        NULL
    };
    int i, length = wcslen(f->path);
    
    /* search for well known locked NTFS meta files */
    if(length >= 9){ /* ensure that we have at least \??\X:\$x */
        if(f->path[7] == '$'){
            for(i = 0; locked_files[i]; i++){
                if(winx_wcsistr(f->path,locked_files[i]))
                    return 1;
            }
        }
    }

    /* check for paging and hibernation files */
    if(winx_wcsistr(f->name,L"pagefile.sys"))
        return 1;
    if(winx_wcsistr(f->name,L"hiberfil.sys"))
        return 1;
    return 0;
}

/**
 * @brief Searches for well known locked files
 * and applies their dispositions to the map.
 * @details Resets f->disp structure of locked files.
 */
static void redraw_well_known_locked_files(udefrag_job_parameters *jp)
{
    winx_file_info *f;
    ULONGLONG time;
    ULONGLONG n = 0;

    winx_dbg_print_header(0,0,I"search for well known locked files...");
    time = winx_xtime();
    
    for(f = jp->filelist; f; f = f->next){
        if(f->disp.blockmap){ /* otherwise nothing to redraw */
            if(is_well_known_locked_file(f,jp)){
                if(!is_file_locked(f,jp)){
                    /* possibility of this case should be reduced */
                    trace(I"false detection: %ws",f->path);
                } else {
                    trace(I"true detection:  %ws",f->path);
                    n ++;
                }
            }
        }
        if(f->next == jp->filelist) break;
    }

    trace(I"%I64u locked files found",n);
    winx_dbg_print_header(0,0,I"well known locked files search completed in %I64u ms",
        winx_xtime() - time);
}

/**
 * @brief Adds file to the list of fragmented files.
 */
int expand_fragmented_files_list(winx_file_info *f,udefrag_job_parameters *jp)
{
    udefrag_fragmented_file *ff, *ffprev = NULL;
    
    /* don't include filtered out files, for better performance */
    if(is_excluded(f)) return 0;

    for(ff = jp->fragmented_files; ff; ff = ff->next){
        if(ff->f->disp.fragments <= f->disp.fragments){
            if(ff != jp->fragmented_files)
                ffprev = ff->prev;
            break;
        }
        if(ff->next == jp->fragmented_files){
            ffprev = ff;
            break;
        }
    }
    
    ff = (udefrag_fragmented_file *)winx_list_insert((list_entry **)(void *)&jp->fragmented_files,
            (list_entry *)ffprev,sizeof(udefrag_fragmented_file));
    if(ff == NULL)
        return UDEFRAG_NO_MEM;

    ff->f = f;
    return 0;
}

/**
 * @brief Removes file from the list of fragmented files.
 */
void truncate_fragmented_files_list(winx_file_info *f,udefrag_job_parameters *jp)
{
    udefrag_fragmented_file *ff;
    
    for(ff = jp->fragmented_files; ff; ff = ff->next){
        if(ff->f == f){
            winx_list_remove((list_entry **)(void *)&jp->fragmented_files,(list_entry *)ff);
            break;
        }
        if(ff->next == jp->fragmented_files) break;
    }
}

/**
 * @brief Produces list of fragmented files.
 */
static void produce_list_of_fragmented_files(udefrag_job_parameters *jp)
{
    winx_file_info *f;
    ULONGLONG bad_fragments = 0;
    
    for(f = jp->filelist; f; f = f->next){
        if(is_fragmented(f) && !is_excluded(f)){
            expand_fragmented_files_list(f,jp);
            /* more precise calculation seems to be too slow */
            bad_fragments += f->disp.fragments;
        }
        if(f->next == jp->filelist) break;
    }
    jp->pi.bad_fragments = bad_fragments;
}

/**
 * @brief Check whether the requested
 * action is allowed or not.
 * @return Zero indicates that the
 * requested operation is allowed,
 * negative value indicates contrary.
 */
static int check_requested_action(udefrag_job_parameters *jp)
{
    /*
    * NTFS volumes with cluster size greater than 4 KB
    * cannot be defragmented on Windows 2000 and Windows NT 4.0.
    * This is a well known limitation of Windows Defrag API.
    */
    if(jp->job_type != ANALYSIS_JOB \
      && jp->fs_type == FS_NTFS \
      && jp->v_info.bytes_per_cluster > 4096 \
      && jp->win_version <= WINDOWS_2K){
        trace(E"cannot defragment NTFS volumes with clusters bigger than 4KB on nt4/w2k");
        return UDEFRAG_W2K_4KB_CLUSTERS;
    }

    if(jp->job_type != ANALYSIS_JOB && jp->fs_type == FS_UDF){
        trace(E"cannot defragment/optimize UDF volumes,");
        trace(E"because the file system driver does not support FSCTL_MOVE_FILE");
        return UDEFRAG_UDF_DEFRAG;
    }

    if(jp->is_fat){
        itrace("FAT directories cannot be moved %s",
            jp->win_version > WINDOWS_2K ? "entirely" : "at all");
    }
    if(jp->fs_type == FS_NTFS && jp->win_version < WINDOWS_XP){
        itrace("MFT is not movable");
        if(jp->win_version < WINDOWS_2K)
            itrace("directories are not movable");
    }
    return 0;
}

/**
 * @brief Defines whether the fragmentation level
 * is above the fragmentation threshold or not.
 */
int check_fragmentation_level(udefrag_job_parameters *jp)
{
    double x, y;
    unsigned int ifr, it;
    double fragmentation;
    
    x = (double)jp->pi.bad_fragments;
    y = (double)jp->pi.fragments;
    if(y == 0) fragmentation = 0.00;
    else fragmentation = (x / y) * 100.00;
    ifr = (unsigned int)(fragmentation * 100.00);
    it = (unsigned int)(jp->udo.fragmentation_threshold * 100.00);
    if(fragmentation < jp->udo.fragmentation_threshold){
        trace(I"fragmentation is below the threshold: %u.%02u%% < %u.%02u%%",
            ifr / 100, ifr % 100, it / 100, it % 100);
        return 0;
    }
    trace(I"fragmentation is above the threshold: %u.%02u%% >= %u.%02u%%",
        ifr / 100, ifr % 100, it / 100, it % 100);
    return 1;
}

/**
 * @brief Performs a volume analysis.
 * @return Zero for success, negative value otherwise.
 */
int analyze(udefrag_job_parameters *jp)
{
    ULONGLONG time;
    int result;
    
    time = start_timing("analysis",jp);
    jp->pi.current_operation = VOLUME_ANALYSIS;
    
    /* update volume information */
    result = get_volume_information(jp);
    if(result < 0)
        return result;
    
    /* scan volume for free space areas */
    if(get_free_space_layout(jp) < 0)
        return (-1);
    
    /* redraw mft zone in light magenta */
    get_mft_zones_layout(jp);
    
    /* search for files */
    if(find_files(jp) < 0)
        return (-1);
    
    /* redraw well known locked files in green */
    redraw_well_known_locked_files(jp);

    /* produce list of fragmented files */
    produce_list_of_fragmented_files(jp);
    (void)check_fragmentation_level(jp); /* for debugging */

    result = check_requested_action(jp);
    if(result < 0)
        return result;
    
    jp->p_counters.analysis_time = winx_xtime() - time;
    stop_timing("analysis",time,jp);
    return 0;
}

/** @} */
