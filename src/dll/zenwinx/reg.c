/*
 *  ZenWINX - WIndows Native eXtended library.
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
 * @file reg.c
 * @brief System registry.
 * @addtogroup Registry
 * @{
 */

/*
* Use standard RtlXxx functions instead of writing 
* specialized versions of them.
*/

#include "zenwinx.h"

static int open_smss_key(HANDLE *pKey);
static int read_boot_exec_value(HANDLE hKey,void **data,DWORD *size);
static int write_boot_exec_value(HANDLE hKey,void *data,DWORD size);
static int cmd_compare(wchar_t *reg_cmd,wchar_t *cmd);
static void flush_smss_key(HANDLE hKey);

/*
* The following two routines replaces bootexctrl in native mode.
* It is not reasonable to use these routines to build bootexctrl
* application over however, since that win32 app uses all the
* benefits of win32 API such as error messages available for all
* error codes.
*/

/**
 * @brief Registers a command to be executed
 * during the Windows boot process.
 * @param[in] command the name of the command's
 * executable, without the extension.
 * @return Zero for success, negative value otherwise.
 * @note Command's executable must be placed inside 
 * a system32 directory to be executed successfully.
 */
int winx_register_boot_exec_command(wchar_t *command)
{
    HANDLE hKey;
    KEY_VALUE_PARTIAL_INFORMATION *data;
    DWORD size, value_size;
    wchar_t *value, *pos;
    DWORD length, i, len;
    
    DbgCheck1(command,-1);
    
    if(open_smss_key(&hKey) < 0) return (-1);
    size = (wcslen(command) + 1) * sizeof(wchar_t);
    if(read_boot_exec_value(hKey,(void **)(void *)&data,&size) < 0){
        NtCloseSafe(hKey);
        return (-1);
    }
    
    if(data->Type != REG_MULTI_SZ){
        etrace("BootExecute value has wrong type 0x%x",data->Type);
        winx_free((void *)data);
        NtCloseSafe(hKey);
        return (-1);
    }
    
    value = (wchar_t *)(data->Data);
    length = (data->DataLength >> 1) - 1;
    for(i = 0; i < length;){
        pos = value + i;
        //trace(D"%ws",pos);
        len = wcslen(pos) + 1;
        /* if the command is yet registered then exit */
        if(cmd_compare(pos,command) > 0)
            goto done;
        i += len;
    }
    wcscpy(value + i,command);
    value[i + wcslen(command) + 1] = 0;

    value_size = (i + wcslen(command) + 1 + 1) * sizeof(wchar_t);
    if(write_boot_exec_value(hKey,(void *)(data->Data),value_size) < 0){
        winx_free((void *)data);
        NtCloseSafe(hKey);
        return (-1);
    }

done:    
    winx_free((void *)data);
    flush_smss_key(hKey);
    NtCloseSafe(hKey);
    return 0;
}

/**
 * @brief Deregisters a command from being executed
 * during the Windows boot process.
 * @param[in] command the name of the command's
 * executable, without the extension.
 * @return Zero for success, negative value otherwise.
 */
int winx_unregister_boot_exec_command(wchar_t *command)
{
    HANDLE hKey;
    KEY_VALUE_PARTIAL_INFORMATION *data;
    DWORD size;
    wchar_t *value, *pos;
    DWORD length, i, len;
    wchar_t *new_value;
    DWORD new_value_size;
    DWORD new_length;
    
    DbgCheck1(command,-1);
    
    if(open_smss_key(&hKey) < 0) return (-1);
    size = (wcslen(command) + 1) * sizeof(wchar_t);
    if(read_boot_exec_value(hKey,(void **)(void *)&data,&size) < 0){
        NtCloseSafe(hKey);
        return (-1);
    }
    
    if(data->Type != REG_MULTI_SZ){
        etrace("BootExecute value has wrong type 0x%x",data->Type);
        winx_free((void *)data);
        NtCloseSafe(hKey);
        return (-1);
    }
    
    value = (wchar_t *)(data->Data);
    length = (data->DataLength >> 1) - 1;
    
    new_value_size = (length + 1) << 1;
    new_value = winx_malloc(new_value_size);
    if(!new_value){
        etrace("cannot allocate %u bytes of memory"
            "for the new BootExecute value",new_value_size);
        winx_free((void *)data);
        NtCloseSafe(hKey);
        return (-1);
    }

    memset((void *)new_value,0,new_value_size);
    new_length = 0;
    for(i = 0; i < length;){
        pos = value + i;
        //trace(D"%ws",pos);
        len = wcslen(pos) + 1;
        if(cmd_compare(pos,command) <= 0){
            wcscpy(new_value + new_length,pos);
            new_length += len;
        }
        i += len;
    }
    new_value[new_length] = 0;
    
    if(write_boot_exec_value(hKey,(void *)new_value,
      (new_length + 1) * sizeof(wchar_t)) < 0){
        winx_free((void *)new_value);
        winx_free((void *)data);
        NtCloseSafe(hKey);
        return (-1);
    }

    winx_free((void *)new_value);
    winx_free((void *)data);
    flush_smss_key(hKey);
    NtCloseSafe(hKey);
    return 0;
}

