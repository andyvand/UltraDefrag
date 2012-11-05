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
 * @file options.c
 * @brief Job options.
 * @addtogroup Options
 * @{
 */
     
#include "udefrag-internals.h"
#include <math.h> /* for pow function */

/**
 * @brief Retrieves all ultradefrag
 * related options from the environment.
 * @return Zero for success, negative
 * value otherwise.
 */
int get_options(udefrag_job_parameters *jp)
{
    wchar_t *buffer, *dp;
    char buf[64];
    double r;
    unsigned int it;
    int i, z, index = 0;
    char *methods[] = {
        "path", "path", "size", "creation time",
        "last modification time", "last access time"
    };

    if(jp == NULL)
        return (-1);
    
    /* reset all options */
    memset(&jp->udo,0,sizeof(udefrag_options));
    jp->udo.refresh_interval = DEFAULT_REFRESH_INTERVAL;
    
    /* allocate memory */
    buffer = winx_malloc(ENV_BUFFER_SIZE * sizeof(wchar_t));
    if(buffer == NULL)
        return UDEFRAG_NO_MEM;
    
    /* set filters */
    if(winx_query_env_variable(L"UD_IN_FILTER",buffer,ENV_BUFFER_SIZE) >= 0){
        DebugPrint("in_filter = %ws",buffer);
        winx_patcomp(&jp->udo.in_filter,buffer,L";\"",WINX_PAT_ICASE);
    }
    if(winx_query_env_variable(L"UD_EX_FILTER",buffer,ENV_BUFFER_SIZE) >= 0){
        DebugPrint("ex_filter = %ws",buffer);
        winx_patcomp(&jp->udo.ex_filter,buffer,L";\"",WINX_PAT_ICASE);
    }
    if(winx_query_env_variable(L"UD_CUT_FILTER",buffer,ENV_BUFFER_SIZE) >= 0){
        DebugPrint("cut_filter = %ws",buffer);
        winx_patcomp(&jp->udo.cut_filter,buffer,L";\"",WINX_PAT_ICASE);
    }

    /* set fragment size threshold */
    if(winx_query_env_variable(L"UD_FRAGMENT_SIZE_THRESHOLD",buffer,ENV_BUFFER_SIZE) >= 0){
        (void)_snprintf(buf,sizeof(buf) - 1,"%ws",buffer);
        buf[sizeof(buf) - 1] = 0;
        jp->udo.fragment_size_threshold = winx_hr_to_bytes(buf);
    }
    /* if theshold isn't set, assign maximum value to it */
    if(jp->udo.fragment_size_threshold == 0)
        jp->udo.fragment_size_threshold = DEFAULT_FRAGMENT_SIZE_THRESHOLD;

    /* set file size threshold */
    if(winx_query_env_variable(L"UD_FILE_SIZE_THRESHOLD",buffer,ENV_BUFFER_SIZE) >= 0){
        (void)_snprintf(buf,sizeof(buf) - 1,"%ws",buffer);
        buf[sizeof(buf) - 1] = 0;
        jp->udo.size_limit = winx_hr_to_bytes(buf);
    }
    if(jp->udo.size_limit == 0)
        jp->udo.size_limit = MAX_FILE_SIZE;
    if(winx_query_env_variable(L"UD_OPTIMIZER_FILE_SIZE_THRESHOLD",buffer,ENV_BUFFER_SIZE) >= 0){
        (void)_snprintf(buf,sizeof(buf) - 1,"%ws",buffer);
        buf[sizeof(buf) - 1] = 0;
        jp->udo.optimizer_size_limit = winx_hr_to_bytes(buf);
    }
    /* if optimizer's threshold isn't set, assign default value to it */
    if(jp->udo.optimizer_size_limit == 0)
        jp->udo.optimizer_size_limit = OPTIMIZER_MAGIC_CONSTANT;

    /* set file fragments threshold */
    if(winx_query_env_variable(L"UD_FRAGMENTS_THRESHOLD",buffer,ENV_BUFFER_SIZE) >= 0)
        jp->udo.fragments_limit = (ULONGLONG)_wtol(buffer);
    
    /* set file sorting options */
    if(winx_query_env_variable(L"UD_SORTING",buffer,ENV_BUFFER_SIZE) >= 0){
        (void)_wcslwr(buffer);
        if(!wcscmp(buffer,L"path"))
            index = 1, jp->udo.sorting_flags |= UD_SORT_BY_PATH;
        else if(!wcscmp(buffer,L"size"))
            index = 2, jp->udo.sorting_flags |= UD_SORT_BY_SIZE;
        else if(!wcscmp(buffer,L"c_time"))
            index = 3, jp->udo.sorting_flags |= UD_SORT_BY_CREATION_TIME;
        else if(!wcscmp(buffer,L"m_time"))
            index = 4, jp->udo.sorting_flags |= UD_SORT_BY_MODIFICATION_TIME;
        else if(!wcscmp(buffer,L"a_time"))
            index = 5, jp->udo.sorting_flags |= UD_SORT_BY_ACCESS_TIME;
    }
    if(winx_query_env_variable(L"UD_SORTING_ORDER",buffer,ENV_BUFFER_SIZE) >= 0){
        (void)_wcslwr(buffer);
        if(wcsstr(buffer,L"desc"))
            jp->udo.sorting_flags |= UD_SORT_DESCENDING;
    }
    
    /* set time limit */
    if(winx_query_env_variable(L"UD_TIME_LIMIT",buffer,ENV_BUFFER_SIZE) >= 0){
        (void)_snprintf(buf,sizeof(buf) - 1,"%ws",buffer);
        buf[sizeof(buf) - 1] = 0;
        jp->udo.time_limit = winx_str2time(buf);
    }

    /* set progress refresh interval */
    if(winx_query_env_variable(L"UD_REFRESH_INTERVAL",buffer,ENV_BUFFER_SIZE) >= 0)
        jp->udo.refresh_interval = _wtoi(buffer);

    /* check for disable_reports option */
    if(winx_query_env_variable(L"UD_DISABLE_REPORTS",buffer,ENV_BUFFER_SIZE) >= 0) {
        if(!wcscmp(buffer,L"1"))
            jp->udo.disable_reports = 1;
    }

    /* set debug print level */
    if(winx_query_env_variable(L"UD_DBGPRINT_LEVEL",buffer,ENV_BUFFER_SIZE) >= 0){
        (void)_wcsupr(buffer);
        if(!wcscmp(buffer,L"DETAILED"))
            jp->udo.dbgprint_level = DBG_DETAILED;
        else if(!wcscmp(buffer,L"PARANOID"))
            jp->udo.dbgprint_level = DBG_PARANOID;
    }
    
    /* set dry_run variable */
    if(winx_query_env_variable(L"UD_DRY_RUN",buffer,ENV_BUFFER_SIZE) >= 0){
        if(!wcscmp(buffer,L"1")){
            DebugPrint("%%UD_DRY_RUN%% environment variable is set to 1,");
            DebugPrint("therefore no actual data moves will be performed on disk");
            jp->udo.dry_run = 1;
        }
    }
    
    /* set fragmentation threshold */
    if(winx_query_env_variable(L"UD_FRAGMENTATION_THRESHOLD",buffer,ENV_BUFFER_SIZE) >= 0){
        jp->udo.fragmentation_threshold = (double)_wtoi(buffer);
        dp = wcschr(buffer,'.');
        if(dp != NULL){
            for(z = 0; dp[z + 1] == '0'; z++) {}
            for(r = (double)_wtoi(dp + 1); r > 1; r /= 10){}
            r *= pow(10, -z);
            jp->udo.fragmentation_threshold += r;
        }
    }
    
    /* print all options */
    winx_dbg_print_header(0,0,"ultradefrag job options");
    if(jp->udo.in_filter.count){
        DebugPrint("in_filter patterns:");
        for(i = 0; i < jp->udo.in_filter.count; i++)
            DebugPrint("  + %ws",jp->udo.in_filter.array[i]);
    }
    if(jp->udo.ex_filter.count){
        DebugPrint("ex_filter patterns:");
        for(i = 0; i < jp->udo.ex_filter.count; i++)
            DebugPrint("  - %ws",jp->udo.ex_filter.array[i]);
    }
    if(jp->udo.cut_filter.count){
        DebugPrint("cut_filter patterns:");
        for(i = 0; i < jp->udo.cut_filter.count; i++)
            DebugPrint("  + %ws",jp->udo.cut_filter.array[i]);
    }
    it = (unsigned int)(jp->udo.fragmentation_threshold * 100.00);
    DebugPrint("fragmentation threshold                   = %u.%02u %%",it / 100,it % 100);
    (void)winx_bytes_to_hr(jp->udo.size_limit,1,buf,sizeof(buf));
    DebugPrint("file size threshold (for defragmentation) = %s",buf);
    (void)winx_bytes_to_hr(jp->udo.optimizer_size_limit,1,buf,sizeof(buf));
    DebugPrint("file size threshold (for optimization)    = %s",buf);
    (void)winx_bytes_to_hr(jp->udo.fragment_size_threshold,1,buf,sizeof(buf));
    DebugPrint("fragment size threshold                   = %s",buf);
    DebugPrint("file fragments threshold                  = %I64u",jp->udo.fragments_limit);
    DebugPrint("files will be sorted by %s in %s order",methods[index],
        (jp->udo.sorting_flags & UD_SORT_DESCENDING) ? "descending" : "ascending");
    DebugPrint("time limit                                = %I64u seconds",jp->udo.time_limit);
    DebugPrint("progress refresh interval                 = %u msec",jp->udo.refresh_interval);
    if(jp->udo.disable_reports) DebugPrint("reports disabled");
    else DebugPrint("reports enabled");
    switch(jp->udo.dbgprint_level){
    case DBG_DETAILED:
        DebugPrint("detailed debug level set");
        break;
    case DBG_PARANOID:
        DebugPrint("paranoid debug level set");
        break;
    default:
        DebugPrint("normal debug level set");
    }

    /* cleanup */
    winx_free(buffer);
    return 0;
}

/**
 * @brief Frees all resources
 * allocated by get_options.
 */
void release_options(udefrag_job_parameters *jp)
{
    if(jp){
        winx_patfree(&jp->udo.in_filter);
        winx_patfree(&jp->udo.ex_filter);
    }
}

/** @} */
