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
 * @file move.c
 * @brief File moving.
 * @addtogroup Move
 * @{
 */

#include "udefrag-internals.h"

/**
 * @brief Actualizes the free space regions list.
 * @details All NTFS regions temporarily allocated
 * by system become freed after this call.
 */
void release_temp_space_regions(udefrag_job_parameters *jp)
{
    ULONGLONG time = winx_xtime();
    
    if(!jp->udo.dry_run){
        winx_release_free_volume_regions(jp->free_regions);
        jp->free_regions = winx_get_free_volume_regions(jp->volume_letter,
            WINX_GVR_ALLOW_PARTIAL_SCAN,NULL,(void *)jp);
        jp->p_counters.temp_space_releasing_time += winx_xtime() - time;
    }
}

/************************************************************/
/*                    can_move routine                      */
/************************************************************/

/**
 * @brief Defines whether the file can be
 * moved or not, at least partially.
 */
int can_move(winx_file_info *f)
{
    /* skip files with empty path */
    if(f->path == NULL)
        return 0;
    if(f->path[0] == 0)
        return 0;
    
    /* skip files already moved to front in optimization */
    if(is_moved_to_front(f))
        return 0;
    
    /* skip files already excluded by the current task */
    if(is_currently_excluded(f))
        return 0;
    
    /* skip files with undefined cluster map and locked files */
    if(f->disp.blockmap == NULL || is_locked(f))
        return 0;
    
    /* skip files of zero length */
    if(f->disp.clusters == 0 || \
      (f->disp.blockmap->next == f->disp.blockmap && \
      f->disp.blockmap->length == 0)){
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        return 0;
    }

    /* skip file in case of improper state detected */
    if(is_in_improper_state(f))
        return 0;
    
    /* avoid infinite loops */
    if(is_moving_failed(f))
        return 0;
    
    return 1;
}

/**
 * @brief Defines whether the file
 * can be moved entirely or not.
 */
int can_move_entirely(winx_file_info *f,udefrag_job_parameters *jp)
{
    if(!can_move(f))
        return 0;

    /* the first clusters of MFT cannot be moved */
    if(is_mft(f,jp))
        return 0;
    
    /* the first clusters of FAT directories cannot be moved */
    if(jp->is_fat && is_directory(f))
        return 0;
        
    return 1;
}

/************************************************************/
/*                    Internal Routines                     */
/************************************************************/

/**
 * @brief Returns the first file block
 * belonging to the cluster chain.
 */
static winx_blockmap *get_first_block_of_cluster_chain(winx_file_info *f,ULONGLONG vcn)
{
    winx_blockmap *block;
    
    for(block = f->disp.blockmap; block; block = block->next){
        if(vcn >= block->vcn && vcn < block->vcn + block->length)
            return block;
        if(block->next == f->disp.blockmap) break;
    }
    return NULL;
}

/**
 * @brief Moves file clusters.
 * @return Zero for success,
 * negative value otherwise.
 * @note 
 * - Execution of the FSCTL_MOVE_FILE request
 * cannot be interrupted, so it's a good idea
 * to move little portions of data at once.
 * - Volume must be opened before this call,
 * jp->fVolume must contain a proper handle.
 */
static int move_file_clusters(HANDLE hFile,ULONGLONG startVcn,
    ULONGLONG targetLcn,ULONGLONG n_clusters,udefrag_job_parameters *jp)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    MOVEFILE_DESCRIPTOR mfd;

    if(jp->udo.dbgprint_level >= DBG_DETAILED){
        DebugPrint("sVcn: %I64u,tLcn: %I64u,n: %u",
             startVcn,targetLcn,n_clusters);
    }
    
    if(jp->termination_router((void *)jp))
        return (-1);
    
    if(jp->udo.dry_run){
        jp->pi.moved_clusters += n_clusters;
        return 0;
    }

    /* setup movefile descriptor and make the call */
    mfd.FileHandle = hFile;
    mfd.StartVcn.QuadPart = startVcn;
    mfd.TargetLcn.QuadPart = targetLcn;
#ifdef _WIN64
    mfd.NumVcns = n_clusters;
#else
    mfd.NumVcns = (ULONG)n_clusters;
#endif
    Status = NtFsControlFile(winx_fileno(jp->fVolume),NULL,NULL,0,&iosb,
                        FSCTL_MOVE_FILE,&mfd,sizeof(MOVEFILE_DESCRIPTOR),
                        NULL,0);
    if(NT_SUCCESS(Status)){
        NtWaitForSingleObject(winx_fileno(jp->fVolume),FALSE,NULL);
        Status = iosb.Status;
    }
    jp->last_move_status = Status;
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"cannot move file clusters");
        return (-1);
    }

    /*
    * Actually moving result is unknown here,
    * because API may return success, while
    * actually clusters may be moved partially.
    */
    jp->pi.moved_clusters += n_clusters;
    return 0;
}

