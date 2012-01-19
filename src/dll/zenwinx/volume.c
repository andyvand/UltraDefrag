/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2012 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file volume.c
 * @brief Disk volumes.
 * @addtogroup Disks
 * @{
 */

#include "zenwinx.h"

/**
 * @internal
 * @brief Opens root directory of the volume.
 * @param[in] volume_letter the volume letter.
 * @return File handle, NULL indicates failure.
 */
static HANDLE OpenRootDirectory(unsigned char volume_letter)
{
    wchar_t rootpath[] = L"\\??\\A:\\";
    HANDLE hRoot;
    NTSTATUS Status;
    UNICODE_STRING uStr;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    rootpath[4] = (wchar_t)winx_toupper(volume_letter);
    RtlInitUnicodeString(&uStr,rootpath);
    InitializeObjectAttributes(&ObjectAttributes,&uStr,
                   FILE_READ_ATTRIBUTES,NULL,NULL); /* ?? */
    Status = NtCreateFile(&hRoot,FILE_GENERIC_READ,
                &ObjectAttributes,&IoStatusBlock,NULL,0,
                FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
                NULL,0);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"OpenRootDirectory: cannot open %ls",rootpath);
        return NULL;
    }
    return hRoot;
}

/**
 * @brief Win32 GetDriveType() native equivalent.
 * @param[in] letter the volume letter
 * @return Drive type, negative value indicates failure.
 */
int winx_get_drive_type(char letter)
{
    wchar_t link_name[] = L"\\??\\A:";
    #define MAX_TARGET_LENGTH 256
    wchar_t link_target[MAX_TARGET_LENGTH];
    PROCESS_DEVICEMAP_INFORMATION pdi;
    FILE_FS_DEVICE_INFORMATION ffdi;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    int drive_type;
    HANDLE hRoot;

    /* An additional checks for DFS were suggested by Stefan Pendl (pendl2megabit@yahoo.de). */
    /* DFS shares have DRIVE_NO_ROOT_DIR type though they are actually remote. */

    letter = winx_toupper(letter); /* possibly required for w2k */
    if(letter < 'A' || letter > 'Z'){
        DebugPrint("winx_get_drive_type: invalid letter %c",letter);
        return (-1);
    }
    
    /* check for the drive existence */
    link_name[4] = (wchar_t)letter;
    if(winx_query_symbolic_link(link_name,link_target,MAX_TARGET_LENGTH) < 0)
        return (-1);
    
    /* check for an assignment made by subst command */
    if(wcsstr(link_target,L"\\??\\") == (wchar_t *)link_target)
        return DRIVE_ASSIGNED_BY_SUBST_COMMAND;

    /* check for classical floppies */
    if(wcsstr(link_target,L"Floppy"))
        return DRIVE_REMOVABLE;
    
    /* try to define exactly which type has a specified drive (w2k+) */
    RtlZeroMemory(&pdi,sizeof(PROCESS_DEVICEMAP_INFORMATION));
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                    ProcessDeviceMap,&pdi,
                    sizeof(PROCESS_DEVICEMAP_INFORMATION),
                    NULL);
    if(NT_SUCCESS(Status)){
        drive_type = (int)pdi.Query.DriveType[letter - 'A'];
        /*
        * Type DRIVE_NO_ROOT_DIR have the following drives:
        * 1. assigned by subst command
        * 2. SCSI external drives
        * 3. RAID volumes
        * 4. DFS shares
        * We need additional checks to know exactly.
        */
        if(drive_type != DRIVE_NO_ROOT_DIR)
            return drive_type;
    } else {
        if(Status != STATUS_INVALID_INFO_CLASS){ /* on NT4 this is always false */
            DebugPrintEx(Status,"winx_get_drive_type: cannot get device map");
            return (-1);
        }
    }
    
    /* try to define exactly again which type has a specified drive (nt4+) */
    /* note that the drive motor can be powered on during this check */
    hRoot = OpenRootDirectory(letter);
    if(hRoot == NULL)
        return (-1);
    RtlZeroMemory(&ffdi,sizeof(FILE_FS_DEVICE_INFORMATION));
    Status = NtQueryVolumeInformationFile(hRoot,&iosb,
                    &ffdi,sizeof(FILE_FS_DEVICE_INFORMATION),
                    FileFsDeviceInformation);
    NtClose(hRoot);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_get_drive_type: cannot get volume type for \'%c\'",letter);
        return (-1);
    }

    /* detect remote/cd/dvd/unknown drives */
    if(ffdi.Characteristics & FILE_REMOTE_DEVICE)
        return DRIVE_REMOTE;
    switch(ffdi.DeviceType){
    case FILE_DEVICE_CD_ROM:
    case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
    case FILE_DEVICE_DVD:
        return DRIVE_CDROM;
    case FILE_DEVICE_NETWORK_FILE_SYSTEM:
    case FILE_DEVICE_NETWORK: /* ? */
    case FILE_DEVICE_NETWORK_BROWSER: /* ? */
    case FILE_DEVICE_DFS_FILE_SYSTEM:
    case FILE_DEVICE_DFS_VOLUME:
    case FILE_DEVICE_DFS:
        return DRIVE_REMOTE;
    case FILE_DEVICE_UNKNOWN:
        return DRIVE_UNKNOWN;
    }

    /* detect removable disks */
    if(ffdi.Characteristics & FILE_REMOVABLE_MEDIA)
        return DRIVE_REMOVABLE;

    /* detect fixed disks */
    switch(ffdi.DeviceType){
    case FILE_DEVICE_DISK:
    case FILE_DEVICE_FILE_SYSTEM: /* ? */
    /*case FILE_DEVICE_VIRTUAL_DISK:*/
    /*case FILE_DEVICE_MASS_STORAGE:*/
    case FILE_DEVICE_DISK_FILE_SYSTEM:
        return DRIVE_FIXED;
    default:
        break;
    }
    
    /* nothing detected => drive type is unknown */
    return DRIVE_UNKNOWN;
}

