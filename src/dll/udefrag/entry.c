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
 * @file entry.c
 * @brief Entry point.
 * @addtogroup Entry
 * @{
 */

#include "udefrag-internals.h"

HANDLE hMutex = NULL;

/**
 * @brief udefrag.dll entry point.
 */
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
    if(dwReason == DLL_PROCESS_ATTACH){
        /* deny installation/upgrade */
        if(winx_get_os_version() >= WINDOWS_VISTA){
            (void)winx_create_mutex(L"\\Sessions\\1"
                L"\\BaseNamedObjects\\ultradefrag_mutex",
                &hMutex);
        } else {
            (void)winx_create_mutex(L"\\BaseNamedObjects"
                L"\\ultradefrag_mutex",&hMutex);
        }
        /* accept UD_LOG_FILE_PATH environment variable */
        (void)udefrag_set_log_file_path();
    } else if(dwReason == DLL_PROCESS_DETACH){
        /* allow installation/upgrade */
        winx_destroy_mutex(hMutex);
    }
    return 1;
}

/** @} */