/**
 * @internal
 * @brief Opens the SMSS registry key.
 * @param[out] pKey pointer to the key handle.
 * @return Zero for success, negative value otherwise.
 */
static int open_smss_key(HANDLE *pKey)
{
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    NTSTATUS status;
    
    RtlInitUnicodeString(&us,L"\\Registry\\Machine\\SYSTEM\\"
                             L"CurrentControlSet\\Control\\Session Manager");
    InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
    status = NtOpenKey(pKey,KEY_QUERY_VALUE | KEY_SET_VALUE,&oa);
    if(status != STATUS_SUCCESS){
        strace(status,"cannot open %ws",us.Buffer);
        return (-1);
    }
    return 0;
}

/**
 * @internal
 * @brief Queries the BootExecute
 * value of the SMSS registry key.
 * @param[in] hKey the key handle.
 * @param[out] data pointer to a buffer that receives the value.
 * @param[in,out] size This paramenter must contain before the call
 * a number of bytes which must be allocated additionally to the size
 * of the BootExecute value. After the call this parameter contains
 * size of the allocated buffer containing the queried value, in bytes.
 * @return Zero for success, negative value otherwise.
 */
static int read_boot_exec_value(HANDLE hKey,void **data,DWORD *size)
{
    void *data_buffer = NULL;
    DWORD data_size = 0;
    DWORD data_size2 = 0;
    DWORD additional_space_size = *size;
    UNICODE_STRING us;
    NTSTATUS status;
    
    RtlInitUnicodeString(&us,L"BootExecute");
    status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
            NULL,0,&data_size);
    if(status != STATUS_BUFFER_TOO_SMALL){
        strace(status,"cannot query BootExecute value size");
        return (-1);
    }
    data_size += additional_space_size;
    data_buffer = winx_malloc(data_size);
    if(data_buffer == NULL){
        etrace("cannot allocate %u bytes of memory",data_size);
        return (-1);
    }
    
    RtlZeroMemory(data_buffer,data_size);
    status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
            data_buffer,data_size,&data_size2);
    if(status != STATUS_SUCCESS){
        strace(status,"cannot query BootExecute value");
        winx_free(data_buffer);
        return (-1);
    }
    
    *data = data_buffer;
    *size = data_size;
    return 0;
}

/**
 * @internal
 * @brief Sets the BootExecute value of the SMSS registry key.
 * @param[in] hKey the key handle.
 * @param[in] data pointer to a buffer containing the value.
 * @param[in] size the size of buffer, in bytes.
 * @return Zero for success, negative value otherwise.
 */
static int write_boot_exec_value(HANDLE hKey,void *data,DWORD size)
{
    UNICODE_STRING us;
    NTSTATUS status;
    
    RtlInitUnicodeString(&us,L"BootExecute");
    status = NtSetValueKey(hKey,&us,0,REG_MULTI_SZ,data,size);
    if(status != STATUS_SUCCESS){
        strace(status,"cannot set BootExecute value");
        return (-1);
    }
    
    return 0;
}

/**
 * @internal
 * @brief Compares two boot execute commands.
 * @details Treats 'command' and 'autocheck command' as the same.
 * @param[in] reg_cmd the command read from the registry.
 * @param[in] cmd the command to be searched for.
 * @return Positive value indicates that the commands are equal,
 * zero indicates that they're different, negative value
 * indicates a failure of the comparison.
 */
static int cmd_compare(wchar_t *reg_cmd,wchar_t *cmd)
{
    wchar_t *long_cmd;
    int size,result;
    
    /* do we have the command registered as it is? */
    if(!winx_wcsicmp(cmd,reg_cmd))
        return 1;
        
    /* compare reg_cmd with 'autocheck {cmd}' */
    winx_swprintf(long_cmd,size,result,L"autocheck %ws",cmd);
    if(long_cmd == NULL){
        mtrace();
        return (-1);
    }
        
    result = winx_wcsicmp(long_cmd,reg_cmd);
    winx_free(long_cmd);
    return (result == 0) ? 1 : 0;
}

/**
 * @internal
 * @brief Flushes the SMSS registry key.
 * @param[in] hKey the key handle.
 * @note
 * - Internal use only.
 * - Takes no effect in native apps before 
 * the system shutdown/reboot :-)
 */
static void flush_smss_key(HANDLE hKey)
{
    NTSTATUS status;
    
    status = NtFlushKey(hKey);
    if(status != STATUS_SUCCESS)
        strace(status,"cannot update Session Manager registry key on disk");
}

/** @} */