/**
 * @internal
 * @brief Retrieves drive geometry.
 * @param[in] hRoot handle to the
 * root directory.
 * @param[out] pointer to the structure
 * receiving drive geometry.
 * @return Zero for success, negative
 * value otherwise.
 */
static int get_drive_geometry(HANDLE hRoot,winx_volume_information *v)
{
    FILE_FS_SIZE_INFORMATION ffs;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    
    /* get drive geometry */
    RtlZeroMemory(&ffs,sizeof(FILE_FS_SIZE_INFORMATION));
    Status = NtQueryVolumeInformationFile(hRoot,&IoStatusBlock,&ffs,
                sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_get_volume_information: cannot get geometry of drive %c:",
            v->volume_letter);
        return (-1);
    }
    
    /* fill all geometry related fields of the output structure */
    v->total_bytes = (ULONGLONG)ffs.TotalAllocationUnits.QuadPart * \
        ffs.SectorsPerAllocationUnit * ffs.BytesPerSector;
    v->free_bytes = (ULONGLONG)ffs.AvailableAllocationUnits.QuadPart * \
        ffs.SectorsPerAllocationUnit * ffs.BytesPerSector;
    v->total_clusters = (ULONGLONG)ffs.TotalAllocationUnits.QuadPart;
    v->bytes_per_cluster = ffs.SectorsPerAllocationUnit * ffs.BytesPerSector;
    v->sectors_per_cluster = ffs.SectorsPerAllocationUnit;
    v->bytes_per_sector = ffs.BytesPerSector;
    return 0;
}

/**
 * @internal
 * @brief Retrieves the name of the file system.
 * @param[in] hRoot handle to the root directory.
 * @param[out] pointer to the structure receiving
 * the filesystem name.
 * @return Zero for success, negative value otherwise.
 * @note We could analyze the first sector of the 
 * partition directly, but this method is not so good
 * as it accesses the disk physically.
 */
