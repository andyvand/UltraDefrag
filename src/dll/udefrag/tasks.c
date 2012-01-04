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
 * @file tasks.c
 * @brief Volume processing tasks.
 * @details Set of atomic volume processing tasks 
 * defined here is built over the move_file routine
 * and is intended to build defragmentation and 
 * optimization routines over it.
 * @addtogroup Tasks
 * @{
 */

/*
* Ideas by Dmitri Arkhangelski <dmitriar@gmail.com>
* and Stefan Pendl <stefanpe@users.sourceforge.net>.
*/

#include "udefrag-internals.h"

/************************************************************/
/*                      Atomic Tasks                        */
/************************************************************/

/**
 * @brief Auxiliary routine used to sort files in binary tree.
 */
static int files_compare(const void *prb_a, const void *prb_b, void *prb_param)
{
    winx_file_info *a, *b;
    
    a = (winx_file_info *)prb_a;
    b = (winx_file_info *)prb_b;
    
    if(a->disp.blockmap == NULL || b->disp.blockmap == NULL){
        DebugPrint("files_compare: unexpected condition");
        return 0;
    }
    
    if(a->disp.blockmap->lcn < b->disp.blockmap->lcn)
        return (-1);
    if(a->disp.blockmap->lcn == b->disp.blockmap->lcn)
        return 0;
    return 1;
}

/**
 * @brief Moves selected group of files
 * to the beginning of the volume to free its end.
 * @details This routine tries to move each file entirely
 * to avoid increasing fragmentation.
 * @param[in] jp job parameters.
 * @param[in] start_lcn the first cluster of an area intended 
 * to be cleaned up - all clusters before it should be skipped.
 * @param[in] flags one of MOVE_xxx flags defined in udefrag.h
 * @return Zero for success, negative value otherwise.
 */
int move_files_to_front(udefrag_job_parameters *jp, ULONGLONG start_lcn, int flags)
{
    ULONGLONG time, tm;
    ULONGLONG moves, pass;
    winx_file_info *file,/* *last_file, */*largest_file;
    ULONGLONG length, rgn_size_threshold;
    int files_found;
    ULONGLONG min_lcn, rgn_lcn, min_file_lcn, min_rgn_lcn, max_rgn_lcn;
    /*ULONGLONG end_lcn, last_lcn;*/
    winx_volume_region *rgn;
    ULONGLONG clusters_to_move;
    ULONGLONG file_length, file_lcn;
    winx_blockmap *block;
    ULONGLONG new_sp;
    int completed;
    char buffer[32];
    
    struct prb_table *pt;
    struct prb_traverser t;
    winx_file_info f;
    winx_blockmap b;
    
    jp->pi.current_operation = VOLUME_OPTIMIZATION;
    jp->pi.moved_clusters = 0;
    
    pt = prb_create(files_compare,NULL,NULL);
    if(pt == NULL){
        DebugPrint("move_files_to_front: cannot create files tree");
        DebugPrint("move_files_to_front: slow file search will be used");
    }
    
    /* no files are excluded by this task currently */
    for(file = jp->filelist; file; file = file->next){
        file->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(pt != NULL){
            if(can_move(file) && !is_mft(file,jp)){
                if(file->disp.blockmap->lcn >= start_lcn){
                    if(prb_probe(pt,(void *)file) == NULL){
                        DebugPrint("move_files_to_front: cannot add file to the tree");
                        prb_destroy(pt,NULL);
                        pt = NULL;
                    }
                }
            }
        }
        if(file->next == jp->filelist) break;
    }
    
    /*DebugPrint("start LCN = %I64u",start_lcn);
    file = prb_t_first(&t,pt);
    while(file){
        DebugPrint("file %ws at %I64u",file->path,file->disp.blockmap->lcn);
        file = prb_t_next(&t);
    }*/

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("file moving to front",jp);
    
    /* do the job */
    /* strategy 1: the most effective one */
    rgn_size_threshold = 1; pass = 0; jp->pi.total_moves = 0;
    while(!jp->termination_router((void *)jp)){
        moves = 0;
    
        /* free as much temporarily allocated space as possible */
        release_temp_space_regions(jp);
        
        min_rgn_lcn = 0; max_rgn_lcn = jp->v_info.total_clusters - 1;
repeat_scan:
        for(rgn = jp->free_regions; rgn; rgn = rgn->next){
            if(jp->termination_router((void *)jp)) break;
            /* break if we have already moved files behind the current region */
            if(rgn->lcn > max_rgn_lcn) break;
            if(rgn->length > rgn_size_threshold && rgn->lcn >= min_rgn_lcn){
                /* break if on the next pass the current region will be moved to the end */
                if(jp->udo.job_flags & UD_JOB_REPEAT){
                    new_sp = calculate_starting_point(jp,start_lcn);
                    if(rgn->lcn > new_sp){
                        completed = 1;
                        if(jp->job_type == QUICK_OPTIMIZATION_JOB){
                            if(get_number_of_fragmented_clusters(jp,new_sp,rgn->lcn) == 0)
                                completed = 0;
                        }
                        if(completed){
                            DebugPrint("rgn->lcn = %I64u, rgn->length = %I64u",rgn->lcn,rgn->length);
                            DebugPrint("move_files_to_front: heavily fragmented space begins at %I64u cluster",new_sp);
                            goto done;
                        }
                    }
                }
try_again:
                tm = winx_xtime();
                if(pt != NULL){
                    largest_file = NULL; length = 0; files_found = 0;
                    b.lcn = rgn->lcn + 1;
                    f.disp.blockmap = &b;
                    prb_t_init(&t,pt);
                    file = prb_t_insert(&t,pt,&f);
                    if(file == NULL){
                        DebugPrint("move_files_to_front: cannot insert file to the tree");
                        prb_destroy(pt,NULL);
                        pt = NULL;
                        goto slow_search;
                    }
                    if(file == &f){
                        file = prb_t_next(&t);
                        if(prb_delete(pt,&f) == NULL){
                            DebugPrint("move_files_to_front: cannot remove file from the tree (case 1)");
                            prb_destroy(pt,NULL);
                            pt = NULL;
                        }
                    }
                    while(file){
                        /*DebugPrint("XX: %ws",file->path);*/
                        if(!can_move(file) || is_mft(file,jp)){
                        } else if((flags == MOVE_FRAGMENTED) && !is_fragmented(file)){
                        } else if((flags == MOVE_NOT_FRAGMENTED) && is_fragmented(file)){
                        } else {
                            files_found = 1;
                            file_length = file->disp.clusters;
                            if(file_length > length \
                              && file_length <= rgn->length){
                                largest_file = file;
                                length = file_length;
                                if(file_length == rgn->length) break;
                            }
                        }
                        file = prb_t_next(&t);
                    }
                } else {
slow_search:
                    largest_file = NULL; length = 0; files_found = 0;
                    min_lcn = max(start_lcn, rgn->lcn + 1);
                    for(file = jp->filelist; file; file = file->next){
                        if(can_move(file) && !is_mft(file,jp)){
                            if((flags == MOVE_FRAGMENTED) && !is_fragmented(file)){
                            } else if((flags == MOVE_NOT_FRAGMENTED) && is_fragmented(file)){
                            } else {
                                if(file->disp.blockmap->lcn >= min_lcn){
                                    files_found = 1;
                                    file_length = file->disp.clusters;
                                    if(file_length > length \
                                      && file_length <= rgn->length){
                                        largest_file = file;
                                        length = file_length;
                                    }
                                }
                            }
                        }
                        if(file->next == jp->filelist) break;
                    }
                }
                jp->p_counters.searching_time += winx_xtime() - tm;

                if(files_found == 0){
                    /* no more files can be moved on the current pass */
                    break; /*goto done;*/
                }
                if(largest_file == NULL){
                    /* current free region is too small, let's try next one */
                    rgn_size_threshold = rgn->length;
                } else {
                    if(is_file_locked(largest_file,jp)){
                        jp->pi.processed_clusters += largest_file->disp.clusters;
                        goto try_again; /* skip locked files */
                    }
                    /* move the file */
                    min_file_lcn = jp->v_info.total_clusters - 1;
                    for(block = largest_file->disp.blockmap; block; block = block->next){
                        if(block->length && block->lcn < min_file_lcn)
                            min_file_lcn = block->lcn;
                        if(block->next == largest_file->disp.blockmap) break;
                    }
                    rgn_lcn = rgn->lcn;
                    clusters_to_move = largest_file->disp.clusters;
                    file_lcn = largest_file->disp.blockmap->lcn;
                    if(move_file(largest_file,largest_file->disp.blockmap->vcn,clusters_to_move,rgn->lcn,0,jp) >= 0){
                        moves ++;
                        jp->pi.total_moves ++;
                    }
                    /* regardless of result, exclude the file */
                    largest_file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
                    largest_file->user_defined_flags |= UD_FILE_MOVED_TO_FRONT;
                    /* remove file from the tree to speed things up */
                    block = largest_file->disp.blockmap;
                    if(pt != NULL /*&& largest_file->disp.blockmap == NULL*/){ /* remove invalid tree items */
                        b.lcn = file_lcn;
                        largest_file->disp.blockmap = &b;
                        if(prb_delete(pt,largest_file) == NULL){
                            DebugPrint("move_files_to_front: cannot remove file from the tree (case 2)");
                            prb_destroy(pt,NULL);
                            pt = NULL;
                        }
                    }
                    largest_file->disp.blockmap = block;
                    /* continue from the first free region after used one */
                    min_rgn_lcn = rgn_lcn + 1;
                    if(max_rgn_lcn > min_file_lcn - 1)
                        max_rgn_lcn = min_file_lcn - 1;
                    goto repeat_scan;
                }
            }
            if(rgn->next == jp->free_regions) break;
        }
    
        if(moves == 0) break;
        DebugPrint("move_files_to_front: pass %I64u completed, %I64u files moved",pass,moves);
        pass ++;
    }

done:
    /* display amount of moved data */
    DebugPrint("%I64u files moved totally",jp->pi.total_moves);
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    if(pt != NULL) prb_destroy(pt,NULL);
    stop_timing("file moving to front",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/**
 * @brief Moves selected group of files
 * to the end of the volume to free its
 * beginning.
 * @brief This routine moves individual clusters
 * and never tries to keep the file not fragmented.
 * @param[in] jp job parameters.
 * @param[in] start_lcn the first cluster of an area intended 
 * to be cleaned up - all clusters before it should be skipped.
 * @param[in] flags one of MOVE_xxx flags defined in udefrag.h
 * @return Zero for success, negative value otherwise.
 */
int move_files_to_back(udefrag_job_parameters *jp, ULONGLONG start_lcn, int flags)
{
    ULONGLONG time;
    int nt4w2k_limitations = 0;
    winx_file_info *file, *first_file;
    winx_blockmap *first_block;
    ULONGLONG block_lcn, block_length, n;
    ULONGLONG current_vcn, remaining_clusters;
    winx_volume_region *rgn;
    ULONGLONG clusters_to_move;
    char buffer[32];
    
    jp->pi.current_operation = VOLUME_OPTIMIZATION;
    jp->pi.moved_clusters = 0;
    jp->pi.total_moves = 0;
    
    /* free as much temporarily allocated space as possible */
    release_temp_space_regions(jp);

    /* no files are excluded by this task currently */
    for(file = jp->filelist; file; file = file->next){
        file->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(file->next == jp->filelist) break;
    }

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("file moving to end",jp);

    if(winx_get_os_version() <= WINDOWS_2K && jp->fs_type == FS_NTFS){
        DebugPrint("Windows NT 4.0 and Windows 2000 have stupid limitations in defrag API");
        DebugPrint("volume optimization is not so much effective on these systems");
        nt4w2k_limitations = 1;
    }
    
    /* do the job */
    while(!jp->termination_router((void *)jp)){
        /* find the first block after start_lcn */
        first_block = find_first_block(jp,&start_lcn,flags,1,&first_file);
        if(first_block == NULL) break;
        /* move the first block to the last free regions */
        block_lcn = first_block->lcn; block_length = first_block->length;
        if(!nt4w2k_limitations){
            /* sure, we can fill by the first block's data the last free regions */
            if(jp->free_regions == NULL) goto done;
            current_vcn = first_block->vcn;
            remaining_clusters = first_block->length;
            for(rgn = jp->free_regions->prev; rgn && remaining_clusters; rgn = jp->free_regions->prev){
                if(rgn->lcn < block_lcn + block_length){
                    /* no more moves to the end of disk are possible */
                    goto done;
                }
                if(rgn->length > 0){
                    n = min(rgn->length,remaining_clusters);
                    if(move_file(first_file,current_vcn,n,rgn->lcn + rgn->length - n,0,jp) < 0){
                        /* exclude file from the current task to avoid infinite loops */
                        file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
                    }
                    current_vcn += n;
                    remaining_clusters -= n;
                    jp->pi.total_moves ++;
                }
                if(jp->free_regions == NULL) goto done;
            }
        } else {
            /* it is safe to move entire files and entire blocks of compressed/sparse files */
            if(is_compressed(first_file) || is_sparse(first_file)){
                /* move entire block */
                current_vcn = first_block->vcn;
                clusters_to_move = first_block->length;
            } else {
                /* move entire file */
                current_vcn = first_file->disp.blockmap->vcn;
                clusters_to_move = first_file->disp.clusters;
            }
            rgn = find_matching_free_region(jp,first_block->lcn,clusters_to_move,FIND_MATCHING_RGN_FORWARD);
            //rgn = find_last_free_region(jp,clusters_to_move);
            if(rgn != NULL){
                if(rgn->lcn > first_block->lcn){
                    move_file(first_file,current_vcn,clusters_to_move,
                        rgn->lcn + rgn->length - clusters_to_move,0,jp);
                    jp->pi.total_moves ++;
                }
            }
            /* otherwise the volume processing is extremely slow and may even go to an infinite loop */
            first_file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        }
    }
    
done:
    /* display amount of moved data */
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("file moving to end",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/** @} */
