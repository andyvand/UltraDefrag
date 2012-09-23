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
 * @file udefrag.c
 * @brief Entry point.
 * @details Each disk processing algorithm
 * should comply with the following rules:
 * -# never try to move directories on FAT entirely
 * -# never try to move MFT on NTFS entirely
 * -# work on disks with low amount of free space
 * -# sort primarily small files, because big files sorting is useless
 * -# save more time than needed to complete the disk processing itself
 * -# terminate quickly on already processed disks
 * -# don't sort out all the files when just a few files were changed
 *    since the last optimization
 * -# show progress percentage advancing from 0 to 100%
 * -# cleanup as much space as possible before use of the cleaned up space;
 *    otherwise NTFS processing will be slow
 * -# don't optimize any file twice
 * -# never go into infinite loop
 * -# handle correctly either normal or compressed/sparse files
 * -# handle correctly locked files
 * -# distinguish between file blocks and file fragments
 * -# handle a case when region assumed to be free becomes "already in use"
 * -# filter files properly
 * -# produce reports properly
 *
 * How the statistical data adjusts in all the volume processing routines:
 * -# we calculate the maximum amount of data which may be moved in process
 *    and assign this value to jp->pi.clusters_to_process counter
 * -# when we move something, we adjust jp->pi.processed_clusters
 * -# before each pass we adjust the jp->pi.clusters_to_process value
 *    to prevent the progress indicator overflow
 * @addtogroup Engine
 * @{
 */

/*
* Revised by Stefan Pendl, 2010, 2011
* <stefanpe@users.sourceforge.net>
*/

#include "udefrag-internals.h"

/**
 * @bug _ftol2 unresolved symbol error with WDK 7 for WinXP build
 */
#ifdef _MSC_VER
#if _MSC_VER >= 1400
#ifndef _WIN64
#ifndef STATIC_LIB
extern long _cdecl _ftol(double x);
long _cdecl _ftol2(double x){return _ftol(x);}
#endif
#endif
#endif
#endif

/**
 * @brief Defines whether udefrag library has 
 * been initialized successfully or not.
 * @return Boolean value.
 */
int udefrag_init_failed(void)
{
    return winx_init_failed();
}

/**
 */
static void dbg_print_header(udefrag_job_parameters *jp)
{
    int os_version;
    int mj, mn;
    winx_time t;
    char ch;
    wchar_t c;
    wchar_t s1[] = L"a";
    wchar_t s2[] = L"b";
    int result;

    /* print driver version */
    winx_dbg_print_header(0,0,"*");
    winx_dbg_print_header(0x20,0,"%s",VERSIONINTITLE);

    /* print windows version */
    os_version = winx_get_os_version();
    mj = os_version / 10;
    mn = os_version % 10;
    winx_dbg_print_header(0x20,0,"Windows NT %u.%u",mj,mn);
    
    /* print date and time */
    memset(&t,0,sizeof(winx_time));
    (void)winx_get_local_time(&t);
    winx_dbg_print_header(0x20,0,"[%02i.%02i.%04i at %02i:%02i]",
        (int)t.day,(int)t.month,(int)t.year,(int)t.hour,(int)t.minute);
    winx_dbg_print_header(0,0,"*");
    
    /* force MinGW to export general purpose udefrag_xxx routines */
    ch = 'a'; ch = winx_tolower(ch) - winx_toupper(ch);
    c = 'a'; c = winx_towlower(c) - winx_towupper(c);
    winx_wcslwr(s1); winx_wcsupr(s2);
    result = winx_wcsicmp(s1,s2);
    winx_dbg_print(NULL);
}

/**
 */
static void dbg_print_footer(udefrag_job_parameters *jp)
{
    winx_dbg_print_header(0,0,"*");
    winx_dbg_print_header(0,0,"Processing of %c: %s",
        jp->volume_letter, (jp->pi.completion_status > 0) ? "succeeded" : "failed");
    winx_dbg_print_header(0,0,"*");
}

/**
 */
static void dbg_print_single_counter(udefrag_job_parameters *jp,ULONGLONG counter,char *name)
{
    ULONGLONG time, seconds;
    double p;
    unsigned int ip;
    char buffer[32];
    char *s;

    time = counter;
    seconds = time / 1000;
    winx_time2str(seconds,buffer,sizeof(buffer));
    time -= seconds * 1000;
    
    if(jp->p_counters.overall_time == 0){
        p = 0.00;
    } else {
        /* conversion to LONGLONG is needed for Win DDK */
        p = (double)(LONGLONG)counter / (double)(LONGLONG)jp->p_counters.overall_time;
    }
    ip = (unsigned int)(p * 10000);
    s = winx_sprintf("%s %I64ums",buffer,time);
    if(s == NULL){
        DebugPrint(" - %s %-18s %6I64ums (%u.%02u %%)",name,buffer,time,ip / 100,ip % 100);
    } else {
        DebugPrint(" - %s %-25s (%u.%02u %%)",name,s,ip / 100,ip % 100);
        winx_heap_free(s);
    }
}

/**
 */
static void dbg_print_performance_counters(udefrag_job_parameters *jp)
{
    ULONGLONG time, seconds;
    char buffer[32];
    
    winx_dbg_print_header(0,0,"*");

    time = jp->p_counters.overall_time;
    seconds = time / 1000;
    winx_time2str(seconds,buffer,sizeof(buffer));
    time -= seconds * 1000;
    DebugPrint("volume processing completed in %s %I64ums:",buffer,time);
    dbg_print_single_counter(jp,jp->p_counters.analysis_time,             "analysis ...............");
    dbg_print_single_counter(jp,jp->p_counters.searching_time,            "searching ..............");
    dbg_print_single_counter(jp,jp->p_counters.moving_time,               "moving .................");
    dbg_print_single_counter(jp,jp->p_counters.temp_space_releasing_time, "releasing temp space ...");
}

/**
 * @brief Delivers progress information to the caller.
 * @note 
 * - completion_status parameter becomes delivered to the caller
 * instead of the appropriate field of jp->pi structure.
 * - If cluster map cell is occupied entirely by MFT zone
 * it will be drawn in light magenta if no files exist there.
 * Otherwise, such a cell will be drawn in different color
 * indicating that something still exists inside the zone.
 */
static void deliver_progress_info(udefrag_job_parameters *jp,int completion_status)
{
    udefrag_progress_info pi;
    ULONGLONG x, y;
    int i, k, index, p1, p2;
    int mft_zone_detected;
    int free_cell_detected;
    ULONGLONG maximum, n;
    
    if(jp->cb == NULL)
        return;

    /* make a copy of jp->pi */
    memcpy(&pi,&jp->pi,sizeof(udefrag_progress_info));
    
    /* replace completion status */
    pi.completion_status = completion_status;
    
    /* calculate progress percentage */
    /* conversion to LONGLONG is needed for Win DDK */
    /* so, let's divide both numbers to make safe conversion then */
    x = pi.processed_clusters / 2;
    y = pi.clusters_to_process / 2;
    if(y == 0) pi.percentage = 0.00;
    else pi.percentage = ((double)(LONGLONG)x / (double)(LONGLONG)y) * 100.00;
    
    /* calculate fragmentation percentage */
    /* conversion to LONGLONG is needed for Win DDK */
    /* so, let's divide both numbers to make safe conversion then */
    x = pi.bad_fragments / 2;
    y = pi.fragments / 2;
    if(y == 0) pi.fragmentation = 0.00;
    else pi.fragmentation = ((double)(LONGLONG)x / (double)(LONGLONG)y) * 100.00;
    
    /* refill cluster map */
    if(jp->pi.cluster_map && jp->cluster_map.array \
      && jp->pi.cluster_map_size == jp->cluster_map.map_size){
        for(i = 0; i < jp->cluster_map.map_size; i++){
            /* check for mft zone to apply special rules there */
            mft_zone_detected = free_cell_detected = 0;
            maximum = 1; /* for jp->cluster_map.opposite_order */
            if(!jp->cluster_map.opposite_order){
                if(i == jp->cluster_map.map_size - jp->cluster_map.unused_cells - 1)
                    maximum = jp->cluster_map.clusters_per_last_cell;
                else
                    maximum = jp->cluster_map.clusters_per_cell;
            }
            if(jp->cluster_map.array[i][MFT_ZONE_SPACE] >= maximum)
                mft_zone_detected = 1;
            if(jp->cluster_map.array[i][FREE_SPACE] >= maximum)
                free_cell_detected = 1;
            if(mft_zone_detected && free_cell_detected){
                jp->pi.cluster_map[i] = MFT_ZONE_SPACE;
            } else {
                maximum = jp->cluster_map.array[i][0];
                index = 0;
                for(k = 1; k < jp->cluster_map.n_colors; k++){
                    n = jp->cluster_map.array[i][k];
                    if(n >= maximum){ /* support of colors precedence  */
                        if((k != MFT_ZONE_SPACE && k != FREE_SPACE) || !mft_zone_detected){
                            maximum = n;
                            index = k;
                        }
                    }
                }
                if(maximum == 0)
                    jp->pi.cluster_map[i] = DEFAULT_COLOR;
                else
                    jp->pi.cluster_map[i] = (char)index;
            }
        }
    }
    
    /* deliver information to the caller */
    jp->cb(&pi,jp->p);
    jp->progress_refresh_time = winx_xtime();
    if(jp->udo.dbgprint_level >= DBG_PARANOID)
        winx_dbg_print_header(0x20,0,"progress update");
        
    if(jp->udo.dbgprint_level >= DBG_DETAILED){
        p1 = (int)(__int64)(pi.percentage * 100.00);
        p2 = p1 % 100;
        p1 = p1 / 100;
        
        if(p1 >= jp->progress_trigger){
            winx_dbg_print_header('>',0,"progress %3u.%02u%% completed, "
                "trigger %3u", p1, p2, jp->progress_trigger);
            jp->progress_trigger = (p1 / 10) * 10 + 10;
        }
    }
}

/**
 */
static int terminator(void *p)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)p;
    int result;

    /* ask caller */
    if(jp->t){
        result = jp->t(jp->p);
        if(result){
            winx_dbg_print_header(0,0,"*");
            winx_dbg_print_header(0x20,0,"termination requested by caller");
            winx_dbg_print_header(0,0,"*");
        }
        return result;
    }

    /* continue */
    return 0;
}