static int get_filesystem_name(HANDLE hRoot,winx_volume_information *v)
{
    FILE_FS_ATTRIBUTE_INFORMATION *pfa;
    int fs_attr_info_size;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    wchar_t fs_name[MAX_FS_NAME_LENGTH + 1];
    int length;

    fs_attr_info_size = MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION);
    pfa = winx_heap_alloc(fs_attr_info_size);
    if(pfa == NULL){
        DebugPrint("winx_get_volume_information: cannot allocate %u bytes of memory",
            fs_attr_info_size);
        return(-1);
    }
    
    RtlZeroMemory(pfa,fs_attr_info_size);
    Status = NtQueryVolumeInformationFile(hRoot,&IoStatusBlock,pfa,
                fs_attr_info_size,FileFsAttributeInformation);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"winx_get_volume_information: cannot get file system name of drive %c:",
            v->volume_letter);
        winx_heap_free(pfa);
        return (-1);
    }
    
    /*
    * pfa->FileSystemName.Buffer may be not NULL terminated
    * (theoretically), so name extraction is more tricky
    * than it should be.
    */
    length = min(MAX_FS_NAME_LENGTH,pfa->FileSystemNameLength / sizeof(wchar_t));
    wcsncpy(fs_name,pfa->FileSystemName,length);
    fs_name[length] = 0;
    _snprintf(v->fs_name,MAX_FS_NAME_LENGTH,"%ws",fs_name);
    v->fs_name[MAX_FS_NAME_LENGTH] = 0;

    /* cleanup */
    winx_heap_free(pfa);
    return 0;
}

/**
 * @internal
 * @brief Retrieves NTFS data for the filesystem.
 * @param[out] pointer to the structure
 * receiving the information.
 * @return Zero for success, negative value otherwise.
 */
static int get_ntfs_data(winx_volume_information *v)
{
    WINX_FILE *f;
    int result;
    
    /* open the volume */
    f = winx_vopen(v->volume_letter);
    if(f == NULL)
        return (-1);
    
    /* get ntfs data */
    result = winx_ioctl(f,FSCTL_GET_NTFS_VOLUME_DATA,
      "get_ntfs_data: ntfs data request",NULL,0,
      &v->ntfs_data,sizeof(NTFS_DATA),NULL);
    winx_fclose(f);
    return result;
}

/**
 * @internal
 * @brief Retrieves volume label.
 * @param[in] hRoot handle to the
 * root directory.
 * @param[out] pointer to the structure
 * receiving the volume label.
 */
static void get_volume_label(HANDLE hRoot,winx_volume_information *v)
{
    FILE_FS_VOLUME_INFORMATION *ffvi;
    int buffer_size;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    
    /* reset label */
    v->label[0] = 0;
    
    /* allocate memory */
    buffer_size = (sizeof(FILE_FS_VOLUME_INFORMATION) - sizeof(wchar_t)) + (MAX_PATH + 1) * sizeof(wchar_t);
    ffvi = winx_heap_alloc(buffer_size);
    if(ffvi == NULL){
        DebugPrint("get_volume_label: cannot allocate %u bytes of memory",
            buffer_size);
        return;
    }
    
    /* try to get actual label */
    RtlZeroMemory(ffvi,buffer_size);
    Status = NtQueryVolumeInformationFile(hRoot,&IoStatusBlock,ffvi,
                buffer_size,FileFsVolumeInformation);
    if(!NT_SUCCESS(Status)){
        DebugPrintEx(Status,"get_volume_label: cannot get volume label of drive %c:",
            v->volume_letter);
        winx_heap_free(ffvi);
        return;
    }
    wcsncpy(v->label,ffvi->VolumeLabel,MAX_PATH);
    v->label[MAX_PATH] = 0;
    winx_heap_free(ffvi);
}

/**
 * @internal
 * @brief Retrieves the volume dirty flag.
 * @param[out] pointer to the structure
 * receiving the volume dirty flag.
 */
static void get_volume_dirty_flag(winx_volume_information *v)
{
    WINX_FILE *f;
    ULONG dirty_flag;
    int result;
    
    /* open the volume */
    f = winx_vopen(v->volume_letter);
    if(f == NULL) return;
    
    /* get dirty flag */
    result = winx_ioctl(f,FSCTL_IS_VOLUME_DIRTY,
        "get_volume_dirty_flag: dirty flag request",
        NULL,0,&dirty_flag,sizeof(ULONG),NULL);
    winx_fclose(f);
    if(result >= 0 && (dirty_flag & VOLUME_IS_DIRTY)){
        DebugPrint("%c: volume is dirty! Run Check Disk to repair it.",
            v->volume_letter);
        v->is_dirty = 1;
    }
}

