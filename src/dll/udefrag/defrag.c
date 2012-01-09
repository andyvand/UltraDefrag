/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2012 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
winx_blockmap *build_fragments_list(winx_file_info *f);
void release_fragments_list(winx_blockmap **fragments);

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
    if(move_file(f,f->disp.blockmap->vcn,1,target_rgn->lcn,jp) < 0){
        DebugPrint("test_move: move failed for %ws",f->path);
        return;
    } else {
        DebugPrint("test_move: move succeeded for %ws",f->path);
    }
    /* release temporary allocated clusters */
    release_temp_space_regions(jp);
    /* try to move the first cluster back */
    if(can_move(f,jp)){
        if(move_file(f,f->disp.blockmap->vcn,1,source_lcn,jp) < 0){
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
/*                   Auxiliary routines                     */
/************************************************************/

/**
 * @brief Defines whether the file can be defragmented or not.
 */
static int can_defragment(winx_file_info *f,udefrag_job_parameters *jp)
{
    if(!can_move(f))
        return 0;

    /* skip files with less than 2 fragments */
    if(f->disp.blockmap->next == f->disp.blockmap \
      || f->disp.fragments < 2 || !is_fragmented(f))
        return 0;
        
    /* skip MFT */
    if(is_mft(f,jp))
        return 0;
    
    /* skip FAT directories */
    if(jp->is_fat && is_directory(f))
        return 0;
        
    /* in MFT optimization defragment marked files only */
    if(jp->job_type == MFT_OPTIMIZATION_JOB \
      && !is_fragmented_by_file_opt(f))
        return 0;
    
    return 1;
}

/**
 * @brief build_fragments_list helper.
 */
static winx_blockmap *add_fragment(winx_blockmap **fragments,
    winx_blockmap **prev_fragment, ULONGLONG vcn, ULONGLONG lcn,
    ULONGLONG length)
{
    winx_blockmap *fragment;
    
    fragment = (winx_blockmap *)winx_list_insert_item((list_entry **)(void *)fragments,
        (list_entry *)*prev_fragment,sizeof(winx_blockmap));
    if(fragment == NULL){
        release_fragments_list(fragments);
    } else {
        fragment->vcn = vcn;
        fragment->lcn = lcn;
        fragment->length = length;
    }
    *prev_fragment = fragment;
    return fragment;
}

/**
 * @brief Builds list of file fragments.
 */
winx_blockmap *build_fragments_list(winx_file_info *f)
{
    winx_blockmap *block, *p = NULL, *fragments = NULL;
    ULONGLONG vcn = 0, lcn = 0, length = 0;
    
    for(block = f->disp.blockmap; block; block = block->next){
        if(block == f->disp.blockmap){
            vcn = block->vcn;
            lcn = block->lcn;
            length = block->length;
        } else {
            if(block->lcn == block->prev->lcn + block->prev->length){
                length += block->length;
            } else {
                if(length){
                    if(!add_fragment(&fragments,&p,vcn,lcn,length))
                        break;
                }
                vcn = block->vcn;
                lcn = block->lcn;
                length = block->length;
            }
        }
        if(block->next == f->disp.blockmap) break;
    }
    
    if(length) add_fragment(&fragments,&p,vcn,lcn,length);
    
    return fragments;
}

/**
 * @brief Releases list of file fragments.
 */
void release_fragments_list(winx_blockmap **fragments)
{
    winx_list_destroy((list_entry **)(void *)fragments);
}

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
    if(jp->weak_api || jp->udo.fragment_size_threshold == 0){
        if(jp->udo.fragment_size_threshold == 0){
            DebugPrint("defragment: rough routines will be used");
            DebugPrint("defragment: because of fragment size threshold filter not set");
        }
        defrag_routine = rough_defrag_routine;
        cc_routine = rough_cc_routine;
    } else {
        DebugPrint("defragment: fine routines will be used");
        defrag_routine = fine_defrag_routine;
        cc_routine = fine_cc_routine;
    }

    /* reset counters */
    jp->pi.processed_clusters = 0;
    jp->pi.clusters_to_process = cc_routine(jp);
    
    /* do the job */
    jp->pi.pass_number = 0;
    while(!jp->termination_router((void *)jp)){
        result = defrag_routine(jp);
        if(result == 0){
            /* defragmentation succeeded at least once */
            overall_result = 0;
        }
        
        /* break if nothing moved */
        if(result < 0 || jp->pi.moved_clusters == 0) break;
        
        /* defragment a few remaining files on the next pass */
        jp->pi.pass_number ++;
    }
    
    /* defragment remaining files partially */
    if(jp->weak_api == 0 && jp->udo.fragment_size_threshold == 0){
        jp->udo.fragment_size_threshold = PART_DEFRAG_MAGIC_CONSTANT;
        jp->udo.algorithm_defined_fst = 1;
        DebugPrint("partial defragmentation: fragment size threshold = %I64u",
            jp->udo.fragment_size_threshold);
        if(fine_defrag_routine(jp) == 0){
            /* at least partial defragmentation succeeded */
            overall_result = 0;
        }
        jp->udo.fragment_size_threshold = 0;
        jp->udo.algorithm_defined_fst = 0;
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
    udefrag_fragmented_file *f, *head, *next;
    winx_volume_region *rgn;
    winx_file_info *file;
    ULONGLONG defragmented_files;
    char buffer[32];
    ULONGLONG time;

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

    /*
    * Move fragmented files to free regions large enough
    * to join all fragments together. Defragment the most
    * fragmented files first of all.
    */
    defragmented_files = 0;
    for(f = jp->fragmented_files; f; f = next){
        if(jp->termination_router((void *)jp)) break;
        head = jp->fragmented_files;
        next = f->next;
        file = f->f; /* f will be destroyed by move_file */
        if(can_defragment(file,jp)){
            /* defragment the file */
            rgn = find_first_free_region(jp,0,file->disp.clusters);
            if(rgn){
                if(move_file(file,file->disp.blockmap->vcn,
                 file->disp.clusters,rgn->lcn,jp) >= 0){
                    if(jp->udo.dbgprint_level >= DBG_DETAILED)
                        DebugPrint("Defrag success for %ws",file->path);
                    defragmented_files ++;
                } else {
                    DebugPrint("Defrag failure for %ws",file->path);
                }
            }
        }
        file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        /* go to the next file */
        if(jp->fragmented_files == NULL) break;
        if(next == head) break;
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
    /* fine calculation will take too much time */
    return rough_cc_routine(jp);
}

/**
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
    udefrag_fragmented_file *f, *head, *next;
    winx_volume_region *rgn;
    winx_file_info *file;
    ULONGLONG defragmented_files;
    ULONGLONG defragmented_entirely = 0, defragmented_partially = 0;
    ULONGLONG x, moved_entirely = 0, moved_partially = 0;
    ULONGLONG min_vcn, max_vcn; /* used to avoid infinite loops */
    winx_blockmap *fragments, *fr, *fr2, *next_fr, *head_fr;
    ULONGLONG vcn, length, n, new_min_vcn;
    ULONGLONG cut_length;
    int defrag_succeeded;
    char buffer[32];
    ULONGLONG time;

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

    /*
    * Eliminate little fragments. Defragment
    * the most fragmented files first of all.
    */
    defragmented_files = 0;
    for(f = jp->fragmented_files; f; f = next){
        if(jp->termination_router((void *)jp)) break;
        head = jp->fragmented_files;
        next = f->next;
        file = f->f; /* f will be destroyed by move_file */
        if(can_defragment(file,jp)){
            if(file->disp.clusters * jp->v_info.bytes_per_cluster \
              < 2 * jp->udo.fragment_size_threshold){
                /* move entire file */
                rgn = find_first_free_region(jp,0,file->disp.clusters);
                if(rgn){
                    x = jp->pi.moved_clusters;
                    if(move_file(file,file->disp.blockmap->vcn,
                     file->disp.clusters,rgn->lcn,jp) >= 0){
                        if(jp->udo.dbgprint_level >= DBG_DETAILED)
                            DebugPrint("Defrag success for %ws",file->path);
                        defragmented_files ++;
                        defragmented_entirely ++;
                        moved_entirely += (jp->pi.moved_clusters - x);
                    } else {
                        DebugPrint("Defrag failure for %ws",file->path);
                    }
                }
            } else {
                /* eliminate little fragments */
                min_vcn = file->disp.blockmap->vcn;
                max_vcn = file->disp.blockmap->prev->vcn + \
                    file->disp.blockmap->prev->length;
                defrag_succeeded = 0;
                x = jp->pi.moved_clusters;
                while(min_vcn < max_vcn){
                    /* build list of fragments */
                    fragments = build_fragments_list(file);
                    if(fragments == NULL) break;
                    
                    /* cut off already processed fragments and data after max_vcn */
                    for(fr = fragments; fr; fr = next_fr){
                        head_fr = fragments;
                        next_fr = fr->next;
                        if(fr->vcn < min_vcn || (fr->vcn + fr->length > max_vcn))
                            winx_list_remove_item((list_entry **)(void *)&fragments,(list_entry *)(void *)fr);
                        if(fragments == NULL) goto completed;
                        if(next_fr == head_fr) break;
                    }
                    
                    /* find clusters needing optimization */
                    vcn = length = n = new_min_vcn = 0;
                    for(fr = fragments; fr; fr = fr->next){
                        /* find the first little fragment */
                        if(fr->length * jp->v_info.bytes_per_cluster < jp->udo.fragment_size_threshold){
                            vcn = fr->vcn;
                            length = fr->length, n++;
                            new_min_vcn = fr->vcn + fr->length;
                            /* look forward for the next little fragments */
                            for(fr2 = fr->next; fr2 != fragments; fr2 = fr2->next){
                                if(fr2->length * jp->v_info.bytes_per_cluster >= jp->udo.fragment_size_threshold)
                                    break;
                                length += fr2->length, n++;
                                new_min_vcn = fr2->vcn + fr2->length;
                            }
                            if(length * jp->v_info.bytes_per_cluster < jp->udo.fragment_size_threshold){
                                cut_length = jp->udo.fragment_size_threshold / jp->v_info.bytes_per_cluster;
                                if(cut_length * jp->v_info.bytes_per_cluster != jp->udo.fragment_size_threshold)
                                    cut_length ++;
                                cut_length -= length;
                                if(fr2 != fragments){
                                    /* let's cut from the next fragment */
                                    if((fr2->length - cut_length) * jp->v_info.bytes_per_cluster \
                                      < jp->udo.fragment_size_threshold){
                                        length += fr2->length, n++;
                                        new_min_vcn = fr2->vcn + fr2->length;
                                    } else {
                                        length += cut_length, n++;
                                        new_min_vcn = fr2->vcn + cut_length;
                                    }
                                } else if(fr != fragments){
                                    /* let's cut from the previous fragment */
                                    if((fr->prev->length - cut_length) * jp->v_info.bytes_per_cluster \
                                      < jp->udo.fragment_size_threshold){
                                        vcn = fr->prev->vcn;
                                        length += fr->prev->length, n++;
                                    } else {
                                        vcn = fr->prev->vcn + (fr->prev->length - cut_length);
                                        length += cut_length, n++;
                                    }
                                }
                            }
                            break;
                        }
                        if(fr->next == fragments) break;
                    }
                    
                    /* move clusters */
                    if(length == 0 || n < 2){
                        min_vcn = max_vcn;
                    } else {
                        rgn = find_first_free_region(jp,0,length);
                        if(rgn){
                            if(move_file(file,vcn,length,rgn->lcn,jp) >= 0){
                                if(jp->udo.dbgprint_level >= DBG_DETAILED)
                                    DebugPrint("Defrag success for %ws",file->path);
                                defrag_succeeded = 1;
                            } else {
                                DebugPrint("Defrag failure for %ws",file->path);
                            }
                        }
                        min_vcn = new_min_vcn;
                    }
                    
                    /* release list of fragments */
                    release_fragments_list(&fragments);
                }
                if(defrag_succeeded){
                    defragmented_files ++;
                    defragmented_partially ++;
                    moved_partially += (jp->pi.moved_clusters - x);
                }
            }
        }
completed:
        file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        /* go to the next file */
        if(jp->fragmented_files == NULL) break;
        if(next == head) break;
    }
    
    /* display amount of moved data and number of defragmented files */
    DebugPrint("%I64u files defragmented",defragmented_files);
    DebugPrint("  %I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("  %s moved",buffer);
    
    DebugPrint("%I64u files defragmented entirely",defragmented_entirely);
    DebugPrint("  %I64u clusters moved",moved_entirely);
    winx_bytes_to_hr(moved_entirely * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("  %s moved",buffer);
    DebugPrint("%I64u files defragmented partially",defragmented_partially);
    DebugPrint("  %I64u clusters moved",moved_partially);
    winx_bytes_to_hr(moved_partially * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("  %s moved",buffer);
    
    stop_timing("defragmentation",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/** @} */