/**
 */
static int killer(void *p)
{
    winx_dbg_print_header(0,0,"*");
    winx_dbg_print_header(0x20,0,"termination requested by caller");
    winx_dbg_print_header(0,0,"*");
    return 1;
}

/**
 */
static DWORD WINAPI start_job(LPVOID p)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)p;
    char *action = "analyzing";
    int result = 0;

    /* check job flags */
    if(jp->udo.job_flags & UD_JOB_REPEAT)
        DebugPrint("repeat action until nothing left to move");
    
    /* do the job */
    if(jp->job_type == DEFRAGMENTATION_JOB) action = "defragmenting";
    else if(jp->job_type == FULL_OPTIMIZATION_JOB) action = "optimizing";
    else if(jp->job_type == QUICK_OPTIMIZATION_JOB) action = "quick optimizing";
    else if(jp->job_type == MFT_OPTIMIZATION_JOB) action = "optimizing $mft on";
    winx_dbg_print_header(0,0,"Start %s disk %c:",action,jp->volume_letter);
    remove_fragmentation_report(jp);
    (void)winx_vflush(jp->volume_letter); /* flush all file buffers */
    
    /* speedup file searching in optimization */
    if(jp->job_type == FULL_OPTIMIZATION_JOB \
      || jp->job_type == QUICK_OPTIMIZATION_JOB \
      || jp->job_type == MFT_OPTIMIZATION_JOB)
        create_file_blocks_tree(jp);

    switch(jp->job_type){
    case ANALYSIS_JOB:
        result = analyze(jp);
        break;
    case DEFRAGMENTATION_JOB:
        result = defragment(jp);
        break;
    case FULL_OPTIMIZATION_JOB:
    case QUICK_OPTIMIZATION_JOB:
        result = optimize(jp);
        break;
    case MFT_OPTIMIZATION_JOB:
        result = optimize_mft(jp);
        break;
    default:
        result = 0;
        break;
    }

    destroy_file_blocks_tree(jp);
    if(jp->job_type != ANALYSIS_JOB)
        release_temp_space_regions(jp);
    (void)save_fragmentation_report(jp);
    
    /* now it is safe to adjust the completion status */
    jp->pi.completion_status = result;
    if(jp->pi.completion_status == 0)
    jp->pi.completion_status ++; /* success */
    
    winx_exit_thread(0); /* 8k/12k memory leak here? */
    return 0;
}