/**
 * @brief move_file helper.
 */
static void move_file_helper(HANDLE hFile, winx_file_info *f,
    ULONGLONG vcn, ULONGLONG length, ULONGLONG target,
    udefrag_job_parameters *jp)
{
    winx_blockmap *block, *first_block;
    ULONGLONG curr_vcn, curr_target, j, n, r;
    ULONGLONG clusters_to_process;
    ULONGLONG clusters_to_move;
    int result;
    
    clusters_to_process = length;
    curr_vcn = vcn;
    curr_target = target;
    
    /* move blocks of the file */
    first_block = get_first_block_of_cluster_chain(f,vcn);
    for(block = first_block; block; block = block->next){
        /* move the current block or its part */
        clusters_to_move = min(block->length - (curr_vcn - block->vcn),clusters_to_process);
        n = clusters_to_move / jp->clusters_at_once;
        for(j = 0; j < n; j++){
            result = move_file_clusters(hFile,curr_vcn,
                curr_target,jp->clusters_at_once,jp);
            if(result < 0)
                goto done;
            jp->pi.processed_clusters += jp->clusters_at_once;
            clusters_to_process -= jp->clusters_at_once;
            curr_vcn += jp->clusters_at_once;
            curr_target += jp->clusters_at_once;
        }
        /* try to move rest of the block */
        r = clusters_to_move % jp->clusters_at_once;
        if(r){
            result = move_file_clusters(hFile,curr_vcn,curr_target,r,jp);
            if(result < 0)
                goto done;
            jp->pi.processed_clusters += r;
            clusters_to_process -= r;
            curr_vcn += r;
            curr_target += r;
        }
        if(!clusters_to_move || block->next == f->disp.blockmap) break;
        curr_vcn = block->next->vcn;
    }

done: /* count all unprocessed clusters here */
    jp->pi.processed_clusters += clusters_to_process;
}

/**
 * @brief Prints list of file blocks.
 */
static void DbgPrintBlocksOfFile(winx_blockmap *blockmap)
{
    winx_blockmap *block;
    
    for(block = blockmap; block; block = block->next){
        DebugPrint("VCN: %I64u, LCN: %I64u, LENGTH: %u",
            block->vcn,block->lcn,block->length);
        if(block->next == blockmap) break;
    }
}

/**
 * @brief Adds a new block to the file map.
 */
static winx_blockmap *add_new_block(winx_blockmap **head,ULONGLONG vcn,ULONGLONG lcn,ULONGLONG length)
{
    winx_blockmap *block, *last_block = NULL;
    
    if(*head != NULL)
        last_block = (*head)->prev;
    
    block = (winx_blockmap *)winx_list_insert_item((list_entry **)head,
                (list_entry *)last_block,sizeof(winx_blockmap));
    if(block != NULL){
        block->vcn = vcn;
        block->lcn = lcn;
        block->length = length;
    }
    return block;
}

/**
 * @brief Calculates new file disposition.
 * @param[in] f pointer to the file information structure.
 * @param[in] vcn VCN of the moved cluster chain.
 * @param[in] length length of the moved cluster chain.
 * @param[in] target new LCN of the moved cluster chain.
 * @param[out] new_file_info pointer to structure 
 * receiving the new file information.
 */
