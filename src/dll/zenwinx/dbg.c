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
 * @file dbg.c
 * @brief Debugging.
 * @details All the debugging messages will be delivered
 * to the Debug View program whenever possible. If logging
 * to the file is enabled, they will be saved there too.
 * Size of the log is limited by available memory. This
 * technique prevents the log file update on disk, thus
 * it makes the disk defragmentation more efficient.
 * @addtogroup Debug
 * @{
 */

#include "zenwinx.h"

void (__stdcall *pOutputDebugString)(char *msg) = NULL;

/* controls whether the messages will be collected or not */
int logging_enabled = 0;

/**
 * @internal
 * @brief Describes
 * a log list entry.
 */
typedef struct _winx_dbg_log_entry {
    struct _winx_dbg_log_entry *next;
    struct _winx_dbg_log_entry *prev;
    winx_time time_stamp;
    char *buffer;
} winx_dbg_log_entry;

/* all the messages will be collected to this list */
winx_dbg_log_entry *dbg_log = NULL;
/* synchronization event for the log list access */
winx_spin_lock *dbg_lock = NULL;

static int init_dbg_log(void);
static void close_dbg_log(void);

/**
 * @internal
 * @brief Initializes the
 * debugging facility.
 * @return Zero for success,
 * negative value otherwise.
 */
int winx_dbg_init(void)
{
    (void)winx_get_proc_address(
        L"kernel32.dll",
        "OutputDebugStringA",
        (void *)&pOutputDebugString
    );
    if(dbg_lock == NULL)
        dbg_lock = winx_init_spin_lock("winx_dbg_lock");
    if(dbg_lock == NULL)
        return (-1);
    return init_dbg_log();
}

/**
 * @internal
 * @brief Deinitializes
 * the debugging facility.
 */
void winx_dbg_close(void)
{
    close_dbg_log();
    winx_destroy_spin_lock(dbg_lock);
    dbg_lock = NULL;
}

/**
 * @internal
 * @brief Appends a string to the log list.
 */
static void add_dbg_log_entry(char *msg)
{
    winx_dbg_log_entry *new_log_entry = NULL;
    winx_dbg_log_entry *last_log_entry = NULL;

    /* synchronize with other threads */
    if(winx_acquire_spin_lock(dbg_lock,INFINITE) == 0){
        if(logging_enabled){
            if(dbg_log)
                last_log_entry = dbg_log->prev;
            new_log_entry = (winx_dbg_log_entry *)winx_list_insert_item((list_entry **)(void *)&dbg_log,
                (list_entry *)last_log_entry,sizeof(winx_dbg_log_entry));
            if(new_log_entry == NULL){
                /* not enough memory */
            } else {
                new_log_entry->buffer = winx_strdup(msg);
                if(new_log_entry->buffer == NULL){
                    /* not enough memory */
                    winx_list_remove_item((list_entry **)(void *)&dbg_log,(list_entry *)new_log_entry);
                } else {
                    memset(&new_log_entry->time_stamp,0,sizeof(winx_time));
                    (void)winx_get_local_time(&new_log_entry->time_stamp);
                }
            }
        }
        winx_release_spin_lock(dbg_lock);
    }
}

/**
 * @internal
 */
enum {
    ENC_ANSI,
    ENC_UTF8
};

/**
 * @internal
 * @brief Retuns error description
 * as UTF-8 encoded string.
 * @note Returned string should be
 * freed by winx_heap_free call.
 */
static char *get_description(ULONG error,int encoding)
{
    UNICODE_STRING uStr;
    NTSTATUS Status;
    HMODULE base_addr;
    MESSAGE_RESOURCE_ENTRY *mre;
    int bytes;
    char *desc;

    /* kernel32.dll has much better messages than ntdll.dll */
    RtlInitUnicodeString(&uStr,L"kernel32.dll");
    Status = LdrGetDllHandle(0,0,&uStr,&base_addr);
    if(!NT_SUCCESS(Status)){
        RtlInitUnicodeString(&uStr,L"ntdll.dll");
        Status = LdrGetDllHandle(0,0,&uStr,&base_addr);
        if(!NT_SUCCESS(Status))
            return NULL; /* this case is very extraordinary */
    }
    Status = RtlFindMessage(base_addr,(ULONG)(DWORD_PTR)RT_MESSAGETABLE,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),error,&mre);
    if(!NT_SUCCESS(Status))
        return NULL; /* no appropriate message found */
    if(mre->Flags & MESSAGE_RESOURCE_UNICODE){
        bytes = mre->Length - sizeof(MESSAGE_RESOURCE_ENTRY) + 1;
        desc = winx_heap_alloc(bytes * 2); /* enough to hold UTF-8 string */
        if(desc == NULL)
            return NULL;
        if(encoding == ENC_UTF8){
            winx_to_utf8(desc,bytes * 2,(wchar_t *)mre->Text);
        } else {
            _snprintf(desc,bytes * 2,"%ls",(wchar_t *)mre->Text);
            desc[bytes * 2 - 1] = 0;
        }
        return desc;
    } else {
        return winx_strdup((const char *)mre->Text);
    }
    return NULL;
}