/**
 * @brief Destroys list of free regions, 
 * list of files and list of fragmented files.
 */
void destroy_lists(udefrag_job_parameters *jp)
{
    winx_scan_disk_release(jp->filelist);
    winx_release_free_volume_regions(jp->free_regions);
    winx_list_destroy((list_entry **)(void *)&jp->fragmented_files);
    jp->filelist = NULL;
    jp->free_regions = NULL;
}

/**
 * @brief Starts disk analysis/defragmentation/optimization job.
 * @param[in] volume_letter the volume letter.
 * @param[in] job_type one of the xxx_JOB constants, defined in udefrag.h
 * @param[in] flags combination of UD_JOB_xxx and UD_PREVIEW_xxx flags defined in udefrag.h
 * @param[in] cluster_map_size size of the cluster map, in cells.
 * Zero value forces to avoid cluster map use.
 * @param[in] cb address of procedure to be called each time when
 * progress information updates, but no more frequently than
 * specified in UD_REFRESH_INTERVAL environment variable.
 * @param[in] t address of procedure to be called each time
 * when requested job would like to know whether it must be terminated or not.
 * Nonzero value, returned by terminator, forces the job to be terminated.
 * @param[in] p pointer to user defined data to be passed to both callbacks.
 * @return Zero for success, negative value otherwise.
 * @note Callback procedures should complete as quickly
 * as possible to avoid slowdown of the volume processing.
 */
