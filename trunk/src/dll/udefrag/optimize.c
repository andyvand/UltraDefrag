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
 * @file optimize.c
 * @brief Volume optimization.
 * @addtogroup Optimizer
 * @{
 */

#include "udefrag-internals.h"

static void calculate_free_rgn_size_threshold(udefrag_job_parameters *jp);
static int increase_starting_point(udefrag_job_parameters *jp, ULONGLONG *sp);

/************************************************************/
/*                   Auxiliary routines                     */
/************************************************************/

/**
 * @brief Optimizes the file by placing all its
 * fragments as close as possible after the first
 * one.
 * @details Intended to optimize MFT on NTFS-formatted
 * volumes and to optimize directories on FAT. In both
 * cases the first clusters are not moveable, therefore
 * regular defragmentation cannot help.
 * @note
 * - As a side effect this routine may increase
 * number of fragmented files (they become marked
 * by UD_FILE_FRAGMENTED_BY_FILE_OPT flag). 
 * - The volume must be opened before this call,
 * jp->fVolume must contain a proper handle.
 * @return Zero if the file needs no optimization, 
 * positive value on success, negative value otherwise.
 */
static int optimize_file(winx_file_info *f,udefrag_job_parameters *jp)
{
    ULONGLONG clusters_to_process; /* the number of file clusters not processed yet */
    ULONGLONG start_vcn;           /* VCN of the portion of the file not processed yet */
    
    ULONGLONG start_lcn;           /* address of space not processed yet */
    ULONGLONG clusters_to_move;    /* the number of the file clusters intended for the current move */
    winx_volume_region *rgn, *target_rgn;
    winx_file_info *first_file;
    winx_blockmap *block, *first_block;
    ULONGLONG end_lcn, min_lcn, next_vcn;
    ULONGLONG current_vcn, remaining_clusters, n, lcn;
    winx_volume_region region = {0};
    ULONGLONG clusters_to_cleanup;
    ULONGLONG target;
    int block_cleaned_up;
    ULONGLONG tm;
    
    /* check whether the file needs optimization or not */
    if(!can_move(f) || !is_fragmented(f))
        return 0;
    
    /* check whether the file is locked or not */
    if(is_file_locked(f,jp))
        return (-1);

    /* reset counters */
    jp->pi.total_moves = 0;
    
    clusters_to_process = f->disp.clusters - f->disp.blockmap->length;
    if(clusters_to_process == 0)
        return 0;
    
    start_lcn = f->disp.blockmap->lcn + f->disp.blockmap->length;
    start_vcn = f->disp.blockmap->next->vcn;
    while(clusters_to_process > 0){
        if(jp->termination_router((void *)jp)) break;
        if(jp->free_regions == NULL) break;

        /* release temporary allocated space */
        release_temp_space_regions(jp);
        
        /* search for the first free region after start_lcn */
        target_rgn = NULL; tm = winx_xtime();
        for(rgn = jp->free_regions; rgn; rgn = rgn->next){
            if(rgn->lcn >= start_lcn && rgn->length){
                target_rgn = rgn;
                break;
            }
            if(rgn->next == jp->free_regions) break;
        }
        jp->p_counters.searching_time += winx_xtime() - tm;
        
        /* process file blocks between start_lcn and target_rgn */
        if(target_rgn) end_lcn = target_rgn->lcn;
        else end_lcn = jp->v_info.total_clusters;
        clusters_to_cleanup = clusters_to_process;
        block_cleaned_up = 0;
        region.length = 0;
        while(clusters_to_cleanup > 0){
            if(jp->termination_router((void *)jp)) goto done;
            min_lcn = start_lcn;
            first_block = find_first_block(jp, &min_lcn, MOVE_ALL, 0, &first_file);
            if(first_block == NULL) break;
            if(first_block->lcn >= end_lcn) break;
            
            /* does the first block follow a previously moved one? */
            if(block_cleaned_up){
                if(first_block->lcn != region.lcn + region.length)
                    break;
                if(first_file == f)
                    break;
            }
            
            /* don't move already optimized parts of the file */
            if(first_file == f && first_block->vcn == start_vcn){
                if(clusters_to_process <= first_block->length \
                  || first_block->next == first_file->disp.blockmap){
                    clusters_to_process = 0;
                    goto done;
                } else {
                    clusters_to_process -= first_block->length;
                    start_vcn = first_block->next->vcn;
                    start_lcn = first_block->lcn + first_block->length;
                    continue;
                }
            }
            
            /* cleanup space */
            if(jp->free_regions == NULL) goto done;
            lcn = first_block->lcn;
            current_vcn = first_block->vcn;
            clusters_to_move = remaining_clusters = min(clusters_to_cleanup, first_block->length);
            for(rgn = jp->free_regions->prev; rgn && remaining_clusters; rgn = jp->free_regions->prev){
                if(rgn->length > 0){
                    n = min(rgn->length,remaining_clusters);
                    target = rgn->lcn + rgn->length - n;
                    if(first_file != f)
                        first_file->user_defined_flags |= UD_FILE_FRAGMENTED_BY_FILE_OPT;
                    if(move_file(first_file,current_vcn,n,target,0,jp) < 0){
                        if(!block_cleaned_up)
                            goto done;
                        else
                            goto move_the_file;
                    }
                    current_vcn += n;
                    remaining_clusters -= n;
                } else {
                    /* infinite loop */
                }
                if(jp->free_regions == NULL){
                    if(remaining_clusters)
                        goto done;
                    else
                        break;
                }
            }
            /* space cleaned up successfully */
            region.next = region.prev = &region;
            if(!block_cleaned_up)
                region.lcn = lcn;
            region.length += clusters_to_move;
            target_rgn = &region;
            start_lcn = region.lcn + region.length;
            clusters_to_cleanup -= clusters_to_move;
            block_cleaned_up = 1;
        }
    
move_the_file:        
        /* release temporary allocated space ! */
        release_temp_space_regions(jp);
        
        /* target_rgn points to the target free region, so let's move the next portion of the file */
        if(target_rgn == NULL) break;
        n = clusters_to_move = min(clusters_to_process,target_rgn->length);
        next_vcn = 0; current_vcn = start_vcn;
        for(block = f->disp.blockmap; block && n; block = block->next){
            if(block->vcn + block->length > start_vcn){
                if(n > block->length - (current_vcn - block->vcn)){
                    n -= block->length - (current_vcn - block->vcn);
                    current_vcn = block->next->vcn;
                } else {
                    if(n == block->length - (current_vcn - block->vcn)){
                        if(block->next == f->disp.blockmap)
                            next_vcn = block->vcn + block->length;
                        else
                            next_vcn = block->next->vcn;
                    } else {
                        next_vcn = current_vcn + n;
                    }
                    break;
                }
            }
            if(block->next == f->disp.blockmap) break;
        }
        if(next_vcn == 0){
            DebugPrint("optimize_file: next_vcn calculation failed for %ws",f->path);
            break;
        }
        target = target_rgn->lcn;
        if(move_file(f,start_vcn,clusters_to_move,target,0,jp) < 0){
            /* on failures exit */
            break;
        }
        /* file's part moved successfully */
        clusters_to_process -= clusters_to_move;
        start_lcn = target + clusters_to_move;
        start_vcn = next_vcn;
        jp->pi.total_moves ++;
    }

done:
    if(jp->termination_router((void *)jp)) return 1;
    return (clusters_to_process > 0) ? (-1) : 1;
}

/**
 * @brief Calculates number of clusters
 * needed to be moved to complete the
 * optimization of directories.
 */
static ULONGLONG opt_dirs_cc_routine(udefrag_job_parameters *jp)
{
    udefrag_fragmented_file *f;
    ULONGLONG n = 0;
    
    for(f = jp->fragmented_files; f; f = f->next){
        if(jp->termination_router((void *)jp)) break;
        if(is_directory(f->f) && can_move(f->f))
            n += f->f->disp.clusters * 2;
        if(f->next == jp->fragmented_files) break;
    }
    return n;
}

/**
 * @brief Optimizes directories by placing their
 * fragments after the first ones as close as possible.
 * @details Intended for use on FAT-formatted volumes.
 * @return Zero for success, negative value otherwise.
 */
static int optimize_directories(udefrag_job_parameters *jp)
{
    udefrag_fragmented_file *f, *head, *next;
    winx_file_info *file;
    ULONGLONG optimized_dirs;
    char buffer[32];
    ULONGLONG time;

    jp->pi.current_operation = VOLUME_OPTIMIZATION;
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

    time = start_timing("directories optimization",jp);

    optimized_dirs = 0;
    for(f = jp->fragmented_files; f; f = next){
        if(jp->termination_router((void *)jp)) break;
        head = jp->fragmented_files;
        next = f->next;
        file = f->f; /* f will be destroyed by move_file */
        if(is_directory(file) && can_move(file)){
            if(optimize_file(file,jp) > 0)
                optimized_dirs ++;
        }
        file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        /* go to the next file */
        if(jp->fragmented_files == NULL) break;
        if(next == head) break;
    }
    
    /* display amount of moved data and number of optimized directories */
    DebugPrint("%I64u directories optimized",optimized_dirs);
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("directories optimization",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/**
 * @brief Sends list of $mft blocks to the debugger.
 */
static void list_mft_blocks(winx_file_info *mft_file)
{
    winx_blockmap *block;
    ULONGLONG i;

    for(block = mft_file->disp.blockmap, i = 0; block; block = block->next, i++){
        DebugPrint("mft part #%I64u start: %I64u, length: %I64u",
            i,block->lcn,block->length);
        if(block->next == mft_file->disp.blockmap) break;
    }
}

/**
 * @brief Calculates number of clusters
 * needed to be moved to complete the
 * MFT optimization.
 */
static ULONGLONG opt_mft_cc_routine(udefrag_job_parameters *jp)
{
    winx_file_info *f;
    ULONGLONG n = 0;

    /* search for $mft file */
    for(f = jp->filelist; f; f = f->next){
        if(jp->termination_router((void *)jp)) break;
        if(is_mft(f,jp)){
            n = f->disp.clusters * 2;
            break;
        }
        if(f->next == jp->filelist) break;
    }
    return n;
}

/**
 * @brief Optimizes MFT by placing its fragments
 * as close as possible after the first one.
 * @details MFT Zone follows MFT automatically. 
 * @return Zero for success, negative value otherwise.
 */
static int optimize_mft_routine(udefrag_job_parameters *jp)
{
    winx_file_info *f, *mft_file = NULL;
    ULONGLONG time;
    char buffer[32];
    int result;

    jp->pi.current_operation = VOLUME_OPTIMIZATION;
    jp->pi.moved_clusters = 0;

    /* free as much temporarily allocated space as possible */
    release_temp_space_regions(jp);

    /* no files are excluded by this task currently */
    for(f = jp->filelist; f; f = f->next){
        f->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(f->next == jp->filelist) break;
    }

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("mft optimization",jp);

    /* search for $mft file */
    for(f = jp->filelist; f; f = f->next){
        if(is_mft(f,jp)){
            mft_file = f;
            break;
        }
        if(f->next == jp->filelist) break;
    }
    
    /* do the job */
    if(mft_file == NULL){
        DebugPrint("optimize_mft_routine: cannot find $mft file");
        result = -1;
    } else {
        DebugPrint("optimize_mft: initial $mft map:");
        list_mft_blocks(mft_file);

        result = optimize_file(mft_file,jp);
        if(result > 0) result = 0;

        DebugPrint("optimize_mft: final $mft map:");
        list_mft_blocks(mft_file);
    }

    /* display amount of moved data */
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("mft optimization",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return result;
}

/**
 * @brief Auxiliary routine to sort files by path in binary tree.
 */
static int files_compare(const void *prb_a, const void *prb_b, void *prb_param)
{
    winx_file_info *a, *b;
    
    a = (winx_file_info *)prb_a;
    b = (winx_file_info *)prb_b;
    
    return _wcsicmp(a->path, b->path);
}

/**
 * @brief Calculates number of clusters
 * needed to be moved to optimize the disk.
 */
static ULONGLONG opt_cc_routine(udefrag_job_parameters *jp)
{
    winx_file_info *f;
    ULONGLONG n = 0;

    /* count files below file size threshold only */
    for(f = jp->filelist; f; f = f->next){
        if(jp->termination_router((void *)jp)) break;
        if(f->disp.clusters * jp->v_info.bytes_per_cluster \
          < jp->udo.optimizer_size_limit){
            if(can_move_entirely(f,jp) && !is_excluded(f))
                n += f->disp.clusters * 2;
        }
        if(f->next == jp->filelist) break;
    }
    return n;
}

/**
 * @brief Optimizes MFT by placing its fragments
 * as close as possible after the first one.
 * @details MFT Zone follows MFT automatically. 
 * @return Zero for success, negative value otherwise.
 */
static int optimize_routine(udefrag_job_parameters *jp)
{
    winx_file_info *f;
    struct prb_table *pt;
    struct prb_traverser t;
    ULONGLONG time;
    char buffer[32];

    jp->pi.current_operation = VOLUME_OPTIMIZATION;
    jp->pi.moved_clusters = 0;

    /* free as much temporarily allocated space as possible */
    release_temp_space_regions(jp);

    /* exclude files above file size threshold */
    for(f = jp->filelist; f; f = f->next){
        if(f->disp.clusters * jp->v_info.bytes_per_cluster \
          < jp->udo.optimizer_size_limit){
            f->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        } else {
            f->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        }
        if(f->next == jp->filelist) break;
    }

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("optimization",jp);

    /* build tree of files sorted by path */
    pt = prb_create(files_compare,NULL,NULL);
    if(pt == NULL){
        DebugPrint("optimize_routine: cannot create binary tree");
        goto done;
    }
    for(f = jp->filelist; f; f = f->next){
        if(can_move_entirely(f,jp) && !is_excluded(f)){
            if(prb_probe(pt,(void *)f) == NULL){
                DebugPrint("optimize_routine: cannot add file to the tree");
                goto done;
            }
        }
        if(f->next == jp->filelist) break;
    }
    
    /* do the job */
    prb_t_init(&t,pt);
    f = (winx_file_info *)prb_t_first(&t,pt);
    while(f){
        /* cleanup space before the file by moving
        little files and fragments to the end */
        
        /* try to move the file closer to the previous one */
        
        /* mark the file as already optimized */
        f->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        
        /* go to the next file */
        f = (winx_file_info *)prb_t_next(&t);
    }
    
done:
    /* display amount of moved data */
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("optimization",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    if(pt) prb_destroy(pt,NULL);
    return 0;
}

/************************************************************/
/*                    The entry point                       */
/************************************************************/

/**
 * @brief Performs a volume optimization.
 * @details The disk optimization sorts
 * little files (below fragment size threshold)
 * by path on the disk. On FAT it optimizes
 * directories also. On NTFS it optimizes the MFT.
 * @note
 * - The optimizer relies on the fragment size threshold
 * filter. If it isn't set, the default value of 20 Mb
 * is used.
 * - The multi-pass processing is not used here,
 * because it may easily cause repeated moves of huge
 * amounts of data on disks being in use during the
 * optimization.
 * @return Zero for success, negative value otherwise.
 */
int optimize(udefrag_job_parameters *jp)
{
    int result, overall_result = -1;
    int win_version;
    
    ULONGLONG start_lcn = 0, new_start_lcn;
    ULONGLONG remaining_clusters;

    /* perform volume analysis */
    result = analyze(jp); /* we need to call it once, here */
    if(result < 0) return result;

    /* FAT specific: optimize directories */
    win_version = winx_get_os_version();
    if(jp->is_fat && win_version > WINDOWS_2K){
        jp->pi.processed_clusters = 0;
        jp->pi.clusters_to_process = opt_dirs_cc_routine(jp);
        result = optimize_directories(jp);
        if(result == 0){
            /* at least something succeeded */
            overall_result = 0;
        }
    }
    
    /* NTFS specific: optimize MFT */
    if(jp->fs_type == FS_NTFS && win_version > WINDOWS_2K){
        jp->pi.processed_clusters = 0;
        jp->pi.clusters_to_process = opt_mft_cc_routine(jp);
        result = optimize_mft_routine(jp);
        if(result == 0){
            /* at least something succeeded */
            overall_result = 0;
        }
    }
    
    /* get rid of fragmented files */
    //defragment(jp);

    /* optimize the disk */
    if(jp->udo.optimizer_size_limit == 0){
        DebugPrint("optimize: file size threshold not set; the default value will be used");
        jp->udo.optimizer_size_limit = OPTIMIZER_MAGIC_CONSTANT;
    }
    DebugPrint("optimize: file size threshold = %I64u",
        jp->udo.optimizer_size_limit);
    jp->pi.processed_clusters = 0;
    jp->pi.clusters_to_process = opt_cc_routine(jp);
    result = optimize_routine(jp);
    if(result == 0){
        /* optimization succeeded */
        overall_result = 0;
    }
    
    /* get rid of fragmented files again */
    defragment(jp);
    return overall_result;
    
    /* perform volume analysis */
    result = analyze(jp); /* we need to call it once, here */
    if(result < 0) return result;
    
    /* set free region size threshold, reset counters */
    calculate_free_rgn_size_threshold(jp);
    jp->pi.processed_clusters = 0;
    /* we have a chance to move everything to the end and then in contrary direction */
    jp->pi.clusters_to_process = (jp->v_info.total_clusters - \
        jp->v_info.free_bytes / jp->v_info.bytes_per_cluster) * 2;
    
    /* optimize MFT separately to keep its optimal location */
    //result = optimize_mft_helper(jp);
    if(result == 0){
        /* at least mft optimization succeeded */
        overall_result = 0;
    }

    /* do the job */
    jp->pi.pass_number = 0;
    while(!jp->termination_router((void *)jp)){
        /* exclude already optimized part of the volume */
        new_start_lcn = calculate_starting_point(jp,start_lcn);
        if(0){//if(new_start_lcn <= start_lcn && jp->pi.pass_number){
            DebugPrint("volume optimization completed: old_sp = %I64u, new_sp = %I64u",
                start_lcn, new_start_lcn);
            break;
        }
        if(new_start_lcn > start_lcn)
            start_lcn = new_start_lcn;
        
        /* reset counters */
        remaining_clusters = get_number_of_movable_clusters(jp,start_lcn,jp->v_info.total_clusters,MOVE_ALL);
        jp->pi.processed_clusters = 0; /* reset counter */
        jp->pi.clusters_to_process = (jp->v_info.total_clusters - \
            jp->v_info.free_bytes / jp->v_info.bytes_per_cluster) * 2;
        jp->pi.processed_clusters = jp->pi.clusters_to_process - \
            remaining_clusters * 2; /* set counter */
                
        DebugPrint("volume optimization pass #%u, starting point = %I64u, remaining clusters = %I64u",
            jp->pi.pass_number, start_lcn, remaining_clusters);
        
        /* move fragmented files to the terminal part of the volume */
        result = move_files_to_back(jp, 0, MOVE_FRAGMENTED);
        if(result == 0){
            /* at least something succeeded */
            overall_result = 0;
        }
        
        /* break if nothing moveable exist after starting point */
        if(jp->pi.moved_clusters == 0 && remaining_clusters == 0) break;
        
        /* full opt: cleanup space after start_lcn */
        if(jp->job_type == FULL_OPTIMIZATION_JOB){
            result = move_files_to_back(jp, start_lcn, MOVE_ALL);
            if(result == 0){
                /* at least something succeeded */
                overall_result = 0;
            }
        }
        
        /* move files back to the beginning */
        result = move_files_to_front(jp, start_lcn, MOVE_ALL);
        if(result == 0){
            /* at least something succeeded */
            overall_result = 0;
        }
        
        /* move starting point to the next free region if nothing moved */
        if(result < 0 || jp->pi.moved_clusters == 0){
            if(increase_starting_point(jp, &start_lcn) < 0)
                break; /* end of disk reached */
        }
        
        /* break if no repeat */
        if(!(jp->udo.job_flags & UD_JOB_REPEAT)) break;
        
        /* go to the next pass */
        jp->pi.pass_number ++;
    }
    
    if(jp->termination_router((void *)jp)) return 0;
    
    return overall_result;
}

/**
 * @brief MFT optimizer entry point.
 */
int optimize_mft(udefrag_job_parameters *jp)
{
    int result;

    /* perform volume analysis */
    result = analyze(jp); /* we need to call it once, here */
    if(result < 0) return result;

    /* mft optimization is NTFS specific task */
    if(jp->fs_type != FS_NTFS){
        jp->pi.processed_clusters = 0;
        jp->pi.clusters_to_process = 1;
        jp->pi.current_operation = VOLUME_OPTIMIZATION;
        return 0; /* nothing to do */
    }

    /* mft optimization requires at least windows xp */
    if(winx_get_os_version() < WINDOWS_XP){
        DebugPrint("MFT is not movable on NT4 and Windows 2000");
        return UDEFRAG_UNMOVABLE_MFT;
    }

    /* reset counters */
    jp->pi.processed_clusters = 0;
    jp->pi.clusters_to_process = opt_mft_cc_routine(jp);
    
    /* do the job */
    result = optimize_mft_routine(jp);
    
    /* cleanup the disk */
    defragment(jp);
    return result;
}

/************************* Internal routines ****************************/

/**
 * @brief Calculates free region size
 * threshold used in volume optimization.
 */
static void calculate_free_rgn_size_threshold(udefrag_job_parameters *jp)
{
    winx_volume_region *rgn;
    ULONGLONG length = 0;

    if(jp->v_info.free_bytes >= jp->v_info.total_bytes / 10){
        /*
        * We have at least 10% of free space on the volume, so
        * it seems to be reasonable to put all data together
        * even if the free space is split to many little pieces.
        */
        DebugPrint("calculate_free_rgn_size_threshold: strategy #1 because of at least 10%% of free space on the volume");
        for(rgn = jp->free_regions; rgn; rgn = rgn->next){
            if(rgn->length > length)
                length = rgn->length;
            if(rgn->next == jp->free_regions) break;
        }
        /* Threshold = 0.5% of the volume or a half of the largest free space region. */
        jp->free_rgn_size_threshold = min(jp->v_info.total_clusters / 200, length / 2);
    } else {
        /*
        * On volumes with less than 10% of free space
        * we're searching for the free space region
        * at least 0.5% long.
        */
        DebugPrint("calculate_free_rgn_size_threshold: strategy #2 because of less than 10%% of free space on the volume");
        jp->free_rgn_size_threshold = jp->v_info.total_clusters / 200;
    }
    //jp->free_rgn_size_threshold >>= 1;
    if(jp->free_rgn_size_threshold < 2) jp->free_rgn_size_threshold = 2;
    DebugPrint("free region size threshold = %I64u clusters",jp->free_rgn_size_threshold);
}

/**
 * @brief Calculates starting point
 * for the volume optimization process
 * to skip already optimized data.
 * All the clusters before it will
 * be skipped in the move_files_to_back
 * routine.
 */
ULONGLONG calculate_starting_point(udefrag_job_parameters *jp, ULONGLONG old_sp)
{
    ULONGLONG new_sp;
    ULONGLONG fragmented, free, lim, i, max_new_sp;
    winx_volume_region *rgn;
    udefrag_fragmented_file *f;    
    winx_blockmap *block;
    ULONGLONG time;

    /* free temporarily allocated space */
    release_temp_space_regions_internal(jp);

    /* search for the first large free space gap after an old starting point */
    new_sp = old_sp; time = winx_xtime();
    for(rgn = jp->free_regions; rgn; rgn = rgn->next){
        if(jp->udo.dbgprint_level >= DBG_PARANOID)
            DebugPrint("Free block start: %I64u len: %I64u",rgn->lcn,rgn->length);
        if(rgn->lcn >= old_sp && rgn->length >= jp->free_rgn_size_threshold){
            new_sp = rgn->lcn;
            break;
        }
        if(rgn->next == jp->free_regions) break;
    }
    jp->p_counters.searching_time += winx_xtime() - time;
    
    /* move starting point back to release heavily fragmented data */
    /* allow no more than 5% of fragmented data inside of a skipped part of the disk */
    fragmented = get_number_of_fragmented_clusters(jp,old_sp,new_sp);
    if(fragmented > (new_sp - old_sp) / 20){
        /* based on bsearch() algorithm from ReactOS */
        i = old_sp;
        for(lim = new_sp - old_sp + 1; lim; lim >>= 1){
            new_sp = i + (lim >> 1);
            fragmented = get_number_of_fragmented_clusters(jp,old_sp,new_sp);
            if(fragmented > (new_sp - old_sp) / 20){
                /* move left */
            } else {
                /* move right */
                i = new_sp + 1; lim --;
            }
        }
    }
    if(new_sp == old_sp)
        return old_sp;
    
    //DebugPrint("*** 1 old = %I64u, new = %I64u ***",old_sp,new_sp);
    /* cut off heavily fragmented free space */
    i = old_sp; max_new_sp = new_sp;
    for(lim = new_sp - old_sp + 1; lim; lim >>= 1){
        new_sp = i + (lim >> 1);
        free = get_number_of_free_clusters(jp,new_sp,max_new_sp);
        if(free > (max_new_sp - new_sp) / 3){
            /* move left */
            //max_new_sp = new_sp;
        } else {
            /* move right */
            i = new_sp + 1; lim --;
        }
    }
    if(new_sp == old_sp)
        return old_sp;
    //DebugPrint("*** 2 old = %I64u, new = %I64u ***",old_sp,new_sp);
    
    /* is starting point inside a fragmented file block? */
    time = winx_xtime();
    for(f = jp->fragmented_files; f; f = f->next){
        for(block = f->f->disp.blockmap; block; block = block->next){
            if(new_sp >= block->lcn && new_sp <= block->lcn + block->length - 1){
                if(can_move(f->f) && !is_mft(f->f,jp) && !is_file_locked(f->f,jp)){
                    /* include block */
                    jp->p_counters.searching_time += winx_xtime() - time;
                    return block->lcn;
                } else {
                    goto skip_unmovable_files;
                }
            }
            if(block->next == f->f->disp.blockmap) break;
        }
        if(f->next == jp->fragmented_files) break;
    }
    jp->p_counters.searching_time += winx_xtime() - time;

skip_unmovable_files:    
    /* skip not movable contents */
    for(rgn = jp->free_regions; rgn; rgn = rgn->next){
        if(rgn->lcn > new_sp){
            if(get_number_of_movable_clusters(jp,new_sp,rgn->lcn,MOVE_ALL) != 0){
                break;
            } else {
                new_sp = rgn->lcn;
            }
        }
        if(rgn->next == jp->free_regions) break;
    }
    return new_sp;
}

/**
 * @brief Moves starting point to the next free region.
 * @return Zero for success, negative value otherwise.
 */
static int increase_starting_point(udefrag_job_parameters *jp, ULONGLONG *sp)
{
    ULONGLONG new_sp;
    winx_volume_region *rgn;
    ULONGLONG time;
    
    if(sp == NULL)
        return (-1);
    
    new_sp = calculate_starting_point(jp,*sp);

    /* go to the first large free space gap after a new starting point */
    time = winx_xtime();
    for(rgn = jp->free_regions; rgn; rgn = rgn->next){
        if(rgn->lcn > new_sp && rgn->length >= jp->free_rgn_size_threshold){
            *sp = rgn->lcn;
            jp->p_counters.searching_time += winx_xtime() - time;
            return 0;
        }
        if(rgn->next == jp->free_regions) break;
    }
    /* end of disk reached */
    jp->p_counters.searching_time += winx_xtime() - time;
    return (-1);
}

/** @} */
