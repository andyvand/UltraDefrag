/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010-2012 Stefan Pendl (stefanpe@users.sourceforge.net).
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
 * @file keyboard.c
 * @brief Keyboard input.
 * @addtogroup Keyboard
 * @{
 */

#include "zenwinx.h"

/*
* This delay affects primarily text typing
* speed when winx_prompt_ex() is used to get
* user input.
*/
#define MAX_TYPING_DELAY 10 /* msec */

#define MAX_NUM_OF_KEYBOARDS 100

/*
* Keyboard queue is needed to keep all
* key hits, including composite ones
* like Pause/Break.
*/
#define KB_QUEUE_LENGTH      100

typedef struct _KEYBOARD {
    int device_number; /* for debugging purposes */
    HANDLE hKbDevice;
    HANDLE hKbEvent;
} KEYBOARD, *PKEYBOARD;

KEYBOARD kb[MAX_NUM_OF_KEYBOARDS] = {{0}};
int number_of_keyboards = 0;

HANDLE hKbSynchEvent = NULL;
KEYBOARD_INPUT_DATA kids[KB_QUEUE_LENGTH] = {{0}};
int start_index = 0;
int n_written = 0;
int stop_kb_wait_for_input = 0;
int kb_wait_for_input_threads = 0;
#define STOP_KB_WAIT_INTERVAL 100 /* ms */

/* prototypes */
void kb_close(void);
static int kb_check(HANDLE hKbDevice);
static int kb_open_internal(int device_number, int kbd_count);
char *winx_get_status_description(unsigned long status);
static int query_keyboard_count(void);

/**
 * @internal
 * @brief Waits for user input on
 * the specified keyboard.
 */
static DWORD WINAPI kb_wait_for_input(LPVOID p)
{
    LARGE_INTEGER ByteOffset;
    IO_STATUS_BLOCK iosb;
    KEYBOARD_INPUT_DATA kid;
    NTSTATUS Status;
    LARGE_INTEGER interval;
    LARGE_INTEGER synch_interval;
    int index;
    char buffer[128];
    KEYBOARD *kbd = (KEYBOARD *)p;
    
    /*
    * Either debug print or winx_printf,
    * or memory allocation aren't available here...
    */
    
    kb_wait_for_input_threads ++;
    interval.QuadPart = -((signed long)STOP_KB_WAIT_INTERVAL * 10000);

    while(!stop_kb_wait_for_input){
        ByteOffset.QuadPart = 0;
        /* make a read request */
        Status = NtReadFile(kbd->hKbDevice,kbd->hKbEvent,NULL,NULL,
            &iosb,&kid,sizeof(KEYBOARD_INPUT_DATA),&ByteOffset,0);
        /* wait for key hits */
        if(NT_SUCCESS(Status)){
            do {
                Status = NtWaitForSingleObject(kbd->hKbEvent,FALSE,&interval);
                if(stop_kb_wait_for_input){
                    if(Status == STATUS_TIMEOUT){
                        /* cancel the pending operation */
                        Status = NtCancelIoFile(kbd->hKbDevice,&iosb);
                        if(NT_SUCCESS(Status)){
                            Status = NtWaitForSingleObject(kbd->hKbEvent,FALSE,NULL);
                            if(NT_SUCCESS(Status)) Status = iosb.Status;
                        }
                        if(!NT_SUCCESS(Status)){
                            _snprintf(buffer,sizeof(buffer),"\nNtCancelIoFile for KeyboadClass%u failed: %x!\n%s\n",
                                kbd->device_number,(UINT)Status,winx_get_status_description((ULONG)Status));
                            buffer[sizeof(buffer) - 1] = 0;
                            winx_print(buffer);
                        }
                    }
                    goto done;
                }
            } while(Status == STATUS_TIMEOUT);
            if(NT_SUCCESS(Status)) Status = iosb.Status;
        }
        /* here we have either an input gathered or an error */
        if(!NT_SUCCESS(Status)){
            _snprintf(buffer,sizeof(buffer),"\nCannot read the KeyboadClass%u device: %x!\n%s\n",
                kbd->device_number,(UINT)Status,winx_get_status_description((ULONG)Status));
            buffer[sizeof(buffer) - 1] = 0;
            winx_print(buffer);
            goto done;
        } else {
            /* synchronize with other threads */
            if(hKbSynchEvent){
                synch_interval.QuadPart = MAX_WAIT_INTERVAL;
                Status = NtWaitForSingleObject(hKbSynchEvent,FALSE,&synch_interval);
                if(Status != WAIT_OBJECT_0){
                    _snprintf(buffer,sizeof(buffer),"\nkb_wait_for_input: synchronization failed: %x!\n%s\n",
                        (UINT)Status,winx_get_status_description((ULONG)Status));
                    buffer[sizeof(buffer) - 1] = 0;
                    winx_print(buffer);
                }
            }

            /* push new item to the keyboard queue */
            if(start_index < 0 || start_index >= KB_QUEUE_LENGTH){
                winx_print("\nkb_wait_for_input: unexpected condition #1!\n\n");
                start_index = 0;
            }
            if(n_written < 0 || n_written > KB_QUEUE_LENGTH){
                winx_print("\nkb_wait_for_input: unexpected condition #2!\n\n");
                n_written = 0;
            }

            index = start_index + n_written;
            if(index >= KB_QUEUE_LENGTH)
                index -= KB_QUEUE_LENGTH;

            if(n_written == KB_QUEUE_LENGTH)
                start_index ++;
            else
                n_written ++;
            
            memcpy(&kids[index],&kid,sizeof(KEYBOARD_INPUT_DATA));
            
            /* release synchronization event */
            if(hKbSynchEvent)
                (void)NtSetEvent(hKbSynchEvent,NULL);
        }
    }

done:
    kb_wait_for_input_threads --;
    winx_exit_thread(0);
    return 0;
}

/**
 * @brief Prepares all existing keyboards
 * for work with user input related procedures.
 * @details If checking of first keyboard fails
 * it waits ten seconds for the initialization.
 * This is needed for wireless devices.
 * @return Zero for success, negative value otherwise.
 */
int winx_kb_init(void)
{
    wchar_t event_name[64];
    int i, j, kbdCount;

    kbdCount = query_keyboard_count();

    /* create synchronization event for safe access to kids array */
    _snwprintf(event_name,64,L"\\winx_kb_synch_event_%u",
        (unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess));
    event_name[63] = 0;
    
    if(hKbSynchEvent == NULL){
        (void)winx_create_event(event_name,SynchronizationEvent,&hKbSynchEvent);
        if(hKbSynchEvent) (void)NtSetEvent(hKbSynchEvent,NULL);
    }
    
    if(hKbSynchEvent == NULL){
        winx_printf("\nCannot create %ws event!\n\n",event_name);
        return (-1);
    }

    /* initialize kb array */
    memset((void *)kb,0,sizeof(kb));
    number_of_keyboards = 0;
    
    /* check all the keyboards and wait ten seconds
       for any keyboard that fails detection.
       required for USB devices, which can change ports */
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++) {
        if (kb_open_internal(i, kbdCount) == -1) {
            if (i < kbdCount) {
                winx_printf("Wait 10 seconds for keyboard initialization ");
                
                for(j = 0; j < 10; j++){
                    winx_sleep(1000);
                    winx_printf(".");
                }
                winx_printf("\n\n");

                (void)kb_open_internal(i, kbdCount);
            }
        }
    }
    
    /* start threads waiting for user input */
    stop_kb_wait_for_input = 0;
    kb_wait_for_input_threads = 0;
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++){
        if(kb[i].hKbDevice == NULL) break;
        if(winx_create_thread(kb_wait_for_input,(LPVOID)&kb[i]) < 0){
            winx_printf("\nCannot create thread gathering input from \\Device\\KeyboardClass%u\n\n",
                kb[i].device_number);
            /* stop all threads */
            stop_kb_wait_for_input = 1;
            while(kb_wait_for_input_threads)
                winx_sleep(STOP_KB_WAIT_INTERVAL);
            return (-1);
        }
    }
    
    if(kb[0].hKbDevice) return 0; /* success, at least one keyboard found */
    else return (-1);
}