int udefrag_start_job(char volume_letter,udefrag_job_type job_type,int flags,
        int cluster_map_size,udefrag_progress_callback cb,udefrag_terminator t,void *p)
{
    udefrag_job_parameters jp;
    ULONGLONG time = 0;
    int use_limit = 0;
    int result = -1;
    int win_version = winx_get_os_version();
    
    /* initialize the job */
    dbg_print_header(&jp);

    /* convert volume letter to uppercase - needed for w2k */
    volume_letter = winx_toupper(volume_letter);
    
    memset(&jp,0,sizeof(udefrag_job_parameters));
    jp.filelist = NULL;
    jp.fragmented_files = NULL;
    jp.free_regions = NULL;
    jp.progress_refresh_time = 0;
    
    jp.volume_letter = volume_letter;
    jp.job_type = job_type;
    jp.cb = cb;
    jp.t = t;
    jp.p = p;

    /*
    * We deliver the progress information from
    * the current thread as well as decide whether
    * to terminate the job or not here. This 
    * multi-threaded technique works quite smoothly.
    */
    jp.termination_router = terminator;

    jp.start_time = jp.p_counters.overall_time = winx_xtime();
    jp.pi.completion_status = 0;
    
    if(get_options(&jp) < 0)
        goto done;
    
    jp.udo.job_flags = flags;

    if(allocate_map(cluster_map_size,&jp) < 0){
        release_options(&jp);
        goto done;
    }
    
    /* set additional privileges for Vista and above */
    if(win_version >= WINDOWS_VISTA){
        (void)winx_enable_privilege(SE_BACKUP_PRIVILEGE);
        
        if(win_version >= WINDOWS_7)
            (void)winx_enable_privilege(SE_MANAGE_VOLUME_PRIVILEGE);
    }
    
    /* run the job in separate thread */
    if(winx_create_thread(start_job,(PVOID)&jp,NULL) < 0){
        free_map(&jp);
        release_options(&jp);
        goto done;
    }

    /*
    * Call specified callback every refresh_interval milliseconds.
    * http://sourceforge.net/tracker/index.php?func=
    * detail&aid=2886353&group_id=199532&atid=969873
    */
    if(jp.udo.time_limit){
        time = jp.udo.time_limit * 1000;
        if(time / 1000 == jp.udo.time_limit){
            /* no overflow occured */
            use_limit = 1;
        } else {
            /* Windows will die sooner */
        }
    }
    do {
        winx_sleep(jp.udo.refresh_interval);
        deliver_progress_info(&jp,0); /* status = running */
        if(use_limit){
            if(time <= jp.udo.refresh_interval){
                /* time limit exceeded */
                winx_dbg_print_header(0,0,"*");
                winx_dbg_print_header(0x20,0,"time limit exceeded");
                winx_dbg_print_header(0,0,"*");
                jp.termination_router = killer;
            } else {
                if(jp.start_time){
                    if(winx_xtime() - jp.start_time > jp.udo.time_limit * 1000)
                        time = 0;
                } else {
                    /* this method gives not so fine results, but requires no winx_xtime calls */
                    time -= jp.udo.refresh_interval; 
                }
            }
        }
    } while(jp.pi.completion_status == 0);

    /* cleanup */
    deliver_progress_info(&jp,jp.pi.completion_status);
    destroy_lists(&jp);
    free_map(&jp);
    release_options(&jp);
    
done:
    jp.p_counters.overall_time = winx_xtime() - jp.p_counters.overall_time;
    dbg_print_performance_counters(&jp);
    dbg_print_footer(&jp);
    if(jp.pi.completion_status > 0)
        result = 0;
    else if(jp.pi.completion_status < 0)
        result = jp.pi.completion_status;
    return result;
}

