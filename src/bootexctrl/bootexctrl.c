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

/*
* BootExecute Control program.
*/

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <commctrl.h>

#define WgxTraceHandler dbg_print
#include "../dll/wgx/wgx.h"

#define DisplayLastError(caption) WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,caption)

#include "../include/ultradfgver.h"

int h_flag = 0;
int r_flag = 0;
int u_flag = 0;
int silent = 0;
int invalid_opts = 0;
char cmd[MAX_PATH + 1] = "";

void show_help(void)
{
    if(silent)
        return;
    
    MessageBox(NULL,
        "Usage:\n"
        "bootexctrl /r [/s] command - register command\n"
        "bootexctrl /u [/s] command - unregister command\n"
        "Specify /s option to run the program in silent mode."
        ,
        "BootExecute Control",
        MB_OK
        );
}

/**
 * @brief Replaces CR and LF
 * characters in a string by spaces.
 * @details Intended for use in dbg_print
 * routine to keep logging as clean as possible.
 */
void remove_crlf(char *s)
{
    int i;
    
    if(s){
        for(i = 0; s[i]; i++){
            if(s[i] == '\r' || s[i] == '\n')
                s[i] = ' ';
        }
    }
}

void dbg_print(int flags,char *format, ...)
{
    va_list arg;
    char *buffer;
    char *msg;
    char *ext_buffer;
    DWORD error;
    
    error = GetLastError();
    if(format){
        va_start(arg,format);
        buffer = wgx_vsprintf(format,arg);
        if(buffer){
            if(flags & LAST_ERROR_FLAG){
                if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)(void *)&msg,0,NULL)){
                    if(error == ERROR_COMMITMENT_LIMIT){
                        ext_buffer = wgx_sprintf("%s: 0x%x error: not enough memory\n",buffer,(UINT)error);
                    } else {
                        ext_buffer = wgx_sprintf("%s: 0x%x error\n",buffer,(UINT)error);
                    }
                    if(ext_buffer){
                        OutputDebugString(ext_buffer);
                        free(ext_buffer);
                    } else {
                        OutputDebugString("dbg_print: not enough memory\n");
                    }
                } else {
                    ext_buffer = wgx_sprintf("%s: 0x%x error: %s",buffer,(UINT)error,msg);
                    if(ext_buffer){
                        remove_crlf(ext_buffer);
                        OutputDebugString(ext_buffer);
                        OutputDebugString("\n");
                        free(ext_buffer);
                    } else {
                        OutputDebugString("dbg_print: not enough memory\n");
                    }
                    LocalFree(msg);
                }
            } else {
                OutputDebugString(buffer);
                OutputDebugString("\n");
            }
            free(buffer);
        } else {
            OutputDebugString("dbg_print: not enough memory\n");
        }
        va_end(arg);
    }
}

int open_smss_key(HKEY *phkey)
{
    LONG result;
    
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
          "SYSTEM\\CurrentControlSet\\Control\\Session Manager",
          0,KEY_QUERY_VALUE | KEY_SET_VALUE,phkey);
    if(result != ERROR_SUCCESS){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError(L"Cannot open SMSS key!");
        }
        return 0;
    }
    return 1;
}

/**
 * @brief Compares two boot execute commands.
 * @details Treats 'command' and 'autocheck command' as the same.
 * @param[in] reg_cmd command read from registry.
 * @param[in] cmd command to be searched for.
 * @return Positive value indicates that commands are equal,
 * zero indicates that they're different, negative value
 * indicates failure of comparison.
 * @note Internal use only.
 */
static int cmd_compare(char *reg_cmd,char *cmd)
{
    char *long_cmd;
    int result;
    
    /* do we have the command registered as it is? */
    if(!_stricmp(cmd,reg_cmd))
        return 1;
        
    /* compare reg_cmd with 'autocheck {cmd}' */
    long_cmd = wgx_sprintf("autocheck %hs",cmd);
    if(long_cmd == NULL){
        if(!silent)
            MessageBox(NULL,"Not enough memory!","Error",MB_OK | MB_ICONHAND);
        return (-1);
    }
        
    result = _stricmp(long_cmd,reg_cmd);
    free(long_cmd);
    return (result == 0) ? 1 : 0;
}

int register_cmd(void)
{
    HKEY hKey;
    DWORD type, size;
    char *data, *curr_pos;
    DWORD i, length, curr_len;
    LONG result;

    if(!open_smss_key(&hKey))
        return 3;

    type = REG_MULTI_SZ;
    result = RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
    if(result != ERROR_SUCCESS && result != ERROR_MORE_DATA){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError(L"Cannot query BootExecute value size!");
        }
        return 4;
    }
    
    data = malloc(size + strlen(cmd) + 10);
    if(data == NULL){
        if(!silent)
            MessageBox(NULL,"Not enough memory!","Error",MB_OK | MB_ICONHAND);
        (void)RegCloseKey(hKey);
        return 5;
    }

    type = REG_MULTI_SZ;
    result = RegQueryValueEx(hKey,"BootExecute",NULL,&type,(LPBYTE)data,&size);
    if(result != ERROR_SUCCESS){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError(L"Cannot query BootExecute value!");
        }
        (void)RegCloseKey(hKey);
        free(data);
        return 6;
    }
    
    if(size < 2){ /* "\0\0" */
        /* empty value detected */
        strcpy(data,cmd);
        data[strlen(data) + 1] = 0;
        length = strlen(data) + 1 + 1;
        goto save_changes;
    }
    
    /* terminate value by two zeros */
    data[size - 2] = 0;
    data[size - 1] = 0;

    length = size - 1;
    for(i = 0; i < length;){
        curr_pos = data + i;
        curr_len = strlen(curr_pos) + 1;
        /* if the command is yet registered then exit */
        if(cmd_compare(curr_pos,cmd) > 0) goto done;
        i += curr_len;
    }
    (void)strcpy(data + i,cmd);
    data[i + strlen(cmd) + 1] = 0;
    length += strlen(cmd) + 1 + 1;