/**
 * @brief Delivers a message to the Debug View
 * program and appends it to the log file as well.
 * @note
 * - If <b>: $LE</b> appears at end of the format
 * string, the last error code will be appended 
 * to the message as well as its description.
 * - If <b>: $NS</b> appears at end of the format
 * string, the NT status code of the last operation 
 * will be appended to the message as well as its
 * description.
 * - Not all system API set last status code.
 * Use winx_dbg_print_ex to catch the status for sure.
 */
void winx_dbg_print(char *format, ...)
{
    char *p, *msg = NULL;
    char *err_msg = NULL;
    char *ext_msg = NULL;
    char *ansi_err_msg = NULL;
    char *ansi_ext_msg = NULL;
    char *new_msg;
    ULONG status, error;
    int ns_flag = 0, le_flag = 0;
    int length;
    va_list arg;
    
    /* save last error codes */
    status = NtCurrentTeb()->LastStatusValue;
    error = NtCurrentTeb()->LastErrorValue;
    
    /* format message */
    if(format){
        va_start(arg,format);
        msg = winx_vsprintf(format,arg);
        va_end(arg);
    }
    if(!msg) return;
    
    /* get rid of trailing new line character */
    length = strlen(msg);
    if(length){
        if(msg[length - 1] == '\n')
            msg[length - 1] = 0;
    }

    /* check for $NS and $LE magic sequences */
    p = strstr(msg,": $NS");
    if(p == msg + length - 5){
        msg[length - 5] = 0;
        error = RtlNtStatusToDosError(status);
        ns_flag = 1;
    } else {
        p = strstr(msg,": $LE");
        if(p == msg + length - 5){
            msg[length - 5] = 0;
            le_flag = 1;
        }
    }
    
    if(ns_flag || le_flag){
        err_msg = get_description(error,ENC_UTF8);
        if(err_msg){
            /* get rid of trailing new line character */
            length = strlen(err_msg);
            if(length){
                if(err_msg[length - 1] == '\n')
                    err_msg[length - 1] = 0;
            }
            ext_msg = winx_sprintf("%s: 0x%x %s: %s",
                 msg,ns_flag ? (UINT)status : (UINT)error,
                 ns_flag ? "status" : "error",err_msg);
        } else {
            ext_msg = winx_sprintf("%s: 0x%x %s",
                 msg,ns_flag ? (UINT)status : (UINT)error,
                 ns_flag ? "status" : "error");
        }
        if(pOutputDebugString){
            ansi_err_msg = get_description(error,ENC_ANSI);
            if(ansi_err_msg){
                /* get rid of trailing new line character */
                length = strlen(ansi_err_msg);
                if(length){
                    if(ansi_err_msg[length - 1] == '\n')
                        ansi_err_msg[length - 1] = 0;
                }
                ansi_ext_msg = winx_sprintf("%s: 0x%x %s: %s\n",
                     msg,ns_flag ? (UINT)status : (UINT)error,
                     ns_flag ? "status" : "error",ansi_err_msg);
            } else {
                ansi_ext_msg = winx_sprintf("%s: 0x%x %s\n",
                     msg,ns_flag ? (UINT)status : (UINT)error,
                     ns_flag ? "status" : "error");
            }
        }
    }
    
    /* append message to the log */
    add_dbg_log_entry(ext_msg ? ext_msg : msg);
    
    /* send message to the Debug View program */
    if(pOutputDebugString){
        if(ansi_ext_msg){
            pOutputDebugString(ansi_ext_msg);
        } else {
            new_msg = winx_sprintf("%s\n",msg);
            if(new_msg){
                pOutputDebugString(new_msg);
                winx_heap_free(new_msg);
            }
        }
    }
    
    /* cleanup */
    winx_heap_free(msg);
    winx_heap_free(err_msg);
    winx_heap_free(ext_msg);
    winx_heap_free(ansi_err_msg);
    winx_heap_free(ansi_ext_msg);
}