/**
 * @internal
 * @brief Closes all opened keyboards.
 */
void kb_close(void)
{
    int i;
    
    /*
    * Either debug print or memory
    * allocation calls aren't available here...
    */
    
    /* stop threads waiting for user input */
    stop_kb_wait_for_input = 1; i = 0;
    /* 3 sec should be enough; otherwise sometimes it hangs */
    while(kb_wait_for_input_threads && i < 30){
        winx_sleep(STOP_KB_WAIT_INTERVAL);
        i++;
    }
    if(kb_wait_for_input_threads){
        winx_printf("Keyboards polling terminated forcibly...\n");
        winx_sleep(2000);
    }
    
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++){
        if(kb[i].hKbDevice == NULL) break;
        NtCloseSafe(kb[i].hKbDevice);
        NtCloseSafe(kb[i].hKbEvent);
        /* don't reset device_number member here */
        number_of_keyboards --;
    }
    
    /* destroy synchronization event */
    winx_destroy_event(hKbSynchEvent);
}

/**
 * @internal
 * @brief Checks the console for keyboard input.
 * @details Tries to read from all keyboard devices 
 * until specified time-out expires.
 * @param[out] pKID pointer to the structure receiving keyboard input.
 * @param[in] msec_timeout time-out interval in milliseconds.
 * @return Zero if some key was pressed, negative value otherwise.
 */