save_changes:
    result = RegSetValueEx(hKey,"BootExecute",0,REG_MULTI_SZ,(const BYTE *)data,length);
    if(result != ERROR_SUCCESS){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError(L"Cannot set BootExecute value!");
        }
        (void)RegCloseKey(hKey);
        free(data);
        return 7;
    }

done:
    (void)RegCloseKey(hKey);
    free(data);
    return 0;
}

int unregister_cmd(void)
{
    HKEY hKey;
    DWORD type, size;
    char *data, *new_data = NULL, *curr_pos;
    DWORD i, length, new_length, curr_len;
    LONG result;

    if(!open_smss_key(&hKey))
        return 3;

    type = REG_MULTI_SZ;
    result = RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
    if(result != ERROR_SUCCESS && result != ERROR_MORE_DATA){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError(L"Cannot query BootExecute value size!");
        }
        return 4;
    }
    
    data = malloc(size);
    if(data == NULL){
        if(!silent)
            MessageBox(NULL,"Not enough memory!","Error",MB_OK | MB_ICONHAND);
        (void)RegCloseKey(hKey);
        return 5;
    }

    type = REG_MULTI_SZ;
    result = RegQueryValueEx(hKey,"BootExecute",NULL,&type,(LPBYTE)data,&size);
    if(result != ERROR_SUCCESS){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError(L"Cannot query BootExecute value!");
        }
        (void)RegCloseKey(hKey);
        free(data);
        return 6;
    }

    if(size < 2){ /* "\0\0" */
        /* empty value detected */
        goto done;
    }
    
    /* terminate value by two zeros */
    data[size - 2] = 0;
    data[size - 1] = 0;

    new_data = malloc(size);
    if(new_data == NULL){
        if(!silent)
            MessageBox(NULL,"Not enough memory!","Error",MB_OK | MB_ICONHAND);
        (void)RegCloseKey(hKey);
        free(data);
        return 5;
    }

    /*
    * Now we should copy all strings except specified command
    * to the destination buffer.
    */
    memset((void *)new_data,0,size);
    length = size - 1;
    new_length = 0;
    for(i = 0; i < length;){
        curr_pos = data + i;
        curr_len = strlen(curr_pos) + 1;
        if(cmd_compare(curr_pos,cmd) <= 0){
            (void)strcpy(new_data + new_length,curr_pos);
            new_length += curr_len;
        }
        i += curr_len;
    }
    new_data[new_length] = 0;

    result = RegSetValueEx(hKey,"BootExecute",0,REG_MULTI_SZ,(const BYTE *)new_data,new_length + 1);
    if(result != ERROR_SUCCESS){
        if(!silent){
            SetLastError((DWORD)result);
            DisplayLastError(L"Cannot set BootExecute value!");
        }
        (void)RegCloseKey(hKey);
        free(data);
        free(new_data);
        return 7;
    }

done:
    (void)RegCloseKey(hKey);
    free(data);
    free(new_data);
    return 0;
}

int parse_cmdline(void)
{
    int argc;
    wchar_t **argv;
    
    if(wcsstr(_wcslwr(GetCommandLineW()),L"/s"))
        silent = 1;

    argv = (wchar_t **)CommandLineToArgvW(_wcslwr(GetCommandLineW()),&argc);
    if(argv == NULL){
        if(!silent)
            DisplayLastError(L"CommandLineToArgvW failed!");
        exit(1);
    }

    for(argc--; argc >= 1; argc--){
        if(!wcscmp(argv[argc],L"/r")){
            r_flag = 1;
        } else if(!wcscmp(argv[argc],L"/u")){
            u_flag = 1;
        } else if(!wcscmp(argv[argc],L"/h")){
            h_flag = 1;
        } else if(!wcscmp(argv[argc],L"/?")){
            h_flag = 1;
        } else if(!wcscmp(argv[argc],L"/s")){
            silent = 1;
        } else {
            if(wcslen(argv[argc]) > MAX_PATH){
                invalid_opts = 1;
                if(!silent)
                    MessageBox(NULL,"Command name is too long!","Error",MB_OK | MB_ICONHAND);
                exit(2);
            } else {
                _snprintf(cmd,MAX_PATH,"%ws",argv[argc]);
                cmd[MAX_PATH] = 0;
            }
        }
    }
    
    if(cmd[0] == 0)
        h_flag = 1;
    
    if(r_flag == 0 && u_flag == 0)
        h_flag = 1;
    
    GlobalFree(argv);
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    /* strongly required! to be compatible with manifest */
    InitCommonControls();

    WgxSetInternalTraceHandler(dbg_print);

    parse_cmdline();

    if(h_flag){
        show_help();
        return 0;
    }
    
    /* check for admin rights - they're strongly required */
    if(!WgxCheckAdminRights()){
        if(!silent){
            MessageBox(NULL,"Administrative rights are needed "
              "to run the program!","BootExecute Control",
              MB_OK | MB_ICONHAND);
        }
        return 1;
    }

    return (r_flag ? register_cmd() : unregister_cmd());
}
