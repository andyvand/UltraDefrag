/*
 *  WGX - Windows GUI Extended Library.
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
 * @file shell.c
 * @brief Windows Shell.
 * @addtogroup Shell
 * @{
 */

#include "wgx-internals.h"

/**
 * @brief Calls a Win32 ShellExecute() procedure and shows a message
 * box with detailed information in case of errors.
 */
BOOL WgxShellExecuteW(HWND hwnd,LPCWSTR lpOperation,LPCWSTR lpFile,
                               LPCWSTR lpParameters,LPCWSTR lpDirectory,INT nShowCmd)
{
    HINSTANCE hApp;
    int ret;
    char *desc;
    wchar_t *msg;
    
    hApp = ShellExecuteW(hwnd,lpOperation,lpFile,lpParameters,lpDirectory,nShowCmd);
    ret = (int)(LONG_PTR)hApp;
    if(ret > 32) return TRUE;
    
    /* handle errors */
    switch(ret){
    case 0:
        desc = "The operating system is out of memory or resources.";
        break;
    case ERROR_FILE_NOT_FOUND:
    /*case SE_ERR_FNF:*/
        desc = "The specified file was not found.";
        break;
    case ERROR_PATH_NOT_FOUND:
    /*case SE_ERR_PNF:*/
        desc = "The specified path was not found.";
        break;
    case ERROR_BAD_FORMAT:
        desc = "The .exe file is invalid (non-Microsoft Win32 .exe or error in .exe image).";
        break;
    case SE_ERR_ACCESSDENIED:
        desc = "The operating system denied access to the specified file.";
        break;
    case SE_ERR_ASSOCINCOMPLETE:
        desc = "The file name association is incomplete or invalid.";
        break;
    case SE_ERR_DDEBUSY:
        desc = "The Dynamic Data Exchange (DDE) transaction could not be completed\n"
               " because other DDE transactions were being processed.";
        break;
    case SE_ERR_DDEFAIL:
        desc = "The DDE transaction failed.";
        break;
    case SE_ERR_DDETIMEOUT:
        desc = "The DDE transaction could not be completed because the request timed out.";
        break;
    case SE_ERR_DLLNOTFOUND:
        desc = "The specified dynamic-link library (DLL) was not found.";
        break;
    case SE_ERR_NOASSOC:
        desc = "There is no application associated with the given file name extension.\n"
               "Or you attempt to print a file that is not printable.";
        break;
    case SE_ERR_OOM:
        desc = "There was not enough memory to complete the operation.";
        break;
    case SE_ERR_SHARE:
        desc = "A sharing violation occurred.";
        break;
    default:
        desc = "";
        break;
    }
    
    if(!lpOperation) lpOperation = L"open";
    if(!lpFile) lpFile = L"";
    if(!lpParameters) lpParameters = L"";

    if(desc[0]){
        msg = wgx_swprintf(L"Cannot %ls %ls %ls\n%hs",
            lpOperation,lpFile,lpParameters,desc);
    } else {
        msg = wgx_swprintf(L"Cannot %ls %ls %ls\nError code = 0x%x",
            lpOperation,lpFile,lpParameters,ret);
    }
    MessageBoxW(hwnd,msg ? msg : L"Not enough memory!",
        L"Error!",MB_OK | MB_ICONHAND);
    free(msg);
    return FALSE;
}

/** @} */
