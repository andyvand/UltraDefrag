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
 * @file dbg.c
 * @brief Debugging.
 * @addtogroup Debug
 * @{
 */

#include "wgx-internals.h"

/* should be enough for any message */
#define DBG_BUFFER_SIZE (64 * 1024)

WGX_TRACE_HANDLER InternalTraceHandler = NULL;

/**
 * @brief Sets an internal trace handler
 * intended for being called each time
 * Wgx library produces debugging output.
 * @param[in] h address of the routine.
 * May be NULL if no debugging output
 * is needed.
 */
void WgxSetInternalTraceHandler(WGX_TRACE_HANDLER h)
{
    InternalTraceHandler = h;
}

/**
 * @brief Displays message box with formatted string in caption
 * and with description of the last Win32 error inside the window.
 * @param[in] hParent handle to the parent window.
 * @param[in] msgbox_flags flags passed to MessageBox routine.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 * @return Return value is the same as MessageBox returns.
 */
int WgxDisplayLastError(HWND hParent,UINT msgbox_flags, char *format, ...)
{
    char *msg;
    va_list arg;
    unsigned int length;
    DWORD error = GetLastError();
    wchar_t *umsg, *desc = NULL, *text;
    #define SM_BUFFER_SIZE 32
    wchar_t b[SM_BUFFER_SIZE];
    int result;
    
    if(format == NULL)
        return 0;

    msg = malloc(DBG_BUFFER_SIZE);
    if(msg == NULL){
        etrace("not enough memory (case 1)");
        return 0;
    }

    /* store formatted string into buffer */
    va_start(arg,format);
    memset(msg,0,DBG_BUFFER_SIZE);
    length = _vsnprintf(msg,DBG_BUFFER_SIZE - 1,format,arg);
    (void)length;
    msg[DBG_BUFFER_SIZE - 1] = 0;
    va_end(arg);

    /* send formatted string to the debugger */
    if(InternalTraceHandler){
        SetLastError(error);
        InternalTraceHandler(LAST_ERROR_FLAG,E"%s",msg);
    }    
    
    /* display message box */
    umsg = malloc(DBG_BUFFER_SIZE * sizeof(wchar_t));
    if(umsg == NULL){
        etrace("not enough memory (case 2)");
        free(msg);
        return 0;
    }
    _snwprintf(umsg,DBG_BUFFER_SIZE,L"%hs",msg);
    umsg[DBG_BUFFER_SIZE - 1] = 0;
    
    if(!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPWSTR)(void *)&desc,0,NULL)){
        if(error == ERROR_COMMITMENT_LIMIT)
            (void)_snwprintf(b,SM_BUFFER_SIZE,L"Not enough memory.");
        else
            (void)_snwprintf(b,SM_BUFFER_SIZE,L"Error code = 0x%x",(UINT)error);
        b[SM_BUFFER_SIZE - 1] = 0;
        desc = NULL;
        text = b;
    } else {
        text = desc;
    }

    result = MessageBoxW(hParent,text,umsg,msgbox_flags);
    
    /* cleanup */
    if(desc) LocalFree(desc);
    free(umsg);
    free(msg);
    return result;
}

/** @} */
