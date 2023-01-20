/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#ifndef _MTP_FS_H
#define _MTP_FS_H

#include <stdio.h>
#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif
struct mtp_fs {
    void* db;
    const char *root;
    DIR *find_data;
    FILE *file;
    char *iobuf;
};

extern const struct mtp_storage_api simple_fs_api;

struct mtp_fs* mtp_fs_alloc(void *disk);
void mtp_fs_free(struct mtp_fs *fs);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif /* _MTP_FS_H */