int kb_read(PKEYBOARD_INPUT_DATA pKID,int msec_timeout)
{
    int attempts = 0;
    ULONGLONG xtime = 0;
    LARGE_INTEGER synch_interval;
    NTSTATUS Status;
    
    DbgCheck1(pKID,-1);
    
    if(msec_timeout != INFINITE){
        attempts = msec_timeout / MAX_TYPING_DELAY + 1;
        xtime = winx_xtime();
    }
    
    while(number_of_keyboards){
        /* synchronize with other threads */
        if(hKbSynchEvent){
            synch_interval.QuadPart = MAX_WAIT_INTERVAL;
            Status = NtWaitForSingleObject(hKbSynchEvent,FALSE,&synch_interval);
            if(Status != WAIT_OBJECT_0){
                winx_printf("\nkb_read: synchronization failed: 0x%x\n",(UINT)Status);
                winx_printf("%s\n\n",winx_get_status_description((ULONG)Status));
            }
        }

        /* pop item from the keyboard queue */
        if(n_written > 0){
            memcpy(pKID,&kids[start_index],sizeof(KEYBOARD_INPUT_DATA));
            start_index ++;
            if(start_index >= KB_QUEUE_LENGTH)
                start_index = 0;
            n_written --;
            if(hKbSynchEvent)
                (void)NtSetEvent(hKbSynchEvent,NULL);
            return 0;
        }

        /* release synchronization event */
        if(hKbSynchEvent)
            (void)NtSetEvent(hKbSynchEvent,NULL);

        winx_sleep(MAX_TYPING_DELAY);
        if(msec_timeout != INFINITE){
            attempts --;
            if(attempts == 0) break;
            if(xtime && (winx_xtime() - xtime >= msec_timeout)) break;
        }
    }
    return (-1);
}

/*
**************************************************************
*                   internal functions                       *
**************************************************************
*/

/**
 * @internal
 * @brief Opens the keyboard.
 * @param[in] device_number the number of the keyboard device.
 * @param[in] kbd_count the number of registered keyboards.
 * @return Zero for success, negative value otherwise.
 */
static int kb_open_internal(int device_number, int kbd_count)
{
    wchar_t device_name[32];
    wchar_t event_name[32];
    UNICODE_STRING uStr;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS status;
    HANDLE hKbDevice = NULL;
    HANDLE hKbEvent = NULL;
    int i;

    (void)_snwprintf(device_name,32,L"\\Device\\KeyboardClass%u",device_number);
    device_name[31] = 0;
    RtlInitUnicodeString(&uStr,device_name);
    InitializeObjectAttributes(&ObjectAttributes,&uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
    status = NtCreateFile(&hKbDevice,
                GENERIC_READ | FILE_RESERVE_OPFILTER | FILE_READ_ATTRIBUTES/*0x80100080*/,
                &ObjectAttributes,&IoStatusBlock,NULL,FILE_ATTRIBUTE_NORMAL/*0x80*/,
                0,FILE_OPEN/*1*/,FILE_DIRECTORY_FILE/*1*/,NULL,0);
    if(!NT_SUCCESS(status)){
        if(device_number < kbd_count){
            strace(status,"cannot open %ws",device_name);
            winx_printf("\nCannot open the keyboard %ws: %x!\n",
                device_name,(UINT)status);
            winx_printf("%s\n",winx_get_status_description((ULONG)status));
        }
        return (-1);
    }
    
    /* ensure that we have opened a really connected keyboard */
    if(kb_check(hKbDevice) < 0){
        strace(status,"invalid keyboard device %ws",device_name);
        winx_printf("\nInvalid keyboard device %ws: %x!\n",device_name,(UINT)status);
        winx_printf("%s\n",winx_get_status_description((ULONG)status));
        NtCloseSafe(hKbDevice);
        return (-1);
    }
    
    /* create a special event object for internal use */
    (void)_snwprintf(event_name,32,L"\\kb_event%u",device_number);
    event_name[31] = 0;
    RtlInitUnicodeString(&uStr,event_name);
    InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
    status = NtCreateEvent(&hKbEvent,STANDARD_RIGHTS_ALL | 0x1ff/*0x1f01ff*/,
        &ObjectAttributes,SynchronizationEvent,0/*FALSE*/);
    if(!NT_SUCCESS(status)){
        NtCloseSafe(hKbDevice);
        strace(status,"cannot create kb_event%u",device_number);
        winx_printf("\nCannot create kb_event%u: %x!\n",device_number,(UINT)status);
        winx_printf("%s\n",winx_get_status_description((ULONG)status));
        return (-1);
    }
    
    /* add information to kb array */
    for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++){
        if(kb[i].hKbDevice == NULL){
            kb[i].hKbDevice = hKbDevice;
            kb[i].hKbEvent = hKbEvent;
            kb[i].device_number = device_number;
            number_of_keyboards ++;
            winx_printf("Keyboard device found: %ws.\n",device_name);
            return 0;
        }
    }

    winx_printf("\nkb array is full!\n");
    return (-1);
}

