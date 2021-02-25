/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "usb.h"
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "mtp_file_system_adapter.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define DATE_YEAR_SHIFT  (9U)
#define DATE_YEAR_MASK   (0xFE00U)
#define DATE_MONTH_SHIFT (5U)
#define DATE_MONTH_MASK  (0x01E0U)
#define DATE_DAY_SHIFT   (0U)
#define DATE_DAY_MASK    (0x001FU)

#define TIME_HOUR_SHIFT   (11U)
#define TIME_HOUR_MASK    (0xF800U)
#define TIME_MINUTE_SHIFT (5U)
#define TIME_MINUTE_MASK  (0x07E0U)
#define TIME_SECOND_SHIFT (0U)
#define TIME_SECOND_MASK  (0x001FU)

typedef struct
{
    FILE *file;
    uint8_t flags;
} usb_device_mtp_file_instance_t;

typedef struct
{
    DIR *dir;
    uint8_t flags;
} usb_device_mtp_dir_instance_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
static usb_device_mtp_file_instance_t s_FileInstance[USB_DEVICE_MTP_FILE_INSTANCE];
// static usb_device_mtp_dir_instance_t s_DirInstance[USB_DEVICE_MTP_DIR_INSTANCE];
// static FATFS s_Fs;

/*******************************************************************************
 * Code
 ******************************************************************************/

static usb_status_t USB_DeviceMtpAllocateFileHandle(FILE **file)
{
    uint32_t i;

    for (i = 0; i < USB_DEVICE_MTP_FILE_INSTANCE; i++)
    {
        if (s_FileInstance[i].flags == 0U)
        {
            s_FileInstance[i].flags = 1U;
            *file                   = s_FileInstance[i].file;
            return kStatus_USB_Success;
        }
    }

    return kStatus_USB_Busy;
}

static usb_status_t USB_DeviceMtpFreeFileHandle(FILE *file)
{
    uint32_t i;

    for (i = 0; i < USB_DEVICE_MTP_FILE_INSTANCE; i++)
    {
        if ((s_FileInstance[i].flags != 0U) && (s_FileInstance[i].file == file))
        {
            s_FileInstance[i].flags = 0U;
            return kStatus_USB_Success;
        }
    }

    return kStatus_USB_Busy;
}

// static usb_status_t USB_DeviceMtpAllocateDirHandle(DIR **dir)
// {
//     uint32_t i;

//     for (i = 0; i < USB_DEVICE_MTP_DIR_INSTANCE; i++)
//     {
//         if (s_DirInstance[i].flags == 0U)
//         {
//             s_DirInstance[i].flags = 1U;
//             *dir                   = s_DirInstance[i].dir;
//             return kStatus_USB_Success;
//         }
//     }

//     return kStatus_USB_Busy;
// }

// static usb_status_t USB_DeviceMtpFreeDirHandle(DIR *dir)
// {
//     uint32_t i;

//     for (i = 0; i < USB_DEVICE_MTP_DIR_INSTANCE; i++)
//     {
//         if ((s_DirInstance[i].flags != 0U) && (s_DirInstance[i].dir == dir))
//         {
//             s_DirInstance[i].flags = 0U;
//             return kStatus_USB_Success;
//         }
//     }

//     return kStatus_USB_Busy;
// }

