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
 * @file env.c
 * @brief Process environment.
 * @addtogroup Environment
 * @{
 */

#include "zenwinx.h"

/**
 * @brief Queries an environment variable.
 * @param[in] name the environment variable name.
 * @param[out] buffer pointer to the buffer receiving
 * the null-terminated value string.
 * @param[in] length the length of the buffer, in characters.
 * @return Zero for success, negative value otherwise.
 */
int winx_query_env_variable(wchar_t *name, wchar_t *buffer, int length)
{
    UNICODE_STRING n, v;
    NTSTATUS Status;
    
    DbgCheck3(name,buffer,(length > 0),"winx_query_env_variable",-1);

    RtlInitUnicodeString(&n,name);
    v.Buffer = buffer;
    v.Length = 0;
    v.MaximumLength = length * sizeof(wchar_t);
    Status = RtlQueryEnvironmentVariable_U(NULL,&n,&v);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_query_env_variable: cannot query %ws",name);
        return (-1);
    }
    return 0;
}

/**
 * @brief Sets an environment variable.
 * @param[in] name the environment variable name.
 * @param[in] value the null-terminated value string.
 * NULL pointer causes a variable deletion.
 * @return Zero for success, negative value otherwise.
 * @note value buffer size must not exceed 32767 characters,
 * including terminal zero, as mentioned in MSDN. This is
 * because unsigned short data type can hold numbers
 * less than or equal to 32767.
 */
int winx_set_env_variable(wchar_t *name, wchar_t *value)
{
    UNICODE_STRING n, v;
    NTSTATUS status;

    DbgCheck1(name,"winx_set_env_variable",-1);

    RtlInitUnicodeString(&n,name);
    if(value){
        if(value[0]){
            RtlInitUnicodeString(&v,value);
            status = RtlSetEnvironmentVariable(NULL,&n,&v);
        } else {
            status = RtlSetEnvironmentVariable(NULL,&n,NULL);
        }
    } else {
        status = RtlSetEnvironmentVariable(NULL,&n,NULL);
    }
    if(!NT_SUCCESS(status)){
        DebugPrintEx(status,"winx_set_env_variable: cannot set %ws",name);
        return (-1);
    }
    return 0;
}

/** @} */