/**
 * @brief Retrieves detailed information
 * about a disk volume.
 * @param[in] volume_letter the volume letter.
 * @param[in,out] v pointer to structure
 * receiving the volume information.
 * @return Zero for success, negative
 * value otherwise.
 */
int winx_get_volume_information(char volume_letter,winx_volume_information *v)
{
    HANDLE hRoot;
    
    /* check input data correctness */
    if(v == NULL)
        return (-1);

    /* ensure that it will work on w2k */
    volume_letter = winx_toupper(volume_letter);
    
    /* reset all fields of the structure, except of volume_letter */
    memset(v,0,sizeof(winx_volume_information));
    v->volume_letter = volume_letter;

    if(volume_letter < 'A' || volume_letter > 'Z')
        return (-1);
    
    /* open root directory */
    hRoot = OpenRootDirectory(volume_letter);
    if(hRoot == NULL)
        return (-1);
    
    /* get drive geometry */
    if(get_drive_geometry(hRoot,v) < 0){
        NtClose(hRoot);
        return (-1);
    }
    
    /* get the name of contained file system */
    if(get_filesystem_name(hRoot,v) < 0){
        NtClose(hRoot);
        return (-1);
    }
    
    /* get name of the volume */
    get_volume_label(hRoot,v);

    /* get NTFS data */
    memset(&v->ntfs_data,0,sizeof(NTFS_DATA));
    if(!strcmp(v->fs_name,"NTFS")){
        if(get_ntfs_data(v) < 0){
            DebugPrint("winx_get_volume_information: NTFS data is unavailable for %c:",
                volume_letter);
        }
    }
    
    /* get dirty flag */
    get_volume_dirty_flag(v);
    
    NtClose(hRoot);
    return 0;
}

/**
 * @brief Opens a volume for read access.
 * @param[in] volume_letter the volume letter.
 * @return File descriptor, NULL indicates failure.
 */
WINX_FILE *winx_vopen(char volume_letter)
{
    char path[] = "\\??\\A:";
    char flags[2];
    #define FLAG 'r'

    path[4] = winx_toupper(volume_letter);
#if FLAG != 'r'
#error Volume must be opened for read access!
#endif
    flags[0] = FLAG; flags[1] = 0;
    return winx_fopen(path,flags);
}

/**
 * @brief fflush equivalent for entire volume.
 */
int winx_vflush(char volume_letter)
{
    char path[] = "\\??\\A:";
    WINX_FILE *f;
    int result = -1;
    
    path[4] = winx_toupper(volume_letter);
    f = winx_fopen(path,"r+");
    if(f){
        result = winx_fflush(f);
        winx_fclose(f);
    }

    return result;
}

/**
 * @brief Retrieves list of free regions on the volume.
 * @param[in] volume_letter the volume letter.
 * @param[in] flags combination of WINX_GVR_xxx flags.
 * @param[in] cb address of procedure to be called
 * each time when the free region is found on the volume.
 * If callback procedure returns nonzero value,
 * the scan terminates immediately.
 * @param[in] user_defined_data pointer to data
 * passed to the registered callback.
 * @return List of free regions, NULL indicates that
 * either disk is full (unlikely) or some error occured.
 * @note
 * - It is possible to scan disk partially by
 * requesting the scan termination through the callback
 * procedure.
 * - Callback procedure should complete as quickly
 * as possible to avoid slowdown of the scan.
 */