#define LIGHTING_REPEAT_COUNT 0x5

/**
 * @internal
 * @brief Lights up the keyboard indicators.
 * @param[in] hKbDevice the handle of the keyboard device.
 * @param[in] LedFlags the flags specifying
 * which indicators must be lighten up.
 * @return Zero for success, negative value otherwise.
 */
static int kb_light_up_indicators(HANDLE hKbDevice,USHORT LedFlags)
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    KEYBOARD_INDICATOR_PARAMETERS kip;

    kip.LedFlags = LedFlags;
    kip.UnitId = 0;

    status = NtDeviceIoControlFile(hKbDevice,NULL,NULL,NULL,
            &iosb,IOCTL_KEYBOARD_SET_INDICATORS,
            &kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS),NULL,0);
    if(NT_SUCCESS(status)){
        status = NtWaitForSingleObject(hKbDevice,FALSE,NULL);
        if(NT_SUCCESS(status)) status = iosb.Status;
    }
    if(!NT_SUCCESS(status) || status == STATUS_PENDING) return (-1);
    
    return 0;
}

/**
 * @internal
 * @brief Checks the keyboard for existence.
 * @param[in] hKbDevice the handle of the keyboard device.
 * @return Zero for success, negative value otherwise.
 */
static int kb_check(HANDLE hKbDevice)
{
    USHORT LedFlags;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    KEYBOARD_INDICATOR_PARAMETERS kip;
    int i;
    
    /* try to get LED flags */
    RtlZeroMemory(&kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS));
    status = NtDeviceIoControlFile(hKbDevice,NULL,NULL,NULL,
            &iosb,IOCTL_KEYBOARD_QUERY_INDICATORS,NULL,0,
            &kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS));
    if(NT_SUCCESS(status)){
        status = NtWaitForSingleObject(hKbDevice,FALSE,NULL);
        if(NT_SUCCESS(status)) status = iosb.Status;
    }
    if(!NT_SUCCESS(status) || status == STATUS_PENDING) return (-1);

    LedFlags = kip.LedFlags;
    
    /* light up LED's */
    for(i = 0; i < LIGHTING_REPEAT_COUNT; i++){
        (void)kb_light_up_indicators(hKbDevice,KEYBOARD_NUM_LOCK_ON);
        winx_sleep(100);
        (void)kb_light_up_indicators(hKbDevice,KEYBOARD_CAPS_LOCK_ON);
        winx_sleep(100);
        (void)kb_light_up_indicators(hKbDevice,KEYBOARD_SCROLL_LOCK_ON);
        winx_sleep(100);
    }

    (void)kb_light_up_indicators(hKbDevice,LedFlags);
    return 0;
}

/**
 * @internal
 * @brief Collects and logs all the values of a registry key.
 */