static void calculate_file_disposition(winx_file_info *f,ULONGLONG vcn,
    ULONGLONG length,ULONGLONG target,winx_file_info *new_file_info)
{
    winx_blockmap *block, *first_block, *fragments;
    ULONGLONG clusters_to_check, curr_vcn, curr_target, n;
    
    /* duplicate file information */
    memcpy(new_file_info,f,sizeof(winx_file_info));
    
    /* reset new file disposition */
    new_file_info->disp.blockmap = NULL;
    new_file_info->disp.fragments = 0;
    
    first_block = get_first_block_of_cluster_chain(f,vcn);
    if(first_block == NULL){
        DebugPrint("calculate_file_disposition: get_first_block_of_cluster_chain failed");
        new_file_info->disp.clusters = 0;
        return;
    }
    
    /* add all blocks prior to the first_block to the new disposition */
    for(block = f->disp.blockmap;
      block && block != first_block;
      block = block->next){
        if(!add_new_block(&new_file_info->disp.blockmap,
            block->vcn,block->lcn,block->length)) goto fail;
    }
    
    /* add all remaining blocks */
    clusters_to_check = length;
    curr_vcn = vcn;
    curr_target = target;
    for(block = first_block; block; block = block->next){
        if(!clusters_to_check){
            if(!add_new_block(&new_file_info->disp.blockmap,
                block->vcn,block->lcn,block->length)) goto fail;
        } else {
            n = min(block->length - (curr_vcn - block->vcn),clusters_to_check);
            
            if(curr_vcn != block->vcn){
                /* we have the second part of block moved */
                if(!add_new_block(&new_file_info->disp.blockmap,
                    block->vcn,block->lcn,block->length - n)) goto fail;
                if(!add_new_block(&new_file_info->disp.blockmap,
                    curr_vcn,curr_target,n)) goto fail;
            } else {
                if(n != block->length){
                    /* we have the first part of block moved */
                    if(!add_new_block(&new_file_info->disp.blockmap,
                        curr_vcn,curr_target,n)) goto fail;
                    if(!add_new_block(&new_file_info->disp.blockmap,
                        block->vcn + n,block->lcn + n,block->length - n)) goto fail;
                } else {
                    /* we have entire block moved */
                    if(!add_new_block(&new_file_info->disp.blockmap,
                        block->vcn,curr_target,block->length)) goto fail;
                }
            }
            /* XXX: when middle part of the block moved, behaviour is 
               unexpected; however, this never happens in current algorithms
            */

            curr_target += n;
            clusters_to_check -= n;
        }
        if(block->next == f->disp.blockmap) break;
        curr_vcn = block->next->vcn;
    }
    
    /* replace list of blocks by list of fragments */
    fragments = build_fragments_list(new_file_info,&n);
    winx_list_destroy((list_entry **)(void *)&new_file_info->disp.blockmap);
    new_file_info->disp.blockmap = fragments;
    new_file_info->disp.fragments = n;
    return;
    
fail:
    DebugPrint("calculate_file_disposition: not enough memory");
    winx_list_destroy((list_entry **)(void *)&new_file_info->disp.blockmap);
    new_file_info->disp.fragments = 0;
    new_file_info->disp.clusters = 0;
}

/**
 * @brief Compares two file dispositions.
 * @return Positive value indicates 
 * difference, zero indicates equality.
 * Negative value indicates that 
 * arguments are invalid.
 */