winx_volume_region *winx_get_free_volume_regions(char volume_letter,
        int flags, volume_region_callback cb, void *user_defined_data)
{
    winx_volume_region *rlist = NULL, *rgn = NULL;
    BITMAP_DESCRIPTOR *bitmap;
    #define LLINVALID   ((ULONGLONG) -1)
    #define BITMAPBYTES 4096
    #define BITMAPSIZE  (BITMAPBYTES + 2 * sizeof(ULONGLONG))
    /* bit shifting array for efficient processing of the bitmap */
    unsigned char bitshift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    WINX_FILE *f;
    ULONGLONG i, start, next, free_rgn_start;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    
    /* ensure that it will work on w2k */
    volume_letter = winx_toupper(volume_letter);
    
    /* allocate memory */
    bitmap = winx_heap_alloc(BITMAPSIZE);
    if(bitmap == NULL){
        DebugPrint("winx_get_free_volume_regions: cannot allocate %u bytes of memory",
            BITMAPSIZE);
        return NULL;
    }
    
    /* open volume */
    f = winx_vopen(volume_letter);
    if(f == NULL){
        winx_heap_free(bitmap);
        return NULL;
    }
    
    /* get volume bitmap */
    next = 0, free_rgn_start = LLINVALID;
    do {
        /* get next portion of the bitmap */
        memset(bitmap,0,BITMAPSIZE);
        status = NtFsControlFile(winx_fileno(f),NULL,NULL,0,&iosb,
            FSCTL_GET_VOLUME_BITMAP,&next,sizeof(ULONGLONG),
            bitmap,BITMAPSIZE);
        if(NT_SUCCESS(status)){
            NtWaitForSingleObject(winx_fileno(f),FALSE,NULL);
            status = iosb.Status;
        }
        if(status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW){
            DebugPrintEx(status,"winx_get_free_volume_regions: cannot get volume bitmap");
            winx_fclose(f);
            winx_heap_free(bitmap);
            if(flags & WINX_GVR_ALLOW_PARTIAL_SCAN){
                return rlist;
            } else {
                winx_list_destroy((list_entry **)(void *)&rlist);
                return NULL;
            }
        }
        
        /* scan through the returned bitmap info */
        start = bitmap->StartLcn;
        for(i = 0; i < min(bitmap->ClustersToEndOfVol, 8 * BITMAPBYTES); i++){
            if(!(bitmap->Map[ i/8 ] & bitshift[ i % 8 ])){
                /* cluster is free */
                if(free_rgn_start == LLINVALID)
                    free_rgn_start = start + i;
            } else {
                /* cluster isn't free */
                if(free_rgn_start != LLINVALID){
                    /* add free region to the list */
                    rgn = (winx_volume_region *)winx_list_insert_item((list_entry **)(void *)&rlist,
                        (list_entry *)rgn,sizeof(winx_volume_region));
                    if(rgn == NULL){
                        DebugPrint("winx_get_free_volume_regions: cannot allocate %u bytes of memory",
                            sizeof(winx_volume_region));
                        /* return if partial results aren't allowed */
                        if(!(flags & WINX_GVR_ALLOW_PARTIAL_SCAN))
                            goto fail;
                    } else {
                        rgn->lcn = free_rgn_start;
                        rgn->length = start + i - free_rgn_start;
                        if(cb != NULL){
                            if(cb(rgn,user_defined_data))
                                goto done;
                        }
                    }
                    free_rgn_start = LLINVALID;
                }
            }
        }
        
        /* go to the next portion of data */
        next = bitmap->StartLcn + i;
    } while(status != STATUS_SUCCESS);

    if(free_rgn_start != LLINVALID){
        /* add free region to the list */
        rgn = (winx_volume_region *)winx_list_insert_item((list_entry **)(void *)&rlist,
            (list_entry *)rgn,sizeof(winx_volume_region));
        if(rgn == NULL){
            DebugPrint("winx_get_free_volume_regions: cannot allocate %u bytes of memory",
                sizeof(winx_volume_region));
            /* return if partial results aren't allowed */
            if(!(flags & WINX_GVR_ALLOW_PARTIAL_SCAN))
                goto fail;
        } else {
            rgn->lcn = free_rgn_start;
            rgn->length = start + i - free_rgn_start;
            if(cb != NULL){
                if(cb(rgn,user_defined_data))
                    goto done;
            }
        }
        free_rgn_start = LLINVALID;
    }

done:    
    /* cleanup */
    winx_fclose(f);
    winx_heap_free(bitmap);
    return rlist;
    
fail:
    winx_fclose(f);
    winx_heap_free(bitmap);
    winx_list_destroy((list_entry **)(void *)&rlist);
    return NULL;
}

/**
 * @brief Adds a range of clusters to the list of regions.
 * @param[in,out] rlist the list of volume regions.
 * @param[in] lcn the logical cluster number of the region to be added.
 * @param[in] length the size of the region to be added, in clusters.
 * @return Pointer to updated list of regions.
 * @note For performance reason this routine doesn't insert
 * regions of zero length.
 */