void winx_log_reg_key_info(wchar_t *key_name)
{
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    NTSTATUS status;
    HANDLE hKey;
    KEY_FULL_INFORMATION *info_buffer = NULL;
    KEY_VALUE_FULL_INFORMATION *value_buffer = NULL;
    DWORD data_size = 0;
    DWORD data_size2 = 0;
    DWORD value_size = 0;
    DWORD value_size2 = 0;
    wchar_t *value_name = NULL;
    wchar_t *value_string_data = NULL;
    int value_dword_data = 0;
    int i;

    RtlInitUnicodeString(&us,key_name);
    InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
    itrace("enumerating %ws",key_name);
    status = NtOpenKey(&hKey,KEY_READ,&oa);
    if(status != STATUS_SUCCESS){
        strace(status,"cannot open %ws",key_name);
        return;
    }

    status = NtQueryKey(hKey,KeyFullInformation,NULL,0,&data_size);
    if(status != STATUS_BUFFER_TOO_SMALL){
        strace(status,"cannot query information size");
        NtCloseSafe(hKey);
        return;
    }
    info_buffer = (KEY_FULL_INFORMATION *)winx_malloc(data_size);
    if(info_buffer == NULL){
        etrace("cannot allocate %u bytes of memory",data_size);
        NtCloseSafe(hKey);
        return;
    }

    RtlZeroMemory(info_buffer,data_size);
    status = NtQueryKey(hKey,KeyFullInformation,info_buffer,data_size,&data_size2);
    if(status != STATUS_SUCCESS){
        strace(status,"cannot query information");
        winx_free(info_buffer);
        NtCloseSafe(hKey);
        return;
    }

    itrace("values count = %u",info_buffer->Values);
    itrace("max value name length = %u",info_buffer->MaxValueNameLen);
    itrace("max value data length = %u",info_buffer->MaxValueDataLen);

    value_name = (wchar_t *)winx_malloc(info_buffer->MaxValueNameLen);
    if(value_name == NULL){
        etrace("cannot allocate %u bytes of memory",info_buffer->MaxValueNameLen);
        winx_free(info_buffer);
        NtCloseSafe(hKey);
        return;
    }

    value_string_data = (wchar_t *)winx_malloc(info_buffer->MaxValueDataLen);
    if(value_string_data == NULL){
        etrace("cannot allocate %u bytes of memory",info_buffer->MaxValueDataLen);
        winx_free(value_name);
        winx_free(info_buffer);
        NtCloseSafe(hKey);
        return;
    }

    for(i = 0; i < (int)info_buffer->Values; i++) {
        status = NtEnumerateValueKey(hKey,i,KeyValueFullInformation,
                NULL,0,&value_size);
        if(status != STATUS_BUFFER_TOO_SMALL){
            strace(status,"cannot query full information size");
            break;
        }
        value_buffer = (KEY_VALUE_FULL_INFORMATION *)winx_malloc(value_size);
        if(value_buffer == NULL){
            etrace("cannot allocate %u bytes of memory",value_size);
            break;
        }

        RtlZeroMemory(value_buffer,value_size);
        status = NtEnumerateValueKey(hKey,i,KeyValueFullInformation,
                value_buffer,value_size,&value_size2);
        if(status != STATUS_SUCCESS){
            strace(status,"cannot query full information");
            winx_free(value_buffer);
            break;
        }
        RtlCopyMemory(value_name,value_buffer->Name,value_buffer->NameLength);
        value_name[value_buffer->NameLength / sizeof(wchar_t)] = 0;

        switch(value_buffer->Type){
            case REG_DWORD:
                value_dword_data = (int)*(DWORD *)((BYTE *)value_buffer + value_buffer->DataOffset);
                itrace("value %d = %ws ... %d",i,value_name,value_dword_data);
                break;
            case REG_EXPAND_SZ:
            case REG_SZ:
                RtlCopyMemory(value_string_data,(BYTE *)value_buffer + value_buffer->DataOffset,value_buffer->DataLength);
                value_string_data[value_buffer->DataLength / sizeof(wchar_t)] = 0;
                itrace("value %d = %ws ... %ws",i,value_name,value_string_data);
                break;
        }

        winx_free(value_buffer);
    }
    trace(I"----------");

    winx_free(value_string_data);
    winx_free(value_name);
    winx_free(info_buffer);
    NtCloseSafe(hKey);
}

/**
 * @internal
 * @brief Collects and logs all the keyboard registry information.
 */