/**
 * @brief Retrieves the default formatted results 
 * of the completed disk defragmentation job.
 * @param[in] pi pointer to udefrag_progress_info structure.
 * @return A string containing default formatted results
 * of the disk defragmentation job. NULL indicates failure.
 * @note This function is used in console and native applications.
 */
char *udefrag_get_default_formatted_results(udefrag_progress_info *pi)
{
    #define MSG_LENGTH 4095
    char *msg;
    char total_space[68];
    char free_space[68];
    double p;
    unsigned int ip;
    
    /* allocate memory */
    msg = winx_heap_alloc(MSG_LENGTH + 1);
    if(msg == NULL)
        return msg;

    (void)winx_bytes_to_hr(pi->total_space,2,total_space,sizeof(total_space));
    (void)winx_bytes_to_hr(pi->free_space,2,free_space,sizeof(free_space));

    if(pi->files == 0){
        p = 0.00;
    } else {
        /* conversion to LONGLONG is needed for Win DDK */
        p = (double)(LONGLONG)pi->fragments / (double)(LONGLONG)pi->files;
    }
    ip = (unsigned int)(p * 100.00);
    if(ip < 100)
        ip = 100; /* fix round off error */

    (void)_snprintf(msg,MSG_LENGTH,
              "Drive information:\n\n"
              "  Total space                  = %s\n"
              "  Free space                   = %s\n\n"
              "  Total number of files        = %u\n"
              "  Number of fragmented files   = %u\n"
              "  Fragments per file           = %u.%02u\n\n",
              total_space,
              free_space,
              pi->files,
              pi->fragmented,
              ip / 100, ip % 100
             );
    msg[MSG_LENGTH] = 0;
    return msg;
}

/**
 * @brief Releases memory allocated
 * by udefrag_get_default_formatted_results.
 * @param[in] results the string to be released.
 */
void udefrag_release_default_formatted_results(char *results)
{
    if(results)
        winx_heap_free(results);
}

/**
 * @brief Retrieves a human readable error
 * description for ultradefrag error codes.
 * @param[in] error_code the error code.
 * @return A human readable description.
 */
