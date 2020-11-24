/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#ifndef _MTP_FS_H
#define _MTP_FS_H

struct mtp_fs;

extern const struct mtp_storage_api simple_fs_api;

struct mtp_fs* mtp_fs_alloc(void *disk);
void mtp_fs_free(struct mtp_fs *fs);

#endif /* _MTP_FS_H */