/**
 * @brief Delivers a message to the Debug View
 * program and appends it to the log file as well.
 * @details Appends specified status code to the
 * message as well as its description.
 * @param[in] status the NT status code.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 * @par Example:
 * @code
 * NTSTATUS Status = NtCreateFile(...);
 * if(!NT_SUCCESS(Status)){
 *     winx_dbg_print_ex(Status,"Cannot create %s file",filename);
 * }
 * @endcode
 */
void winx_dbg_print_ex(unsigned long status,char *format, ...)
{
    va_list arg;
    char *msg;
    
    if(format){
        va_start(arg,format);
        msg = winx_vsprintf(format,arg);
        if(msg){
            NtCurrentTeb()->LastStatusValue = status;
            winx_dbg_print("%s: $NS",msg);
            winx_heap_free(msg);
        }
        va_end(arg);
    }
}

/**
 * @brief Delivers a message to the Debug View
 * program and appends it to the log file as well.
 * @brief Decorates the message by specified
 * character at both sides.
 * @param[in] ch the character to be used for decoration.
 * If zero value is passed, DEFAULT_DBG_PRINT_DECORATION_CHAR is used.
 * @param[in] width the width of the resulting message, in characters.
 * If zero value is passed, DEFAULT_DBG_PRINT_HEADER_WIDTH is used.
 * @param[in] format the format string.
 * @param[in] ... the parameters.
 */
void winx_dbg_print_header(char ch, int width, char *format, ...)
{
    va_list arg;
    char *string;
    int length;
    char *buffer;
    int left;
    
    if(ch == 0)
        ch = DEFAULT_DBG_PRINT_DECORATION_CHAR;
    if(width <= 0)
        width = DEFAULT_DBG_PRINT_HEADER_WIDTH;

    if(format){
        va_start(arg,format);
        string = winx_vsprintf(format,arg);
        if(string){
            length = strlen(string);
            if(length > (width - 4)){
                /* print string not decorated */
                winx_dbg_print("%s",string);
            } else {
                /* allocate buffer for entire string */
                buffer = winx_heap_alloc(width + 1);
                if(buffer == NULL){
                    /* print string not decorated */
                    winx_dbg_print("%s",string);
                } else {
                    /* fill buffer by character */
                    memset(buffer,ch,width);
                    buffer[width] = 0;
                    /* paste leading space */
                    left = (width - length - 2) / 2;
                    buffer[left] = 0x20;
                    /* paste string */
                    memcpy(buffer + left + 1,string,length);
                    /* paste closing space */
                    buffer[left + 1 + length] = 0x20;
                    /* print decorated string */
                    winx_dbg_print("%s",buffer);
                    winx_heap_free(buffer);
                }
            }
            winx_heap_free(string);
        }
        va_end(arg);
    }
}

/* the following code is intended to be used with winx_printf */

typedef struct _NT_STATUS_DESCRIPTION {
    unsigned long status;
    char *desc;
} NT_STATUS_DESCRIPTION, *PNT_STATUS_DESCRIPTION;