char *udefrag_get_error_description(int error_code)
{
    switch(error_code){
    case UDEFRAG_UNKNOWN_ERROR:
        return "Some unknown internal bug or some\n"
               "rarely arising error has been encountered.";
    case UDEFRAG_NO_MEM:
        return "Not enough memory.";
    case UDEFRAG_CDROM:
        return "It is impossible to defragment CDROM drives.";
    case UDEFRAG_REMOTE:
        return "It is impossible to defragment remote disks.";
    case UDEFRAG_ASSIGNED_BY_SUBST:
        return "It is impossible to defragment disks\n"
               "assigned by the \'subst\' command.";
    case UDEFRAG_REMOVABLE:
        return "You are trying to defragment a removable disk.\n"
               "If the disk type was wrongly identified, send\n"
               "a bug report to the author, thanks.";
    case UDEFRAG_UDF_DEFRAG:
        return "UDF disks can neither be defragmented nor optimized,\n"
               "because the file system driver does not support FSCTL_MOVE_FILE.";
    case UDEFRAG_DIRTY_VOLUME:
        return "Disk is dirty, run CHKDSK to repair it.";
    }
    return "";
}

/**
 * @internal
 * @brief Writes a header to the log file.
 */
static void write_log_file_header(char *path)
{
    WINX_FILE *f;
    char header[] = "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"
                    "\r\n"
                    "                           UltraDefrag log file\r\n"
                    "\r\n"
                    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"
                    "\r\n"
                    "                       If you'd like to report a bug,\r\n"
                    "                  attach this file to your bug report please:\r\n"
                    "          http://sourceforge.net/tracker/?group_id=199532&atid=969870\r\n"
                    "\r\n"
                    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"
                    "\r\n";
    
    f = winx_fopen(path,"a");
    if(f == NULL)
        return;
    
    winx_fwrite(header,1,sizeof(header) - 1,f);
    winx_fclose(f);
}

/**
 * @brief Enables debug logging to the file
 * if <b>\%UD_LOG_FILE_PATH\%</b> is set, otherwise
 * disables the logging.
 * @details If log path does not exist and
 * cannot be created, logs are placed in
 * <b>\%tmp\%\\UltraDefrag_Logs</b> folder.
 * @return Zero for success, negative value
 * otherwise.
 */