void winx_log_keyboard_info(void)
{
    wchar_t *ControlSetKeys[] = {
        L"\\Registry\\Machine\\SYSTEM\\ControlSet002\\Services\\%ws\\Enum",
        L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Services\\%ws\\Enum",
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\%ws\\Enum"
    };
    wchar_t path[MAX_PATH + 1]; /* should be enough */

    int i;

    winx_log_reg_key_info(L"\\Registry\\Machine\\HARDWARE\\DEVICEMAP\\KeyboardClass");

    for(i = 0; i < sizeof(ControlSetKeys) / sizeof(wchar_t *); i++){
        _snwprintf(path,MAX_PATH + 1,ControlSetKeys[i],L"Kbdclass");
        path[MAX_PATH] = 0;

        winx_log_reg_key_info(path);
    }
    for(i = 0; i < sizeof(ControlSetKeys) / sizeof(wchar_t *); i++){
        _snwprintf(path,MAX_PATH + 1,ControlSetKeys[i],L"kbdhid");
        path[MAX_PATH] = 0;

        winx_log_reg_key_info(path);
    }
}

/**
 * @internal
 * @brief Queries the registry for the total number
 * of installed devices of the specified class.
 * @param[in] class_name the class name.
 * @param[in] default_value the default number of
 * the installed devices, intended for being returned
 * on impossibility to query the real information.
 */
static int winx_query_dev_count(wchar_t *class_name,int default_value)
{
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES oa;
    NTSTATUS status;
    HANDLE hKey;
    KEY_VALUE_PARTIAL_INFORMATION *data_buffer = NULL;
    DWORD data_size = 0;
    DWORD data_size2 = 0;
    
    wchar_t *ControlSetKeys[] = {
        L"\\Registry\\Machine\\SYSTEM\\ControlSet002\\Services\\%ws\\Enum",
        L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Services\\%ws\\Enum",
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\%ws\\Enum"
    };
    wchar_t path[MAX_PATH + 1]; /* should be enough */

    int i, count = default_value;

    for(i = 0; i < sizeof(ControlSetKeys) / sizeof(wchar_t *); i++){
        _snwprintf(path,MAX_PATH + 1,ControlSetKeys[i],class_name);
        path[MAX_PATH] = 0;
        RtlInitUnicodeString(&us,path);
        InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
        itrace("checking %ws",path);
        status = NtOpenKey(&hKey,KEY_READ,&oa);
        if(status != STATUS_SUCCESS){
            strace(status,"cannot open %ws",path);
            continue;
        }

        RtlInitUnicodeString(&us,L"Count");
        status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
                NULL,0,&data_size);
        if(status != STATUS_BUFFER_TOO_SMALL){
            strace(status,"cannot query Count value size");
            NtCloseSafe(hKey);
            continue;
        }
        data_buffer = (KEY_VALUE_PARTIAL_INFORMATION *)winx_malloc(data_size);
        if(data_buffer == NULL){
            etrace("cannot allocate %u bytes of memory",data_size);
            NtCloseSafe(hKey);
            continue;
        }

        RtlZeroMemory(data_buffer,data_size);
        status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
                data_buffer,data_size,&data_size2);
        if(status != STATUS_SUCCESS){
            strace(status,"cannot query Count value");
            winx_free(data_buffer);
            NtCloseSafe(hKey);
            continue;
        }

        if(data_buffer->Type != REG_DWORD){
            etrace("Count value has wrong type 0x%x",
                    data_buffer->Type);
            winx_free(data_buffer);
            NtCloseSafe(hKey);
            continue;
        }

        itrace("devices count = %u",
            (int)*(DWORD *)data_buffer->Data);
        count = max(count, (int)*(DWORD *)data_buffer->Data);

        winx_free(data_buffer);
        NtCloseSafe(hKey);
    }

    itrace("total devices count = %u",count);
    return count;
}

/**
 * @internal
 * @brief Queries the registry for 
 * the number of installed keyboards.
 */
static int query_keyboard_count(void)
{
    int count;
    int total = winx_query_dev_count(L"Kbdclass",1);
    int hid = winx_query_dev_count(L"kbdhid",0);

    /* Windows XP sometimes doesn't count the USB keyboards */
    if (winx_get_os_version() == WINDOWS_XP) {
        if (hid > 0) {
            count = (total <= hid) ? (total + hid) : total;
            dtrace("HID above zero - keyboard count is %u (%u total)",count,total);
        } else {
            count = (total == 1 ? 2 : total);
            dtrace("HID equals zero - keyboard count is %u (%u total)",count,total);
        }
    } else {
        count = total;
        itrace("not XP - keyboard count is %u (%u total)",count,total);
    }

    trace(I"==========");
    winx_log_keyboard_info();
    trace(I"==========");

    return count;
}

/** @} */