NT_STATUS_DESCRIPTION descriptions[] = {
    { STATUS_SUCCESS,                "operation successful"           },
    { STATUS_OBJECT_NAME_INVALID,    "object name invalid"            },
    { STATUS_OBJECT_NAME_NOT_FOUND,  "object name not found"          },
    { STATUS_OBJECT_NAME_COLLISION,  "object name already exists"     },
    { STATUS_OBJECT_PATH_INVALID,    "path is invalid"                },
    { STATUS_OBJECT_PATH_NOT_FOUND,  "path not found"                 },
    { STATUS_OBJECT_PATH_SYNTAX_BAD, "bad syntax in path"             },
    { STATUS_BUFFER_TOO_SMALL,       "buffer is too small"            },
    { STATUS_ACCESS_DENIED,          "access denied"                  },
    { STATUS_NO_MEMORY,              "not enough memory"              },
    { STATUS_UNSUCCESSFUL,           "operation failed"               },
    { STATUS_NOT_IMPLEMENTED,        "not implemented"                },
    { STATUS_INVALID_INFO_CLASS,     "invalid info class"             },
    { STATUS_INFO_LENGTH_MISMATCH,   "info length mismatch"           },
    { STATUS_ACCESS_VIOLATION,       "access violation"               },
    { STATUS_INVALID_HANDLE,         "invalid handle"                 },
    { STATUS_INVALID_PARAMETER,      "invalid parameter"              },
    { STATUS_NO_SUCH_DEVICE,         "device not found"               },
    { STATUS_NO_SUCH_FILE,           "file not found"                 },
    { STATUS_INVALID_DEVICE_REQUEST, "invalid device request"         },
    { STATUS_END_OF_FILE,            "end of file reached"            },
    { STATUS_WRONG_VOLUME,           "wrong volume"                   },
    { STATUS_NO_MEDIA_IN_DEVICE,     "no media in device"             },
    { STATUS_UNRECOGNIZED_VOLUME,    "cannot recognize file system"   },
    { STATUS_VARIABLE_NOT_FOUND,     "environment variable not found" },
    
    /* A file cannot be opened because the share access flags are incompatible. */
    { STATUS_SHARING_VIOLATION,      "file is locked by another process"},
    
    /* A file cannot be moved because target clusters are in use. */
    { STATUS_ALREADY_COMMITTED,      "target clusters are already in use"},
    
    { 0xffffffff,                    NULL                             }
};

/**
 * @internal
 * @brief Retrieves a human readable
 * explanation of the NT status code.
 * @param[in] status the NT status code.
 * @return A pointer to string containing
 * the status explanation.
 * @note This function returns explanations
 * only for well known codes. Otherwise 
 * it returns an empty string.
 * @par Example:
 * @code
 * printf("%s\n",winx_get_error_description(STATUS_ACCESS_VIOLATION));
 * // prints "access violation"
 * @endcode
 */
char *winx_get_error_description(unsigned long status)
{
    int i;
    
    for(i = 0; descriptions[i].desc; i++){
        if(descriptions[i].status == status)
            return descriptions[i].desc;
    }
    return "";
}

/* logging to the file */

char *log_path = NULL;

/* synchronization event for the log path access */
winx_spin_lock *path_lock = NULL;

/**
 * @internal
 * @brief Initializes logging to the file.
 * @return Zero for success, negative value otherwise.
 */
static int init_dbg_log(void)
{
    if(path_lock == NULL)
        path_lock = winx_init_spin_lock("winx_dbg_logpath_lock");
    return (path_lock == NULL) ? (-1) : (0);
}

/**
 * @internal
 * @brief Appends all collected debugging
 * information to the log file.
 * @param[in] already_synchronized an internal
 * flag, used in winx_enable_dbg_log only.
 * Should be always set to zero in other cases.
 */
