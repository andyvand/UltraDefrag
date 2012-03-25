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
 * @file optimize.c
 * @brief Volume optimization.
 * @addtogroup Optimizer
 * @{
 */

/*
* Ideas by Dmitri Arkhangelski <dmitriar@gmail.com>
* and Stefan Pendl <stefanpe@users.sourceforge.net>.
*/

#include "udefrag-internals.h"

/************************************************************/
/*                   Auxiliary routines                     */
/************************************************************/

/**
 * @brief Cleans the space up by moving
 * specified number of clusters starting
 * at specified file block's beginning
 * to the free space areas outside of
 * the region intended to be cleaned up.
 * @param[in] jp the job parameters.
 * @param[in] file the file to be moved.
 * @param[in] block the block to be moved.
 * @param[in] clusters_to_cleanup the number
 * of clusters to be cleaned up.
 * @param[in] reserved_start_lcn the first
 * LCN of the space intended to be cleaned up.
 * @param[in] reserved_end_lcn the last
 * LCN of the space intended to be cleaned up.
 * @return Zero indicates success. (-1)
 * indicates that not enough free space
 * exist on the disk. (-2) indicates that
 * the file moving failed.
 */
static int cleanup_space(udefrag_job_parameters *jp, winx_file_info *file,
                         winx_blockmap *block, ULONGLONG clusters_to_cleanup,
                         ULONGLONG reserved_start_lcn, ULONGLONG reserved_end_lcn)
{
    ULONGLONG current_vcn, target, n;
    winx_volume_region *r, *rgn;
    
    if(clusters_to_cleanup == 0) return 0;
    if(file == NULL || block == NULL) return 0;
    
    current_vcn = block->vcn;
    while(clusters_to_cleanup){
        /* use last free region */
        if(jp->free_regions == NULL) return (-1);
        rgn = NULL;
        for(r = jp->free_regions->prev; r; r = r->prev){
            if(r->length > 0 && \
              (r->lcn > reserved_end_lcn || \
              r->lcn + r->length <= reserved_start_lcn)){
                rgn = r;
                break;
            }
            if(r->prev == jp->free_regions->prev) break;
        }
        if(rgn == NULL) return (-1);
        
        n = min(rgn->length,clusters_to_cleanup);
        target = rgn->lcn + rgn->length - n;
        if(move_file(file,current_vcn,n,target,jp) < 0)
            return (-2);
        current_vcn += n;
        clusters_to_cleanup -= n;
    }
    return 0;
}

/**
 * @brief Advances VCN by the specified number of clusters.
 * @return Advanced VCN; may point beyond the file.
 */
static ULONGLONG advance_vcn(winx_file_info *f,ULONGLONG vcn,ULONGLONG n)
{
    ULONGLONG current_vcn;
    winx_blockmap *block;
    
    if(n == 0)
        return vcn;
    
    current_vcn = vcn;
    for(block = f->disp.blockmap; block && n; block = block->next){
        if(block->vcn + block->length > vcn){
            if(n > block->length - (current_vcn - block->vcn)){
                n -= block->length - (current_vcn - block->vcn);
                current_vcn = block->next->vcn;
            } else {
                if(n == block->length - (current_vcn - block->vcn)){
                    if(block->next == f->disp.blockmap)
                        return block->vcn + block->length;
                    else
                        return block->next->vcn;
                } else {
                    return current_vcn + n;
                }
            }
        }
        if(block->next == f->disp.blockmap) break;
    }
    DebugPrint("advance_vcn: vcn calculation failed for %ws",f->path);
    return 0;
}

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
    ULONGLONG first_cluster;       /* LCN of the first cluster of the file */
    ULONGLONG start_lcn;           /* address of space not processed yet */
    ULONGLONG clusters_to_move;    /* the number of the file clusters intended for the current move */
    winx_volume_region *rgn, *target_rgn;
    winx_file_info *first_file;
    winx_blockmap *first_block;
    ULONGLONG end_lcn, min_lcn, next_vcn;
    ULONGLONG lcn;
    winx_volume_region region = {0};
    ULONGLONG clusters_to_cleanup;
    ULONGLONG target;
    int result;
    int block_cleaned_up;
    ULONGLONG tm;
    
    /* check whether the file needs optimization or not */
    if(!can_move(f,jp) || !is_fragmented(f))
        return 0;
    
    /* check whether the file is locked or not */
    if(is_file_locked(f,jp))
        return (-1);

    /* reset counters */
    jp->pi.total_moves = 0;
    
    clusters_to_process = f->disp.clusters - f->disp.blockmap->length;
    if(clusters_to_process == 0)
        return 0;
    
    first_cluster = f->disp.blockmap->lcn;
    start_lcn = f->disp.blockmap->lcn + f->disp.blockmap->length;
    start_vcn = f->disp.blockmap->next->vcn;
    
try_again:
    while(clusters_to_process > 0){
        if(jp->termination_router((void *)jp)) break;

        /* release temporarily allocated space */
        release_temp_space_regions(jp);
        if(jp->free_regions == NULL) break;
        
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
            first_block = find_first_block(jp, &min_lcn, 0, &first_file);
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
                    clusters_to_cleanup -= first_block->length;
                    start_vcn = first_block->next->vcn;
                    start_lcn = first_block->lcn + first_block->length;
                    continue;
                }
            }
            
            /* cleanup space */
            lcn = first_block->lcn;
            clusters_to_move = min(clusters_to_cleanup, first_block->length);
            result = cleanup_space(jp,first_file,first_block,
                clusters_to_move,first_cluster,
                first_block->lcn + first_block->length - 1);
            if(result == -1) goto done;
          
            if(first_file != f){
                first_file->user_defined_flags |= UD_FILE_FRAGMENTED_BY_FILE_OPT;
            }
            
            if(result == -2){
                if(!block_cleaned_up){
                    start_lcn = lcn + clusters_to_move;
                    goto try_again;
                } else {
                    goto move_the_file;
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
        /* target_rgn points to the target free region, so let's move the next portion of the file */
        if(target_rgn == NULL) break;
        clusters_to_move = min(clusters_to_process,target_rgn->length);
        next_vcn = advance_vcn(f,start_vcn,clusters_to_move);
        target = target_rgn->lcn;
        if(move_file(f,start_vcn,clusters_to_move,target,jp) < 0){
            if(jp->last_move_status != STATUS_ALREADY_COMMITTED){
                /* on unrecoverable failures exit */
                break;
            }
            /* go forward and try to cleanup next blocks */
            f->user_defined_flags &= ~UD_FILE_MOVING_FAILED;
            start_lcn = target + clusters_to_move;
            continue;
        }
        /* file's part moved successfully */
        clusters_to_process -= clusters_to_move;
        start_lcn = target + clusters_to_move;
        start_vcn = next_vcn;
        jp->pi.total_moves ++;
        if(next_vcn == 0) break;
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
        if(is_directory(f->f) && can_move(f->f,jp))
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

    /* exclude not fragmented FAT directories only */
    for(file = jp->filelist; file; file = file->next){
        file->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(jp->is_fat && is_directory(file) && !is_fragmented(file))
            file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        if(file->next == jp->filelist) break;
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
        if(is_directory(file) && can_move(file,jp)){
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

        (void)optimize_file(mft_file,jp);
        result = 0;

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
 * @brief Moves small files
 * to the beginning of the disk,
 * sorted by path.
 * @param[in] jp the job parameters.
 * @param[in,out] start_lcn LCN of
 * the space not optimized yet.
 * @param[in] end_lcn LCN of the space
 * beyond the area intended for placement
 * of sorted out files.
 * @param[in] t pointer to the binary tree
 * traverser.
 */
static void move_files_to_front(udefrag_job_parameters *jp,
    ULONGLONG *start_lcn, ULONGLONG end_lcn, struct prb_traverser *t)
{
    winx_file_info *file;
    winx_volume_region *rgn;
    int region_not_found;
    ULONGLONG skipped_files = 0;
    ULONGLONG lcn;
    ULONGLONG time;
    char buffer[32];
    
    time = start_timing("file moving to front",jp);
    jp->pi.moved_clusters = 0;
    jp->pi.total_moves = 0;
    /* release temporarily allocated space */
    release_temp_space_regions(jp);

    /* do the job */
    file = prb_t_cur(t);
    while(file){
        if(can_move_entirely(file,jp)){
            region_not_found = 1;
            rgn = find_first_free_region(jp,*start_lcn,file->disp.clusters);
            if(rgn){
                if(rgn->lcn < end_lcn)
                    region_not_found = 0;
            }
            if(region_not_found){
                if(file->user_defined_flags & UD_FILE_REGION_NOT_FOUND){
                    /* if region is not found twice, skip the file */
                    file = prb_t_next(t);
                    skipped_files ++;
                    continue;
                } else {
                    if(skipped_files && !jp->pi.moved_clusters){
                        /* skip all subsequent big files too */
                        file = prb_t_next(t);
                        skipped_files ++;
                        continue;
                    } else {
                        file->user_defined_flags |= UD_FILE_REGION_NOT_FOUND;
                        break;
                    }
                }
            }
            /* move the file */
            lcn = rgn->lcn;
            if(move_file(file,file->disp.blockmap->vcn,
              file->disp.clusters,rgn->lcn,jp) >= 0){
                jp->pi.total_moves ++;
                if(file->disp.clusters * jp->v_info.bytes_per_cluster \
                  < OPTIMIZER_MAGIC_CONSTANT){
                    *start_lcn = lcn + 1;
                }
            }
            file->user_defined_flags |= UD_FILE_MOVED_TO_FRONT;
        }
        file = prb_t_next(t);
    }
    
    /* display amount of moved data */
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("file moving to front",time,jp);
}

/**
 * @brief Defines whether the file block
 * deserves to be moved to the end
 * of the disk or not in the move_files_to_back
 * routine.
 * @note Optimized for speed.
 */
static int is_block_quite_small(udefrag_job_parameters *jp,
    winx_file_info *file,winx_blockmap *block)
{
    ULONGLONG file_size, block_size;
    ULONGLONG fragment_size;
    winx_blockmap *fragments, *fr;
    
    file_size = file->disp.clusters * jp->v_info.bytes_per_cluster;
    block_size = block->length * jp->v_info.bytes_per_cluster;

    /* move everything which need to be sorted out */
    if(file_size < jp->udo.optimizer_size_limit) return 1;
    
    /* skip big not fragmented files */
    if(!is_fragmented(file)) return 0;
    
    /* move everything fragmented if the fragment size threshold isn't set */
    if(jp->udo.fragment_size_threshold == DEFAULT_FRAGMENT_SIZE_THRESHOLD) return 1;
    
    /* skip fragments bigger than the fragment size threshold */
    if(block_size >= jp->udo.fragment_size_threshold) return 0;

    /* move small fragments needing defragmentation */
    fragments = build_fragments_list(file,NULL);
    for(fr = fragments; fr; fr = fr->next){
        if(block->lcn >= fr->lcn && block->lcn < fr->lcn + fr->length){
            fragment_size = fr->length * jp->v_info.bytes_per_cluster;
            if(fragment_size >= jp->udo.fragment_size_threshold){
                release_fragments_list(&fragments);
                return 0;
            }
            break;
        }
        if(fr->next == fragments) break;
    }
    release_fragments_list(&fragments);
    return 1;
}

/**
 * @brief Cleans up the beginning
 * of the disk by moving small files
 * and fragments to the end.
 * @param[in] jp the job parameters.
 * @param[in,out] start_lcn LCN of
 * the space not optimized yet.
 */
static void move_files_to_back(udefrag_job_parameters *jp,ULONGLONG *start_lcn)
{
    winx_file_info *first_file;
    winx_blockmap *first_block;
    ULONGLONG lcn, min_lcn;
    int move_block = 0;
    int result;
    ULONGLONG time;
    char buffer[32];
    
    time = start_timing("file moving to end",jp);
    jp->pi.moved_clusters = 0;
    jp->pi.total_moves = 0;
    /* release temporarily allocated space */
    release_temp_space_regions(jp);

    /* do the job */
    min_lcn = *start_lcn;
    while(!jp->termination_router((void *)jp)){
        first_block = find_first_block(jp, &min_lcn,
            SKIP_PARTIALLY_MOVABLE_FILES, &first_file);
        if(first_block == NULL) break;
        move_block = is_block_quite_small(jp,first_file,first_block);
        if(move_block){
            lcn = first_block->lcn;
            result = cleanup_space(jp, first_file,
                first_block, first_block->length,
                0, first_block->lcn + first_block->length - 1);
            if(result == 0)
                jp->pi.total_moves ++;
            if(result == -1){
                /* no more free space beyond exist */
                *start_lcn = lcn;
                goto done;
            }
        }
    }
    *start_lcn = jp->v_info.total_clusters;

done:
    /* display amount of moved data */
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("file moving to end",time,jp);
}

/**
 * @brief Marks group of files
 * as already optimized.
 */
static void cut_off_group_of_files(udefrag_job_parameters *jp,
    struct prb_table *pt,winx_file_info *first_file,ULONGLONG n,
    ULONGLONG length)
{
    struct prb_traverser t;
    winx_file_info *file;
    
    /* group should be larger than 20 Mb or should contain at least 10 files */
    if(length * jp->v_info.bytes_per_cluster < OPTIMIZER_MAGIC_CONSTANT){
        if(n < OPTIMIZER_MAGIC_CONSTANT_2)
            return;
    }
    
    prb_t_init(&t,pt);
    file = prb_t_find(&t,pt,first_file);
    while(file && n){
        file->user_defined_flags |= UD_FILE_MOVED_TO_FRONT;
        n --;
        jp->already_optimized_clusters += file->disp.clusters;
        file = prb_t_next(&t);
    }
    if(n > 0){
        DebugPrint("cut_off_group_of_files: cannot find file in tree (case 1)");
    }
}

/**
 * @brief Marks all sorted out groups of
 * files in the tree as already optimized.
 */
static void cut_off_sorted_out_files(udefrag_job_parameters *jp,struct prb_table *pt)
{
    struct prb_traverser t;
    winx_file_info *file;
    winx_file_info *first_file; /* first file of the group */
    ULONGLONG n;                /* number of files in group */
    ULONGLONG length;           /* length of the group, in clusters */
    ULONGLONG pplcn;            /* LCN of (i - 2)-th file */
    ULONGLONG plcn;             /* LCN of (i - 1)-th file */
    ULONGLONG lcn;
    ULONGLONG distance;
    ULONGLONG file_length;
    winx_file_info *prev_file;
    #define INVALID_LCN ((ULONGLONG) -1)
    int belongs_to_group;
    ULONGLONG time;
    char buffer[32];
    
    time = start_timing("cutting off sorted out files",jp);
    jp->already_optimized_clusters = 0;
    
    /* select first not fragmented file */
    prb_t_init(&t,pt);
    file = prb_t_first(&t,pt);
    while(file){
        if(!is_fragmented(file)) break;
        file = prb_t_next(&t);
    }
    if(file == NULL) goto done;

    /* initialize group */
    first_file = file;
    n = 1;
    length = file->disp.clusters;
    pplcn = INVALID_LCN;
    plcn = file->disp.blockmap->lcn;
    prev_file = file;
    
    /* analyze subsequent files */
    file = prb_t_next(&t);
    while(file){
        /* check whether the file is in group or not */
        belongs_to_group = 1;
        /* 1. the file must be not fragmented */
        if(is_fragmented(file))
            belongs_to_group = 0;
        /* 2. the file must be beyond one of the preceding two files */
        if(belongs_to_group){
            if(pplcn != INVALID_LCN && plcn != INVALID_LCN){
                lcn = file->disp.blockmap->lcn;
                if(lcn < pplcn && lcn < plcn)
                    belongs_to_group = 0;
            }
        }
        /* 3. the file must be close to the preceding one */
        if(belongs_to_group && plcn != INVALID_LCN){
            lcn = file->disp.blockmap->lcn;
            if(lcn < plcn){
                distance = (plcn - lcn) * jp->v_info.bytes_per_cluster;
                file_length = file->disp.clusters * jp->v_info.bytes_per_cluster;
                if(distance > max(OPTIMIZER_MAGIC_CONSTANT,file_length))
                    belongs_to_group = 0;
            } else {
                distance = (lcn - plcn) * jp->v_info.bytes_per_cluster;
                file_length = prev_file->disp.clusters * jp->v_info.bytes_per_cluster;
                if(distance > max(OPTIMIZER_MAGIC_CONSTANT,file_length))
                    belongs_to_group = 0;
            }
        }
        if(belongs_to_group){
            n ++;
            length += file->disp.clusters;
            pplcn = plcn;
            plcn = file->disp.blockmap->lcn;
            prev_file = file;
        } else {
            if(n > 1){
                /* remark all files in previous group */
                cut_off_group_of_files(jp,pt,first_file,n,length);
            }
            /* reset group */
            while(file){
                if(!is_fragmented(file)) break;
                file = prb_t_next(&t);
            }
            if(file == NULL) goto done;
            first_file = file;
            n = 1;
            length = file->disp.clusters;
            pplcn = INVALID_LCN;
            plcn = file->disp.blockmap->lcn;
            prev_file = file;
        }
        file = prb_t_next(&t);
    }
    
    if(n > 1){
        /* remark all files in group */
        cut_off_group_of_files(jp,pt,first_file,n,length);
    }

done:
    DebugPrint("%I64u clusters skipped",jp->already_optimized_clusters);
    winx_bytes_to_hr(jp->already_optimized_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s skipped",buffer);
    stop_timing("cutting off sorted out files",time,jp);
}

/**
 * @brief Calculates number of allocated clusters
 * between start_lcn and the end of the disk.
 */
static ULONGLONG count_clusters(udefrag_job_parameters *jp,ULONGLONG start_lcn)
{
    winx_volume_region *rgn;
    ULONGLONG n = 0;
    ULONGLONG time = winx_xtime();

    /* actualize list of free regions */
    release_temp_space_regions(jp);

    for(rgn = jp->free_regions; rgn; rgn = rgn->next){
        if(jp->termination_router((void *)jp)) break;
        if(rgn->lcn >= start_lcn){
            n += rgn->length;
        } else if(rgn->lcn + rgn->length > start_lcn){
            n += rgn->length - (start_lcn - rgn->lcn);
        }
        if(rgn->next == jp->free_regions) break;
    }
    jp->p_counters.searching_time += winx_xtime() - time;
    return (jp->v_info.total_clusters - start_lcn - n);
}

/**
 * @brief Optimizes the disk.
 * @return Zero for success,
 * negative value otherwise.
 */
static int optimize_routine(udefrag_job_parameters *jp,ULONGLONG extra_clusters)
{
    winx_file_info *f;
    struct prb_table *pt;
    struct prb_traverser t;
    ULONGLONG start_lcn, end_lcn;
    ULONGLONG time;
    int result = 0;

    jp->pi.current_operation = VOLUME_OPTIMIZATION;
    jp->pi.processed_clusters = extra_clusters;
    /* we have a chance to move everything to the end and then back */
    /* more precise calculation is difficult */
    jp->pi.clusters_to_process = count_clusters(jp,0) * 2 + extra_clusters;

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("optimization",jp);

    /* no files are excluded by this task currently */
    for(f = jp->filelist; f; f = f->next){
        f->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(f->next == jp->filelist) break;
    }

    /* build tree of files sorted by path */
    pt = prb_create(files_compare,NULL,NULL);
    if(pt == NULL){
        DebugPrint("optimize_routine: cannot create binary tree");
        result = UDEFRAG_NO_MEM;
        goto done;
    }
    for(f = jp->filelist; f; f = f->next){
        if(f->disp.clusters * jp->v_info.bytes_per_cluster \
          < jp->udo.optimizer_size_limit){
            if(can_move_entirely(f,jp)){
                if(prb_probe(pt,(void *)f) == NULL){
                    DebugPrint("optimize_routine: cannot add file to the tree");
                    result = UDEFRAG_NO_MEM;
                    goto done;
                }
            }
        }
        if(f->next == jp->filelist) break;
    }
    
    if(jp->job_type == QUICK_OPTIMIZATION_JOB){
        /* cut off already sorted out groups of files */
        cut_off_sorted_out_files(jp,pt);
    }
    
    /* do the job */
    prb_t_init(&t,pt);
    if(prb_t_first(&t,pt) == NULL) goto done;
    jp->pi.pass_number = 0; start_lcn = end_lcn = 0;
    while(!jp->termination_router((void *)jp)){
        winx_dbg_print_header(0,0,"volume optimization pass #%u",jp->pi.pass_number);
        jp->pi.processed_clusters = \
            jp->pi.clusters_to_process - count_clusters(jp,start_lcn) * 2;
        
        /* cleanup space in the beginning of the disk */
        move_files_to_back(jp,&end_lcn);
        if(jp->termination_router((void *)jp)) break;
        
        /* move small files back, sorted by path */
        move_files_to_front(jp,&start_lcn,end_lcn,&t);
        
        /* break if no more files need optimization */
        if(prb_t_cur(&t) == NULL) break;
        
        /* continue file sorting on the next pass */
        jp->pi.pass_number ++;
    }
    
done:
    stop_timing("optimization",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    if(pt) prb_destroy(pt,NULL);
    return result;
}

/************************************************************/
/*                    The entry point                       */
/************************************************************/

/**
 * @brief Performs a volume optimization.
 * @details The disk optimization sorts
 * small files (below fragment size threshold)
 * by path on the disk. On FAT it optimizes
 * directories also. On NTFS it optimizes the MFT.
 * @return Zero for success, negative value otherwise.
 */
int optimize(udefrag_job_parameters *jp)
{
    int result, overall_result = -1;
    ULONGLONG extra_clusters = 0;
    
    /* reset filters */
    release_options(jp);
    jp->udo.size_limit = MAX_FILE_SIZE;
    jp->udo.fragments_limit = 0;
    
    /* perform volume analysis */
    result = analyze(jp); /* we need to call it once, here */
    if(result < 0) return result;
    
    /* reset counters */
    jp->pi.processed_clusters = 0;
    if(jp->is_fat) extra_clusters += opt_dirs_cc_routine(jp);
    if(jp->fs_type == FS_NTFS) extra_clusters += opt_mft_cc_routine(jp);
    jp->pi.clusters_to_process = count_clusters(jp,0) * 2 + extra_clusters;

    /* FAT specific: optimize directories */
    if(jp->is_fat){
        result = optimize_directories(jp);
        if(result == 0){
            /* at least something succeeded */
            overall_result = 0;
        }
    }
    
    /* NTFS specific: optimize MFT */
    if(jp->fs_type == FS_NTFS){
        result = optimize_mft_routine(jp);
        if(result == 0){
            /* at least something succeeded */
            overall_result = 0;
        }
    }
    
    /* optimize the disk */
    result = optimize_routine(jp,extra_clusters);
    if(result == 0){
        /* optimization succeeded */
        overall_result = 0;
    }
    
    /* get rid of fragmented files */
    jp->pi.clusters_to_process += defrag_cc_routine(jp);
    defragment(jp);
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

    /* reset counters */
    jp->pi.processed_clusters = 0;
    jp->pi.clusters_to_process = opt_mft_cc_routine(jp);
    
    /* do the job */
    result = optimize_mft_routine(jp);
    
    /* cleanup the disk */
    jp->pi.clusters_to_process += defrag_cc_routine(jp);
    defragment(jp);
    return result;
}

/** @} */