static int compare_file_dispositions(winx_file_info *f1, winx_file_info *f2)
{
    winx_blockmap *map1, *map2, *b1, *b2;
    ULONGLONG n1, n2;

    /* validate arguments */
    if(f1 == NULL || f2 == NULL)
        return (-1);
    
    /* get lists of fragments */
    map1 = build_fragments_list(f1,&n1);
    map2 = build_fragments_list(f2,&n2);
    
    /* empty maps are equal */
    if(map1 == NULL && map2 == NULL){
equal_maps:
        release_fragments_list(&map1);
        release_fragments_list(&map2);
        return 0;
    }
    
    /* equal maps have equal lengths */
    if(n1 != n2) goto different_maps;
    
    /* comapare maps */
    for(b1 = map1, b2 = map2; b1 && b2; b1 = b1->next, b2 = b2->next){
        if((b1->vcn != b2->vcn) 
          || (b1->lcn != b2->lcn)
          || (b1->length != b2->length))
            break;
        if(b1->next == map1 && b2->next == map2) goto equal_maps;
        if(b1->next == map1 || b2->next == map2) break;
    }
    
different_maps:
    /* maps are different */
    release_fragments_list(&map1);
    release_fragments_list(&map2);
    return 1;
}

static int dump_terminator(void *user_defined_data)
{
    udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;

    return jp->termination_router((void *)jp);
}

/************************************************************/
/*                    move_file routine                     */
/************************************************************/

/**
 * @internal
 * @brief File moving results
 * intended for use in move_file only.
 */
typedef enum {
    CALCULATED_MOVING_SUCCESS,          /* file has been moved successfully, but its new block map is just calculated */
    DETERMINED_MOVING_FAILURE,          /* nothing has been moved; a real new block map is available */
    DETERMINED_MOVING_PARTIAL_SUCCESS,  /* file has been moved partially; a real new block map is available */
    DETERMINED_MOVING_SUCCESS           /* file has been moved entirely; a real new block map is available */
} ud_file_moving_result;

/**
 * @brief Moves a cluster chain of the file.
 * @details Can move any part of any file.
 * @param[in] f pointer to structure describing the file to be moved.
 * @param[in] vcn the VCN of the first cluster to be moved.
 * @param[in] length the length of the cluster chain to be moved.
 * @param[in] target the LCN of the target free region.
 * @param[in] jp job parameters.
 * @return Zero for success, negative value otherwise.
 * @note 
 * - This routine cannot move the first fragment of MFT
 * on NTFS as well as first clusters of FAT directories.
 * - Volume must be opened before this call,
 * jp->fVolume must contain a proper handle.
 * - If this function returns negative value indicating failure, 
 * one of the flags listed in udefrag_internals.h under "file status flags"
 * becomes set to display a proper message in fragmentation reports.
 */