int udefrag_set_log_file_path(void)
{
    wchar_t *path, *buffer;
    char *native_path, *path_copy, *filename;
    int result;
    DWORD (__stdcall *func_GetLongPathNameW)(
        wchar_t *lpszShortPath,wchar_t *lpszLongPath,
        DWORD cchBuffer) = NULL;
    DWORD (__stdcall *func_GetFullPathNameW)(
        wchar_t *lpFileName,DWORD nBufferLength,
        wchar_t *lpBuffer,wchar_t **lpFilePart) = NULL;
    
    path = winx_heap_alloc(ENV_BUFFER_SIZE * sizeof(wchar_t));
    if(path == NULL)
        return UDEFRAG_NO_MEM;
    buffer = winx_heap_alloc(ENV_BUFFER_SIZE * sizeof(wchar_t));
    if(buffer == NULL){
        winx_heap_free(path);
        return UDEFRAG_NO_MEM;
    }
    
    result = winx_query_env_variable(L"UD_LOG_FILE_PATH",path,ENV_BUFFER_SIZE);
    if(result < 0 || path[0] == 0){
        /* empty variable forces to disable log */
        winx_disable_dbg_log();
        winx_heap_free(path);
        winx_heap_free(buffer);
        return 0;
    }
    
    /* convert to the full path whenever possible */
    winx_get_proc_address(L"kernel32.dll",
        "GetLongPathNameW",
        (void *)&func_GetLongPathNameW);
    winx_get_proc_address(L"kernel32.dll",
        "GetFullPathNameW",
        (void *)&func_GetFullPathNameW);
    if(func_GetLongPathNameW){
        result = func_GetLongPathNameW(path,buffer,ENV_BUFFER_SIZE);
        if(result == 0){
            DebugPrint("udefrag_set_log_file_path: GetLongPathNameW failed");
        } else if(result > ENV_BUFFER_SIZE){
            DebugPrint("udefrag_set_log_file_path: path %ws is too long",path);
        } else {
            buffer[ENV_BUFFER_SIZE - 1] = 0;
            wcscpy(path,buffer);
        }
    }
    if(func_GetFullPathNameW){
        result = func_GetFullPathNameW(path,ENV_BUFFER_SIZE,buffer,NULL);
        if(result == 0){
            DebugPrint("udefrag_set_log_file_path: GetFullPathNameW failed");
        } else if(result > ENV_BUFFER_SIZE){
            DebugPrint("udefrag_set_log_file_path: path %ws is too long",path);
        } else {
            buffer[ENV_BUFFER_SIZE - 1] = 0;
            wcscpy(path,buffer);
        }
    }
    
    /* convert to native path */
    native_path = winx_sprintf("\\??\\%ws",path);
    if(native_path == NULL){
        DebugPrint("udefrag_set_log_file_path: cannot build native path");
        winx_heap_free(path);
        winx_heap_free(buffer);
        return (-1);
    }
    
    /* delete old logfile */
    winx_delete_file(native_path);
    
    /* ensure that target path exists */
    result = 0;
    path_copy = winx_strdup(native_path);
    if(path_copy == NULL){
        DebugPrint("udefrag_set_log_file_path: not enough memory");
    } else {
        winx_path_remove_filename(path_copy);
        result = winx_create_path(path_copy);
        winx_heap_free(path_copy);
    }
    
    /* if target path cannot be created, use %tmp%\UltraDefrag_Logs */
    if(result < 0){
        result = winx_query_env_variable(L"TMP",path,ENV_BUFFER_SIZE);
        if(result < 0 || path[0] == 0){
            DebugPrint("udefrag_set_log_file_path: failed to query %%TMP%%");
        } else {
            filename = winx_strdup(native_path);
            if(filename == NULL){
                DebugPrint("udefrag_set_log_file_path: cannot allocate memory for filename");
            } else {
                winx_path_extract_filename(filename);
                winx_heap_free(native_path);
                native_path = winx_sprintf("\\??\\%ws\\UltraDefrag_Logs\\%s",path,filename);
                if(native_path == NULL){
                    DebugPrint("udefrag_set_log_file_path: cannot build %%tmp%%\\UltraDefrag_Logs\\{filename}");
                } else {
                    /* delete old logfile from the temporary folder */
                    winx_delete_file(native_path);
                }
                winx_heap_free(filename);
            }
        }
    }
    
    if(native_path){
        /* write header to the log file */
        write_log_file_header(native_path);
        /* allow debugging output to be appended */
        winx_enable_dbg_log(native_path);
    }
    
    winx_heap_free(native_path);
    winx_heap_free(path);
    winx_heap_free(buffer);
    return 0;
}

/**
 * @brief Appends all collected debugging
 * information to the log file.
 */
void udefrag_flush_dbg_log(void)
{
    winx_flush_dbg_log();
}

#ifndef STATIC_LIB

HANDLE hMutex = NULL;

/**
 * @brief udefrag.dll entry point.
 */
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
    /*
    * Both command line and GUI clients support
    * UD_LOG_FILE_PATH environment variable
    * to control debug logging to the file.
    */
    if(dwReason == DLL_PROCESS_ATTACH){
        if(winx_get_os_version() >= WINDOWS_VISTA)
            (void)winx_create_mutex(L"\\Sessions\\1\\BaseNamedObjects\\ultradefrag_mutex",&hMutex);
        else
            (void)winx_create_mutex(L"\\BaseNamedObjects\\ultradefrag_mutex",&hMutex);
        (void)udefrag_set_log_file_path();
    } else if(dwReason == DLL_PROCESS_DETACH){
        winx_destroy_mutex(hMutex);
    }
    return 1;
}
#endif

/** @} */
