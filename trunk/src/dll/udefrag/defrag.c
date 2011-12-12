/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file defrag.c
 * @brief Volume defragmentation.
 * @addtogroup Defrag
 * @{
 */

/*
* Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
* and Dmitri Arkhangelski <dmitriar@gmail.com>.
*/

#include "udefrag-internals.h"

/* forward declarations */
static ULONGLONG rough_cc_routine(udefrag_job_parameters *jp);
static ULONGLONG fine_cc_routine(udefrag_job_parameters *jp);
static int rough_defrag_routine(udefrag_job_parameters *jp);
static int fine_defrag_routine(udefrag_job_parameters *jp);

/************************************************************/
/*                       Test suite                         */
/************************************************************/

/*
* Uncomment it to test defragmentation
* of various special files like reparse
* points, attribute lists and others.
*/
//#define TEST_SPECIAL_FILES_DEFRAG

/* Test suite for special files. */
#ifdef TEST_SPECIAL_FILES_DEFRAG
void test_move(winx_file_info *f,udefrag_job_parameters *jp)
{
    winx_volume_region *target_rgn;
    ULONGLONG source_lcn = f->disp.blockmap->lcn;
    
    /* try to move the first cluster to the last free region */
    target_rgn = find_last_free_region(jp,1);
    if(target_rgn == NULL){
        DebugPrint("test_move: no free region found on disk");
        return;
    }
    if(move_file(f,f->disp.blockmap->vcn,1,target_rgn->lcn,0,jp) < 0){
        DebugPrint("test_move: move failed for %ws",f->path);
        return;
    } else {
        DebugPrint("test_move: move succeeded for %ws",f->path);
    }
    /* release temporary allocated clusters */
    release_temp_space_regions(jp);
    /* try to move the first cluster back */
    if(can_move(f,jp)){
        if(move_file(f,f->disp.blockmap->vcn,1,source_lcn,0,jp) < 0){
            DebugPrint("test_move: move failed for %ws",f->path);
            return;
        } else {
            DebugPrint("test_move: move succeeded for %ws",f->path);
        }
    } else {
        DebugPrint("test_move: file became unmovable %ws",f->path);
    }
}

/*
* Tests defragmentation of reparse points,
* encrypted files, bitmaps and attribute lists.
*/
void test_special_files_defrag(udefrag_job_parameters *jp)
{
    winx_file_info *f;
    int special_file = 0;
    
    DebugPrint("test of special files defragmentation started");

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return;

    for(f = jp->filelist; f; f = f->next){
        if(can_move(f,jp)){
            special_file = 0;
            if(is_reparse_point(f)){
                DebugPrint("reparse point detected: %ws",f->path);
                special_file = 1;
            } else if(is_encrypted(f)){
                DebugPrint("encrypted file detected: %ws",f->path);
                special_file = 1;
            } else if(winx_wcsistr(f->path,L"$BITMAP")){
                DebugPrint("bitmap detected: %ws",f->path);
                special_file = 1;
            } else if(winx_wcsistr(f->path,L"$ATTRIBUTE_LIST")){
                DebugPrint("attribute list detected: %ws",f->path);
                special_file = 1;
            }
            if(special_file)
                test_move(f,jp);
        }
        if(f->next == jp->filelist) break;
    }
    
    winx_fclose(jp->fVolume);
    DebugPrint("test of special files defragmentation completed");
}
#endif /* TEST_SPECIAL_FILES_DEFRAG */

/************************************************************/
/*                    The entry point                       */
/************************************************************/

/**
 * @brief Performs a volume defragmentation.
 * @details To avoid infinite data moves in multipass
 * processing, we exclude files for which moving failed.
 * On the other hand, number of fragmented files instantly
 * decreases, so we'll never have infinite loops here.
 * @return Zero for success, negative value otherwise.
 */
int defragment(udefrag_job_parameters *jp)
{
    int result, overall_result = -1;
    disk_processing_routine defrag_routine = NULL;
    clusters_counting_routine cc_routine = NULL;
    int win_version;
    
    /* perform volume analysis */
    if(jp->job_type == DEFRAGMENTATION_JOB){
        result = analyze(jp); /* we need to call it once, here */
        if(result < 0) return result;
    }
    
#ifdef TEST_SPECIAL_FILES_DEFRAG
    if(jp->job_type == DEFRAGMENTATION_JOB){
        test_special_files_defrag(jp);
        return 0;
    }
#endif

    /* set working routines */
    win_version = winx_get_os_version();
    if(jp->udo.fragment_size_threshold == 0 \
      || win_version <= WINDOWS_2K){
        DebugPrint("defragment: rough routines will be used");
        if(win_version <= WINDOWS_2K)
            DebugPrint("defragment: because of nt4/w2k API limitations");
        else
            DebugPrint("defragment: because of fragment size threshold filter not set");
        defrag_routine = rough_defrag_routine;
        cc_routine = rough_cc_routine;
    } else {
        DebugPrint("defragment: fine routines will be used");
        defrag_routine = fine_defrag_routine;
        cc_routine = fine_cc_routine;
    }

    /* do the job */
    jp->pi.pass_number = 0;
    while(!jp->termination_router((void *)jp)){
        /* reset counters */
        jp->pi.processed_clusters = 0;
        jp->pi.clusters_to_process = cc_routine(jp);
        
        result = defrag_routine(jp);
        if(result == 0){
            /* defragmentation succeeded at least once */
            overall_result = 0;
        }
        
        /* break if nothing moved */
        if(result < 0 || jp->pi.moved_clusters == 0) break;
        
        /* break if no repeat */
        if(!(jp->udo.job_flags & UD_JOB_REPEAT)) break;
        
        /* go to the next pass */
        jp->pi.pass_number ++;
    }
    
    return (jp->termination_router((void *)jp)) ? 0 : overall_result;
}

/************************************************************/
/*                  Rough defragmentation                   */
/*                 -----------------------                  */
/*   For nt4/w2k or fragment size threshold turned off.     */
/************************************************************/

/**
 * @brief Calculates total number of fragmented clusters.
 */
static ULONGLONG rough_cc_routine(udefrag_job_parameters *jp)
{
    udefrag_fragmented_file *f;
    ULONGLONG n = 0;
    
    for(f = jp->fragmented_files; f; f = f->next){
        if(jp->termination_router((void *)jp)) break;
        /*
        * Count all fragmented files which can be processed.
        */
        if(can_defragment(f->f,jp))
            n += f->f->disp.clusters;
        if(f->next == jp->fragmented_files) break;
    }
    return n;
}

/**
 * @internal
 * @brief Defragments all fragmented
 * files entirely whenever possible.
 * @details This routine is not so much
 * effective, but when fragment size
 * threshold optimization is turned off
 * it does exactly what's requested.
 * Also, in contrary to the fine_defrag_routine,
 * it can be used on nt4 and w2k systems
 * which have lots of crazy limitations
 * in FSCTL_MOVE_FILE API.
 */
static int rough_defrag_routine(udefrag_job_parameters *jp)
{
    ULONGLONG time, tm;
    ULONGLONG defragmented_files;
    winx_volume_region *rgn;
    udefrag_fragmented_file *f, *f_largest;
    ULONGLONG length;
    char buffer[32];
    ULONGLONG clusters_to_move;
    winx_file_info *file;
    ULONGLONG file_length;

    jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
    jp->pi.moved_clusters = 0;

    /* free as much temporarily allocated space as possible */
    release_temp_space_regions(jp);

    /* no files are excluded by this task currently */
    for(f = jp->fragmented_files; f; f = f->next){
        f->f->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(f->next == jp->fragmented_files) break;
    }

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("defragmentation",jp);

    /* move fragmented files to free regions large enough to hold all fragments */
    defragmented_files = 0;
    while(jp->termination_router((void *)jp) == 0){
        f_largest = NULL, length = 0; tm = winx_xtime();
        for(f = jp->fragmented_files; f; f = f->next){
            file_length = f->f->disp.clusters;
            if(file_length > length){
                if(can_defragment(f->f,jp) && !is_mft(f->f,jp)){
                    f_largest = f;
                    length = file_length;
                }
            }
            if(f->next == jp->fragmented_files) break;
        }
        jp->p_counters.searching_time += winx_xtime() - tm;
        if(f_largest == NULL) break;
        file = f_largest->f; /* f_largest may be destroyed by move_file */

        rgn = find_first_free_region(jp,file->disp.clusters);
        if(jp->termination_router((void *)jp)) break;
        if(rgn == NULL){
            /* exclude file from the current task */
            file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        } else {
            /* move the file */
            clusters_to_move = file->disp.clusters;
            if(move_file(file,file->disp.blockmap->vcn,
             clusters_to_move,rgn->lcn,0,jp) >= 0){
                if(jp->udo.dbgprint_level >= DBG_DETAILED)
                    DebugPrint("Defrag success for %ws",file->path);
                defragmented_files ++;
            } else {
                DebugPrint("Defrag failure for %ws",file->path);
                /* exclude file from the current task */
                file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
            }
        }
    }
    
    /* display amount of moved data and number of defragmented files */
    DebugPrint("%I64u files defragmented",defragmented_files);
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("defragmentation",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/************************************************************/
/*                   Fine defragmentation                   */
/*                  ----------------------                  */
/*   For XP and above, fragment size threshold turned on.   */
/************************************************************/

/**
 * @brief Calculates total number of clusters
 * needed to be moved to complete the defragmentation.
 */
static ULONGLONG fine_cc_routine(udefrag_job_parameters *jp)
{
    /* TODO */
    return 0;
}

/**
 * @internal
 * @brief Eliminates little fragments
 * respect to the fragment size threshold
 * filter.
 * @details This routine is much more effective
 * than the rough_defrag_routine, but requires
 * the fragment size threshold filter to be set.
 * Also it cannot be used on nt4 and w2k systems
 * because of FSCTL_MOVE_FILE API limitations.
 */
static int fine_defrag_routine(udefrag_job_parameters *jp)
{
    /* TODO */
    return 0;
}

/** @} */
