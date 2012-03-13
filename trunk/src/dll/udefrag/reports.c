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
 * @file reports.c
 * @brief File fragmentation reports.
 * @addtogroup Reports
 * @{
 */

/*
* Revised by Stefan Pendl, 2010, 2011
* <stefanpe@users.sourceforge.net>
*/

#include "udefrag-internals.h"

/**
 * @brief The size of the report saving buffer, in bytes.
 */
#define RSB_SIZE (512 * 1024)

static int save_lua_report(udefrag_job_parameters *jp)
{
    char path[] = "\\??\\A:\\fraglist.luar";
    WINX_FILE *f;
    char buffer[512];
    udefrag_fragmented_file *file;
    char *comment;
    char *status;
    int i, length, offset;
    char human_readable_size[32];
    int n1, n2;
    char s[32]; /* must be at least 32 chars long */
    winx_time t;
    
    path[4] = jp->volume_letter;
    f = winx_fbopen(path,"w",RSB_SIZE);
    if(f == NULL){
        f = winx_fopen(path,"w");
        if(f == NULL){
            return (-1);
        }
    }

    /* print header */
    memset(&t,0,sizeof(winx_time));
    (void)winx_get_local_time(&t);
    (void)_snprintf(buffer,sizeof(buffer),
        "-- UltraDefrag report for disk %c:\r\n\r\n"
        "format_version = 5\r\n\r\n"
        "volume_letter = \"%c\"\r\n"
        /*"current_time = \"%02i.%02i.%04i at %02i:%02i\"\r\n"*/
        "current_time = {\r\n"
        "\tyear = %04i,\r\n"
        "\tmonth = %02i,\r\n"
        "\tday = %02i,\r\n"
        "\thour = %02i,\r\n"
        "\tmin = %02i,\r\n"
        "\tsec = %02i,\r\n"
        "\tisdst = false\r\n"
        "}\r\n\r\n"
        "files = {\r\n",
        jp->volume_letter, jp->volume_letter,
        /*(int)t.day,(int)t.month,(int)t.year,(int)t.hour,(int)t.minute*/
        (int)t.year,(int)t.month,(int)t.day,(int)t.hour,(int)t.minute,(int)t.second
        );
    buffer[sizeof(buffer) - 1] = 0;
    (void)winx_fwrite(buffer,1,strlen(buffer),f);
    
    /* print body */
    for(file = jp->fragmented_files; file; file = file->next){
        if(!is_excluded(file->f)){
            if(is_directory(file->f))
                comment = "[DIR]";
            else if(is_compressed(file->f))
                comment = "[CMP]";
            else if(is_over_limit(file->f))
                comment = "[OVR]";
            else
                comment = " - ";
            
            /*
            * On change of status strings don't forget
            * also to adjust write_file_status routine
            * in udreportcnv.lua file.
            */
            if(is_locked(file->f))
                status = "locked";
            else if(is_moving_failed(file->f))
                status = "move failed";
            else if(is_in_improper_state(file->f))
                status = "invalid";
            else
                status = " - ";
            
            (void)winx_bytes_to_hr(file->f->disp.clusters * jp->v_info.bytes_per_cluster,
                1,human_readable_size,sizeof(human_readable_size));
            if(sscanf(human_readable_size,"%u.%u %31s",&n1,&n2,s) == 3){
                if(n2 >= 5) n1 += 1; /* round up, so 1.9 will get 2 instead of 1*/

                _snprintf(human_readable_size,sizeof(human_readable_size),"%u&nbsp;%s",n1,s);
                human_readable_size[sizeof(human_readable_size) - 1] = 0;
            }
            (void)_snprintf(buffer, sizeof(buffer),
                "\t{fragments = %u,size = %I64u,hrsize = \"%s\",filtered = %u,"
                "comment = \"%s\",status = \"%s\",uname = {",
                (UINT)file->f->disp.fragments,
                file->f->disp.clusters * jp->v_info.bytes_per_cluster,
                human_readable_size,
                (UINT)is_excluded(file->f),
                comment,status
                );
            buffer[sizeof(buffer) - 1] = 0;
            (void)winx_fwrite(buffer,1,strlen(buffer),f);

            if(file->f->path == NULL){
                strcpy(buffer,"0");
                (void)winx_fwrite(buffer,1,strlen(buffer),f);
            } else {
                /* skip \??\ sequence in the beginning of the path */
                length = wcslen(file->f->path);
                if(length > 4) offset = 4; else offset = 0;
                for(i = offset; i < length; i++){
                    /* conversion to unsigned short firstly is needed here to convert all unicode chars properly */
                    (void)_snprintf(buffer,sizeof(buffer),"%u,",(unsigned int)(unsigned short)file->f->path[i]);
                    buffer[sizeof(buffer) - 1] = 0;
                    (void)winx_fwrite(buffer,1,strlen(buffer),f);
                }
            }

            (void)strcpy(buffer,"}},\r\n");
            (void)winx_fwrite(buffer,1,strlen(buffer),f);
        }
        if(file->next == jp->fragmented_files) break;
    }
    
    /* print footer */
    (void)strcpy(buffer,"}\r\n");
    (void)winx_fwrite(buffer,1,strlen(buffer),f);

    DebugPrint("report saved to %s",path);
    winx_fclose(f);
    return 0;
}

/**
 * @brief Saves fragmentation report on the volume.
 * @return Zero for success, negative value otherwise.
 */
int save_fragmentation_report(udefrag_job_parameters *jp)
{
    int result = 0;
    ULONGLONG time;

    winx_dbg_print_header(0,0,"*");
    dbg_print_file_counters(jp);
    
    if(jp->udo.disable_reports)
        return 0;
    
    winx_dbg_print_header(0,0,"*");
    winx_dbg_print_header(0,0,"report saving started");
    time = winx_xtime();

    result = save_lua_report(jp);
    
    winx_dbg_print_header(0,0,"report saved in %I64u ms",
        winx_xtime() - time);
    return result;
}

/**
 * @brief Removes all fragmentation reports from the volume.
 */
void remove_fragmentation_report(udefrag_job_parameters *jp)
{
    char *paths[] = {
        "\\??\\%c:\\fraglist.luar",
        "\\??\\%c:\\fraglist.txt",
        "\\??\\%c:\\fraglist.htm",
        "\\??\\%c:\\fraglist.html",
        NULL
    };
    char path[64];
    int i;
    
    winx_dbg_print_header(0,0,"*");
    
    for(i = 0; paths[i]; i++){
        _snprintf(path,64,paths[i],jp->volume_letter);
        path[63] = 0;
        (void)winx_delete_file(path);
    }
}

/** @} */

