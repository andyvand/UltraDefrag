/*
 *  WGX - Windows GUI Extended Library.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file exec.c
 * @brief Processes and Threads.
 * @addtogroup Exec
 * @{
 */

#include "wgx-internals.h"

#define MAX_CMD_LENGTH 32768

typedef BOOL (WINAPI*CHECKTOKENMEMBERSHIP)(HANDLE TokenHandle,PSID SidToCheck,PBOOL IsMember);

/**
 * @brief A lightweight version
 * of the CreateProcess system API.
 */
BOOL WgxCreateProcess(char *cmd,char *args)
{
    char *command = NULL;
    char *cmdline = NULL;
    int error = ERROR_SUCCESS;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    /* validate arguments */
    if(cmd == NULL){
        error = ERROR_INVALID_PARAMETER;
        etrace("cmd == NULL");
        goto done;
    }
    if(args == NULL)
        args = "";
    
    /* allocate memory */
    command = malloc(MAX_CMD_LENGTH);
    cmdline = malloc(MAX_CMD_LENGTH);
    if(command == NULL || cmdline == NULL){
        error = ERROR_NOT_ENOUGH_MEMORY;
        etrace("not enough memory for %s",cmd);
        goto done;
    }
    
    /* expand environment strings in command */
    if(!ExpandEnvironmentStrings(cmd,command,MAX_CMD_LENGTH)){
        error = GetLastError();
        letrace("cannot expand environment strings in %s",cmd);
        goto done;
    }
    
    /* build command line */
    _snprintf(cmdline,MAX_CMD_LENGTH,"%s %s",command,args);
    cmdline[MAX_CMD_LENGTH - 1] = 0;
    
    /* create process */
    ZeroMemory(&si,sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;
    ZeroMemory(&pi,sizeof(pi));

    if(!CreateProcess(command,cmdline,
      NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
        error = GetLastError();
        letrace("cannot create process for %s",cmd);
        goto done;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
done:
    free(command);
    free(cmdline);
    SetLastError(error);
    return (error == ERROR_SUCCESS) ? TRUE : FALSE;
}

/**
 * @brief A lightweight version
 * of the CreateThread system API.
 */
BOOL WgxCreateThread(LPTHREAD_START_ROUTINE routine,LPVOID param)
{
    DWORD id;
    HANDLE h;
    
    h = CreateThread(NULL,0,routine,param,0,&id);
    if(h == NULL)
        return FALSE;
    CloseHandle(h);
    return TRUE;
}

/**
 * @brief Sets priority class for the current process.
 * @param[in] priority_class process priority class.
 * Read MSDN article on SetPriorityClass for details.
 * @return Boolean value. TRUE indicates success.
 */
BOOL WgxSetProcessPriority(DWORD priority_class)
{
    HANDLE hProcess;
    BOOL result;
    
    hProcess = OpenProcess(PROCESS_SET_INFORMATION,
        FALSE,GetCurrentProcessId());
    if(hProcess == NULL){
        letrace("cannot open current process");
        return FALSE;
    }
    result = SetPriorityClass(hProcess,priority_class);
    if(!result){
        letrace("cannot set priority class");
    }
    CloseHandle(hProcess);
    return result;
}

/**
 * @brief Defines whether the user
 * has administrative rights or not.
 * @details Based on the UserInfo
 * NSIS plug-in.
 */
BOOL WgxCheckAdminRights(void)
{
    HANDLE hToken;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = {SECURITY_NT_AUTHORITY};
    PSID psid = NULL;
    BOOL IsMember = FALSE;
    BOOL result = FALSE;
    
    if(!OpenThreadToken(GetCurrentThread(),TOKEN_QUERY,FALSE,&hToken)){
        letrace("cannot open access token of the thread");
        if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hToken)){
            letrace("cannot open access token of the process");
            return FALSE;
        }
    }
    
    if(!AllocateAndInitializeSid(&SystemSidAuthority,2,
      SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,
      0,0,0,0,0,0,&psid)){
        letrace("cannot create the security identifier");
        CloseHandle(hToken);
        return FALSE;
    }
      
    if(!CheckTokenMembership(NULL,psid,&IsMember)){
        letrace("cannot check token membership");
    } else {
        if(!IsMember){
            itrace("the user is not a member of administrators group");
        } else {
            result = TRUE;
        }
    }
    
    if(psid) FreeSid(psid);
    CloseHandle(hToken);
    return result;
}

/** @} */