usb_status_t USB_DeviceMtpOpen(usb_device_mtp_file_handle_t *file, const uint16_t *fileName, uint32_t flags)
{
    FILE *fil;
    char mode[4] = {'r', 0, };
    uint32_t modeIdx = 0;

    if (USB_DeviceMtpAllocateFileHandle(&fil) != kStatus_USB_Success)
    {
        return kStatus_USB_Busy;
    }

    *file = (usb_device_mtp_file_handle_t)fil;

    // if ((flags & USB_DEVICE_MTP_READ) != 0U)
    // {
    //     mode |= FA_READ;
    // }
    if ((flags & USB_DEVICE_MTP_WRITE) != 0U)
    {
        mode[modeIdx++] = 'w';
    }
    if ((flags & USB_DEVICE_MTP_CREATE_NEW) != 0U)
    {
        mode[modeIdx++] = 'x';
    }
    if ((flags & USB_DEVICE_MTP_CREATE_ALWAYS) != 0U)
    {
        mode[modeIdx++] = '+';
    }

    fil = fopen((const char*)fileName, mode);

    if (!fil)
    {
        USB_DeviceMtpFreeFileHandle(fil);
        return kStatus_USB_Error;
    }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpClose(usb_device_mtp_file_handle_t file)
{
    FILE *fil;

    if (file == NULL)
    {
        return kStatus_USB_InvalidHandle;
    }

    fil = (FILE *)file;

    if (!fclose(fil))
    {
        USB_DeviceMtpFreeFileHandle(fil);
        return kStatus_USB_Error;
    }

    USB_DeviceMtpFreeFileHandle(fil);
    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpLseek(usb_device_mtp_file_handle_t file, uint32_t offset)
{
    FILE *fil;

    if (file == NULL)
    {
        return kStatus_USB_InvalidHandle;
    }

    fil = (FILE *)file;

    if (fseek(fil, offset, SEEK_SET))
    {
        return kStatus_USB_Error;
    }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpRead(usb_device_mtp_file_handle_t file, void *buffer, uint32_t size, uint32_t *actualsize)
{
    FILE *fil;

    if (file == NULL)
    {
        return kStatus_USB_InvalidHandle;
    }

    fil = (FILE *)file;

    *actualsize = fread(buffer, 1, size, fil);

    if (*actualsize < size)
    {
        return kStatus_USB_Error;
    }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpWrite(usb_device_mtp_file_handle_t file, void *buffer, uint32_t size, uint32_t *actualsize)
{
    FILE *fil;

    if (file == NULL)
    {
        return kStatus_USB_InvalidHandle;
    }

    fil = (FILE *)file;

    *actualsize = fwrite(buffer, 1, size, fil);

    if (*actualsize < size)
    {
        return kStatus_USB_Error;
    }

    return kStatus_USB_Success;
}

#define IS_READONLY(mode) ((mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
#define IS_DIRECTORY(mode) (mode & S_IFDIR)

usb_status_t USB_DeviceMtpFstat(const uint16_t *fileName, usb_device_mtp_file_info_t *fileInfo)
{
    struct stat fno;
    uint32_t count;
    uint8_t *src;
    uint8_t *dest;

    if (fileInfo == NULL)
    {
        return kStatus_USB_InvalidParameter;
    }

    FILE* fd = fopen((const char*)fileName, "r");

    if (fstat(fileno(fd), &fno) != 0)
    {
        return kStatus_USB_Error; /* return on error */
    }

    fclose(fd);

    // if ((fno.fattrib & AM_SYS) != 0U)
    // {
    //     return kStatus_USB_Error; /* do not expose system directories or files */
    // }

    fileInfo->size = fno.st_size;

    struct tm *time = gmtime(&fno.st_mtime);

    fileInfo->dateUnion.dateBitField.year   = time->tm_year; // ((fno.st_mtime & DATE_YEAR_MASK) >> DATE_YEAR_SHIFT) + 1980U; /* Year origin from 1980 */
    fileInfo->dateUnion.dateBitField.month  = time->tm_mon;  // (fno.st_ & DATE_MONTH_MASK) >> DATE_MONTH_SHIFT;
    fileInfo->dateUnion.dateBitField.day    = time->tm_mday; // (fno.st_mtime & DATE_DAY_MASK) >> DATE_DAY_SHIFT;
    fileInfo->timeUnion.timeBitField.hour   = time->tm_hour; // (fno.ftime & TIME_HOUR_MASK) >> TIME_HOUR_SHIFT;
    fileInfo->timeUnion.timeBitField.minute = time->tm_min;  // (fno.ftime & TIME_MINUTE_MASK) >> TIME_MINUTE_SHIFT;
    fileInfo->timeUnion.timeBitField.second = time->tm_sec;  // ((fno.ftime & TIME_SECOND_MASK) >> TIME_SECOND_SHIFT) << 1U; /* Second / 2 (0...29) */

    fileInfo->attrib = 0U;

    if (IS_DIRECTORY(fno.st_mode))
    {
        fileInfo->attrib |= USB_DEVICE_MTP_DIR;
    }
    if (IS_READONLY(fno.st_mode))
    {
        fileInfo->attrib |= USB_DEVICE_MTP_READ_ONLY;
    }

    /* copy file name, unicode encoding. */
    src   = (uint8_t *)fileName;
    dest  = (uint8_t *)&fileInfo->name[0];
    count = 0;
    while ((src[count] != 0U) || (src[count + 1U] != 0U) || ((count % 2U) != 0U))
    {
        dest[count] = src[count];
        count++;
    }
    dest[count]      = 0U;
    dest[count + 1U] = 0U; /* terminate with 0x00, 0x00 */

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpUtime(const uint16_t *fileName, usb_device_mtp_file_time_stamp_t *timeStamp)
{
    // struct stat fno;

    // fno.fdate = (WORD)((((WORD)timeStamp->year - 1980U) << 9U) | ((WORD)timeStamp->month << 5U) | (WORD)timeStamp->day);
    // fno.ftime =
    //     (WORD)(((WORD)timeStamp->hour << 11U) | ((WORD)timeStamp->minute * 32U) | ((WORD)timeStamp->second / 2U));

    // if (f_utime((const TCHAR *)fileName, &fno) != FR_OK)
    // {
    //     return kStatus_USB_Error;
    // }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpOpenDir(usb_device_mtp_dir_handle_t *dir, const uint16_t *dirName)
{
    // DIR *dir1;

    // if (USB_DeviceMtpAllocateDirHandle(&dir1) != kStatus_USB_Success)
    // {
    //     return kStatus_USB_Busy;
    // }

    // *dir = (usb_device_mtp_dir_handle_t)dir1;

    // if (f_opendir(dir1, (const TCHAR *)dirName) != FR_OK)
    // {
    //     USB_DeviceMtpFreeDirHandle(dir1);
    //     return kStatus_USB_Error;
    // }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpCloseDir(usb_device_mtp_dir_handle_t dir)
{
    // DIR *dir1;

    // if (dir == NULL)
    // {
    //     return kStatus_USB_InvalidHandle;
    // }

    // dir1 = (DIR *)dir;

    // if (f_closedir(dir1) != FR_OK)
    // {
    //     USB_DeviceMtpFreeDirHandle(dir1);
    //     return kStatus_USB_Error;
    // }

    // USB_DeviceMtpFreeDirHandle(dir1);

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpReadDir(usb_device_mtp_dir_handle_t dir, usb_device_mtp_file_info_t *fileInfo)
{
    // DIR *dir1;
    // FILINFO fno;
    // uint32_t count;
    // uint8_t *src;
    // uint8_t *dest;

    // if (dir == NULL)
    // {
    //     return kStatus_USB_InvalidHandle;
    // }

    // if (fileInfo == NULL)
    // {
    //     return kStatus_USB_InvalidParameter;
    // }

    // dir1 = (DIR *)dir;

    // for (;;)
    // {
    //     if (f_readdir(dir1, &fno) != FR_OK)
    //     {
    //         return kStatus_USB_Error; /* return on error */
    //     }

    //     if (fno.fname[0] == 0U)
    //     {
    //         return kStatus_USB_InvalidRequest; /* return on end of dir */
    //     }

    //     if ((fno.fattrib & AM_SYS) == 0U)
    //     {
    //         break; /* do not expose system directories or files */
    //     }
    // }

    // fileInfo->size = fno.fsize;
    // fileInfo->dateUnion.dateBitField.year =
    //     ((fno.fdate & DATE_YEAR_MASK) >> DATE_YEAR_SHIFT) + 1980U; /* Year origin from 1980 */
    // fileInfo->dateUnion.dateBitField.month  = (fno.fdate & DATE_MONTH_MASK) >> DATE_MONTH_SHIFT;
    // fileInfo->dateUnion.dateBitField.day    = (fno.fdate & DATE_DAY_MASK) >> DATE_DAY_SHIFT;
    // fileInfo->timeUnion.timeBitField.hour   = (fno.ftime & TIME_HOUR_MASK) >> TIME_HOUR_SHIFT;
    // fileInfo->timeUnion.timeBitField.minute = (fno.ftime & TIME_MINUTE_MASK) >> TIME_MINUTE_SHIFT;
    // fileInfo->timeUnion.timeBitField.second = ((fno.ftime & TIME_SECOND_MASK) >> TIME_SECOND_SHIFT)
    //                                           << 1U; /* Second / 2 (0...29) */

    // fileInfo->attrib = 0U;
    // if (fno.fattrib & AM_DIR)
    // {
    //     fileInfo->attrib |= USB_DEVICE_MTP_DIR;
    // }
    // if (fno.fattrib & AM_RDO)
    // {
    //     fileInfo->attrib |= USB_DEVICE_MTP_READ_ONLY;
    // }

    // /* copy file name, unicode encoding. */
    // src   = (uint8_t *)&fno.fname[0];
    // dest  = (uint8_t *)&fileInfo->name[0];
    // count = 0;
    // while ((src[count] != 0U) || (src[count + 1U] != 0U) || ((count % 2U) != 0U))
    // {
    //     dest[count] = src[count];
    //     count++;
    // }
    // dest[count]      = 0U;
    // dest[count + 1U] = 0U; /* terminate with 0x00, 0x00 */

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpMakeDir(const uint16_t *fileName)
{
    // if (f_mkdir((const TCHAR *)fileName) != FR_OK)
    // {
    //     return kStatus_USB_Error;
    // }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpUnlink(const uint16_t *fileName)
{
    // if (f_unlink((const TCHAR *)fileName) != FR_OK)
    // {
    //     return kStatus_USB_Error;
    // }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpRename(const uint16_t *oldName, const uint16_t *newName)
{
    // if (f_rename((const TCHAR *)oldName, (const TCHAR *)newName) != FR_OK)
    // {
    //     return kStatus_USB_Error;
    // }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpGetDiskTotalBytes(const uint16_t *path, uint64_t *totalBytes)
{
    // DWORD freeClust;
    // DWORD totalSector;
    // FATFS *fs;
    // uint32_t sectorSize;

    // if (totalBytes == NULL)
    // {
    //     return kStatus_USB_InvalidParameter;
    // }

    // (void)f_getfree((const TCHAR *)path, &freeClust, &fs);
    // (void)disk_ioctl(path[0] - '0', GET_SECTOR_SIZE, &sectorSize);
    // totalSector = (fs->n_fatent - 2) * fs->csize;
    // *totalBytes = (uint64_t)totalSector * sectorSize;

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpGetDiskFreeBytes(const uint16_t *path, uint64_t *freeBytes)
{
    // DWORD freeClust;
    // DWORD freeSector;
    // FATFS *fs;
    // uint32_t sectorSize;

    // if (freeBytes == NULL)
    // {
    //     return kStatus_USB_InvalidParameter;
    // }

    // (void)f_getfree((const TCHAR *)path, &freeClust, &fs);
    // (void)disk_ioctl(path[0] - '0', GET_SECTOR_SIZE, &sectorSize);
    // freeSector = freeClust * fs->csize;
    // *freeBytes = (uint64_t)freeSector * sectorSize;

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpFSInit(const uint16_t *path)
{
//     BYTE work[FF_MAX_SS];

//     if (FR_OK != f_mount(&s_Fs, (const TCHAR *)path, 1U))
//     {
// #if FF_USE_MKFS
//         if (FR_OK != f_mkfs((const TCHAR *)path, 0, work, sizeof(work)))
//         {
//             return kStatus_USB_Error;
//         }
// #endif /* FF_USE_MKFS */
//     }

    return kStatus_USB_Success;
}