int move_file(winx_file_info *f,
              ULONGLONG vcn,
              ULONGLONG length,
              ULONGLONG target,
              udefrag_job_parameters *jp
              )
{
    ULONGLONG time;
    NTSTATUS Status;
    HANDLE hFile;
    int old_color, new_color;
    int was_fragmented, became_fragmented;
    int was_excluded;
    int dump_result;
    winx_blockmap *block, *first_block;
    ULONGLONG clusters_to_redraw;
    ULONGLONG curr_vcn, lcn, n;
    winx_file_info desired_file_info;
    winx_file_info new_file_info;
    ud_file_moving_result moving_result;
    int r1, r2, r3;
    
    time = winx_xtime();
    jp->last_move_status = 0;
    
    /* validate parameters */
    if(f == NULL || jp == NULL){
        DebugPrint("move_file: invalid parameter");
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }
    
    if(jp->udo.dbgprint_level >= DBG_DETAILED){
        if(f->path) DebugPrint("%ws",f->path);
        else DebugPrint("empty filename");
        DebugPrint("vcn = %I64u, length = %I64u, target = %I64u",vcn,length,target);
    }
    
    if(length == 0){
        DebugPrint("move_file: move of zero number of clusters requested");
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return 0; /* nothing to move */
    }
    
    if(f->disp.clusters == 0 || f->disp.fragments == 0 || f->disp.blockmap == NULL){
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return 0; /* nothing to move */
    }
    
    if(vcn + length > f->disp.blockmap->prev->vcn + f->disp.blockmap->prev->length){
        DebugPrint("move_file: data move behind the end of the file requested");
        DbgPrintBlocksOfFile(f->disp.blockmap);
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }
    
    first_block = get_first_block_of_cluster_chain(f,vcn);
    if(first_block == NULL){
        DebugPrint("move_file: data move out of file bounds requested");
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }
    
    if(!check_region(jp,target,length)){
        DebugPrint("move_file: there is no sufficient free space available on target block");
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }
    
    /* save file properties */
    old_color = get_file_color(jp,f);
    was_fragmented = is_fragmented(f);
    was_excluded = is_excluded(f);

    /* open the file */
    Status = winx_defrag_fopen(f,WINX_OPEN_FOR_MOVE,&hFile);
    if(Status != STATUS_SUCCESS){
        DebugPrintEx(Status,"move_file: cannot open %ws",f->path);
        /* redraw space */
        colorize_file_as_system(jp,f);
        /* however, don't reset file information here */
        f->user_defined_flags |= UD_FILE_LOCKED;
        /*jp->pi.processed_clusters += length;*/
        if(jp->progress_router)
            jp->progress_router(jp); /* redraw progress */
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }
    
    /* move the file */
    move_file_helper(hFile,f,vcn,length,target,jp);
    winx_defrag_fclose(hFile);
    
    /* get file moving result */
    calculate_file_disposition(f,vcn,length,target,&desired_file_info);
    if(jp->udo.dry_run){
        dump_result = -1;
    } else {
        memcpy(&new_file_info,f,sizeof(winx_file_info));
        new_file_info.disp.blockmap = NULL;
        dump_result = winx_ftw_dump_file(&new_file_info,dump_terminator,(void *)jp);
        if(dump_result < 0)
            DebugPrint("move_file: cannot redump the file");
    }
    
    if(dump_result < 0){
        /* let's assume a successful move */
        /* we have no new map of file blocks, so let's use calculated one */
        memcpy(&new_file_info,&desired_file_info,sizeof(winx_file_info));
        moving_result = CALCULATED_MOVING_SUCCESS;
    } else {
        /*DebugPrint("OLD MAP:");
        for(block = f->disp.blockmap; block; block = block->next){
            DebugPrint("VCN = %I64u, LCN = %I64u, LEN = %I64u",
                block->vcn, block->lcn, block->length);
            if(block->next == f->disp.blockmap) break;
        }
        DebugPrint("NEW MAP:");
        for(block = new_file_info.disp.blockmap; block; block = block->next){
            DebugPrint("VCN = %I64u, LCN = %I64u, LEN = %I64u",
                block->vcn, block->lcn, block->length);
            if(block->next == new_file_info.disp.blockmap) break;
        }*/
        /* compare file dispositions */
        if(compare_file_dispositions(&new_file_info,&desired_file_info) == 0){
            moving_result = DETERMINED_MOVING_SUCCESS;
        } else {
            if(compare_file_dispositions(&new_file_info,f) == 0){
                DebugPrint("move_file: nothing has been moved");
                moving_result = DETERMINED_MOVING_FAILURE;
            } else {
                DebugPrint("move_file: new file disposition differs from desired one");
                DbgPrintBlocksOfFile(new_file_info.disp.blockmap);
                moving_result = DETERMINED_MOVING_PARTIAL_SUCCESS;
            }
        }
        /* release calculated desired disposition */
        winx_list_destroy((list_entry **)(void *)&desired_file_info.disp.blockmap);
    }
    
    /* handle a case when nothing has been moved */
    if(moving_result == DETERMINED_MOVING_FAILURE){
        winx_list_destroy((list_entry **)(void *)&new_file_info.disp.blockmap);
        f->user_defined_flags |= UD_FILE_MOVING_FAILED;
        /* remove target space from the free space pool */
        jp->free_regions = winx_sub_volume_region(jp->free_regions,target,length);
        jp->p_counters.moving_time += winx_xtime() - time;
        return (-1);
    }

    /*
    * Something has been moved, therefore we need to redraw 
    * space, update free space pool and adjust statistics.
    */
    if(moving_result == DETERMINED_MOVING_PARTIAL_SUCCESS)
        f->user_defined_flags |= UD_FILE_MOVING_FAILED;
    
    /* reapply filters to the file */
    f->user_defined_flags &= ~UD_FILE_EXCLUDED;
    new_file_info.user_defined_flags &= ~UD_FILE_EXCLUDED;
    r1 = exclude_by_fragment_size(&new_file_info,jp);
    r2 = exclude_by_fragments(&new_file_info,jp);
    r3 = exclude_by_size(&new_file_info,jp);
    if(r1 || r2 || r3){
        f->user_defined_flags |= UD_FILE_EXCLUDED;
        new_file_info.user_defined_flags |= UD_FILE_EXCLUDED;
    }

    /* redraw target space */
    new_color = get_file_color(jp,&new_file_info);
    colorize_map_region(jp,target,length,new_color,FREE_SPACE);
    if(jp->progress_router)
        jp->progress_router(jp); /* redraw map */
            
    /* remove target space from the free space pool - before the following map redraw */
    jp->free_regions = winx_sub_volume_region(jp->free_regions,target,length);
    
    /* redraw file clusters in new color */
    if(new_color != old_color){
        for(block = f->disp.blockmap; block; block = block->next){
            colorize_map_region(jp,block->lcn,block->length,new_color,old_color);
            if(block->next == f->disp.blockmap) break;
        }
    }
    
    /* redraw freed range of clusters */
    if(moving_result != DETERMINED_MOVING_PARTIAL_SUCCESS){
        clusters_to_redraw = length;
        curr_vcn = vcn;
        first_block = get_first_block_of_cluster_chain(f,vcn);
        for(block = first_block; block; block = block->next){
            /* redraw the current block or its part */
            lcn = block->lcn + (curr_vcn - block->vcn);
            n = min(block->length - (curr_vcn - block->vcn),clusters_to_redraw);
            colorize_map_region(jp,lcn,n,FREE_SPACE,new_color);
            clusters_to_redraw -= n;
            if(jp->fs_type != FS_NTFS || jp->udo.dry_run){
                /* on NTFS we cannot use freed space until
                   release_temp_space_regions call because
                   Windows marks clusters as temporarily
                   allocated immediately after the move
                */
                jp->free_regions = winx_add_volume_region(jp->free_regions,lcn,n);
            }
            if(!clusters_to_redraw || block->next == f->disp.blockmap) break;
            curr_vcn = block->next->vcn;
        }
    }

    /* adjust statistics */
    became_fragmented = is_fragmented(&new_file_info);
    if(became_fragmented && !is_excluded(f)){
        if(!was_fragmented || was_excluded){
            jp->pi.fragmented ++;
            jp->pi.fragments += (new_file_info.disp.fragments - 1);
        } else {
            jp->pi.fragments -= (f->disp.fragments - new_file_info.disp.fragments);
        }
    }
    if(!became_fragmented || is_excluded(f)){
        if(was_fragmented && !was_excluded){
            jp->pi.fragmented --;
            jp->pi.fragments -= (f->disp.fragments - 1);
        }
    }
    if(jp->progress_router)
        jp->progress_router(jp); /* redraw map and update statistics */

    /* new block map is available - use it */
    for(block = f->disp.blockmap; block; block = block->next){
        /* all blocks must be removed! */
        (void)remove_block_from_file_blocks_tree(jp,block);
        if(block->next == f->disp.blockmap) break;
    }
    winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
    memcpy(&f->disp,&new_file_info.disp,sizeof(winx_file_disposition));
    for(block = f->disp.blockmap; block; block = block->next){
        if(add_block_to_file_blocks_tree(jp,f,block) < 0) break;
        if(block->next == f->disp.blockmap) break;
    }

    /* update list of fragmented files */
    truncate_fragmented_files_list(f,jp);
    if(is_fragmented(f) && !is_excluded(f))
        expand_fragmented_files_list(f,jp);

    jp->p_counters.moving_time += winx_xtime() - time;
    return (moving_result == DETERMINED_MOVING_PARTIAL_SUCCESS) ? (-1) : 0;
}

/** @} */