static void flush_dbg_log(int already_synchronized)
{
    #define DBG_BUFFER_SIZE (100 * 1024) /* 100 Kb */
    WINX_FILE *f;
    char *lb;
    winx_dbg_log_entry *old_dbg_log, *log_entry;
    int length;
    char crlf[] = "\r\n";
    char bom[4] = {0xEF, 0xBB, 0xBF, 0x00};
    char *time_stamp;

    /* synchronize with other threads */
    if(!already_synchronized){
        if(winx_acquire_spin_lock(path_lock,INFINITE) < 0){
            DebugPrint("flush_dbg_log: synchronization failed");
            winx_print("\nflush_dbg_log: synchronization failed!\n");
            return;
        }
    }
    
    /* disable parallel access to dbg_log list */
    if(winx_acquire_spin_lock(dbg_lock,INFINITE) < 0){
        if(!already_synchronized)
            winx_release_spin_lock(path_lock);
        return;
    }
    old_dbg_log = dbg_log;
    dbg_log = NULL;
    winx_release_spin_lock(dbg_lock);
    
    if(!old_dbg_log || !log_path)
        goto done;
    
    if(log_path[0] == 0)
        goto done;
    
    /* open log file */
    f = winx_fbopen(log_path,"a",DBG_BUFFER_SIZE);
    if(f == NULL){
        /* recreate path if it does not exist */
        lb = strrchr(log_path,'\\');
        if(lb) *lb = 0;
        if(winx_create_path(log_path) < 0){
            DebugPrint("flush_old_dbg_log: cannot create directory tree for log path");
            winx_print("\nflush_old_dbg_log: cannot create directory tree for log path\n");
        }
        if(lb) *lb = '\\';
        f = winx_fbopen(log_path,"a",DBG_BUFFER_SIZE);
    }

    /* save log */
    if(f != NULL){
        winx_printf("\nWriting log file \"%s\" ...\n",&log_path[4]);
        /*
        * UTF-8 encoded files need BOM to be written before the contents;
        * multiple occurencies of BOM inside the file contens are allowed.
        */
        (void)winx_fwrite(bom,sizeof(char),3,f);
        for(log_entry = old_dbg_log; log_entry; log_entry = log_entry->next){
            if(log_entry->buffer){
                length = strlen(log_entry->buffer);
                if(length){
                    time_stamp = winx_sprintf("%04d-%02d-%02d %02d:%02d:%02d.%03d ",
                        log_entry->time_stamp.year, log_entry->time_stamp.month, log_entry->time_stamp.day,
                        log_entry->time_stamp.hour, log_entry->time_stamp.minute, log_entry->time_stamp.second,
                        log_entry->time_stamp.milliseconds);
                    if(time_stamp){
                        (void)winx_fwrite(time_stamp,sizeof(char),strlen(time_stamp),f);
                        winx_heap_free(time_stamp);
                    }
                    (void)winx_fwrite(log_entry->buffer,sizeof(char),length,f);
                    /* add a proper newline characters */
                    (void)winx_fwrite(crlf,sizeof(char),2,f);
                }
            }
            if(log_entry->next == old_dbg_log) break;
        }
        winx_fclose(f);
        winx_list_destroy((list_entry **)(void *)&old_dbg_log);
    }

done:
    /* end of synchronization */
    if(!already_synchronized)
        winx_release_spin_lock(path_lock);
}

/**
 * @brief Appends all collected debugging
 * information to the log file.
 */
void winx_flush_dbg_log(void)
{
    flush_dbg_log(0);
}

/**
 * @brief Enables debug logging to the file.
 * @param[in] path the path to the logfile.
 * NULL or an empty string forces to flush
 * all collected data to the disk and disable
 * logging to the file.
 */
void winx_enable_dbg_log(char *path)
{
    if(path == NULL){
        logging_enabled = 0;
    } else {
        logging_enabled = path[0] ? 1 : 0;
    }
    
    /* synchronize with other threads */
    if(winx_acquire_spin_lock(path_lock,INFINITE) < 0){
        DebugPrint("winx_enable_dbg_log: synchronization failed");
        winx_print("\nwinx_enable_dbg_log: synchronization failed!\n");
        return;
    }
    
    /* flush old log to disk */
    if(path || log_path){
        if(!path || !log_path)
            flush_dbg_log(1);
        else if(strcmp(path,log_path))
            flush_dbg_log(1);
    }
    
    /* set new log path */
    if(log_path){
        winx_heap_free(log_path);
        log_path = NULL;
    }
    if(logging_enabled){
        DebugPrint("winx_enable_dbg_log: log_path = %s",path);
        winx_printf("\nUsing log file \"%s\" ...\n",&path[4]);
        log_path = winx_strdup(path);
        if(log_path == NULL){
            DebugPrint("winx_enable_dbg_log: cannot allocate memory for log path");
            winx_print("\nCannot allocate memory for log path!\n");
        }
    }
    
    /* end of synchronization */
    winx_release_spin_lock(path_lock);
}

/**
 * @brief Disables debug logging to the file.
 */
void winx_disable_dbg_log(void)
{
    winx_enable_dbg_log(NULL);
}

/**
 * @internal
 * @brief Deinitializes logging to the file.
 */
static void close_dbg_log(void)
{
    winx_flush_dbg_log();
    if(log_path){
        winx_heap_free(log_path);
        log_path = NULL;
    }
    winx_destroy_spin_lock(path_lock);
    path_lock = NULL;
}

/** @} */