winx_volume_region *winx_add_volume_region(winx_volume_region *rlist,
        ULONGLONG lcn,ULONGLONG length)
{
    winx_volume_region *r, *rnext, *rprev = NULL;
    
    /* don't insert regions of zero length */
    if(length == 0) return rlist;
    
    for(r = rlist; r; r = r->next){
        if(r->lcn > lcn){
            if(r != rlist) rprev = r->prev;
            break;
        }
        if(r->next == rlist){
            rprev = r;
            break;
        }
    }

    /* hits the new region previous one? */
    if(rprev){
        if(rprev->lcn + rprev->length == lcn){
            rprev->length += length;
            if(rprev->lcn + rprev->length == rprev->next->lcn){
                rprev->length += rprev->next->length;
                winx_list_remove_item((list_entry **)(void *)&rlist,
                    (list_entry *)rprev->next);
            }
            return rlist;
        }
    }
    
    /* hits the new region the next one? */
    if(rlist){
        if(rprev == NULL) rnext = rlist;
        else rnext = rprev->next;
        if(lcn + length == rnext->lcn){
            rnext->lcn = lcn;
            rnext->length += length;
            return rlist;
        }
    }
    
    r = (winx_volume_region *)winx_list_insert_item((list_entry **)(void *)&rlist,
        (list_entry *)rprev,sizeof(winx_volume_region));
    if(r == NULL)
        return rlist;
    
    r->lcn = lcn;
    r->length = length;
    return rlist;
}

/**
 * @brief Subtracts a range of clusters from the list of regions.
 * @param[in,out] rlist the list of volume regions.
 * @param[in] lcn the logical cluster number of the region to be subtracted.
 * @param[in] length the size of the region to be subtracted, in clusters.
 * @return Pointer to updated list of regions.
 */
winx_volume_region *winx_sub_volume_region(winx_volume_region *rlist,
        ULONGLONG lcn,ULONGLONG length)
{
    winx_volume_region *r, *head, *next = NULL;
    ULONGLONG remaining_clusters;
    ULONGLONG new_lcn, new_length;

    remaining_clusters = length;
    for(r = rlist; r && remaining_clusters; r = next){
        head = rlist;
        next = r->next;
        if(r->lcn >= lcn + length) break;
        if(r->lcn + r->length > lcn){
            /* sure, at least a part of region is inside a specified range */
            if(r->lcn >= lcn && (r->lcn + r->length) <= (lcn + length)){
                /*
                * list entry is inside a specified range
                * |--------------------|
                *        |-r-|
                */
                remaining_clusters -= r->length;
                winx_list_remove_item((list_entry **)(void *)&rlist,
                    (list_entry *)r);
                goto next_region;
            }
            if(r->lcn < lcn && (r->lcn + r->length) > lcn && \
              (r->lcn + r->length) <= (lcn + length)){
                /*
                * cut the right side of the list entry
                *     |--------------------|
                * |----r----|
                */
                r->length = lcn - r->lcn;
                goto next_region;
            }
            if(r->lcn >= lcn && r->lcn < (lcn + length)){
                /*
                * cut the left side of the list entry
                * |--------------------|
                *                   |----r----|
                */
                new_lcn = lcn + length;
                new_length = r->lcn + r->length - (lcn + length);
                winx_list_remove_item((list_entry **)(void *)&rlist,
                    (list_entry *)r);
                rlist = winx_add_volume_region(rlist,new_lcn,new_length);
                goto next_region;
            }
            if(r->lcn < lcn && (r->lcn + r->length) > (lcn + length)){
                /*
                * specified range is inside list entry
                *   |----|
                * |-------r--------|
                */
                new_lcn = lcn + length;
                new_length = r->lcn + r->length - (lcn + length);
                r->length = lcn - r->lcn;
                rlist = winx_add_volume_region(rlist,new_lcn,new_length);
                goto next_region;
            }
        }
next_region:
        if(rlist == NULL) break;
        if(next == head) break;
    }
    return rlist;
}

/**
 * @brief Frees memory allocated
 * by winx_get_free_volume_regions.
 */
void winx_release_free_volume_regions(winx_volume_region *rlist)
{
    winx_list_destroy((list_entry **)(void *)&rlist);
}

/** @} */
