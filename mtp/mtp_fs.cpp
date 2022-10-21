/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include "log.hpp"
#include <filesystem>
#include <Utils.hpp>

extern "C" {
#include "mtp_responder.h"
#include "mtp_db.h"
#include "mtp_fs.h"
}

namespace
{
    mtp_storage_properties_t disk_properties = {
        .type        = MTP_STORAGE_FIXED_RAM,
        .fs_type     = MTP_STORAGE_FILESYSTEM_FLAT,
        .access_caps = MTP_STORAGE_READ_WRITE,
        .capacity    = 0,
        .description = "Storage",
        .volume_id   = "1234567890abcdef",
    };

    constexpr auto iobuf_size = 64U * 1024U; // 64kB

    bool is_dot(const char *name)
    {
        const auto name_length = strlen(name);
        const bool ret =
            ((name_length == 1) && (name[0] == '.')) || ((name_length == 2) && (memcmp(name, "..", 2) == 0));
        log_debug("is_dot: '%s': %s", name, ret ? "true" : "false");
        return ret;
    }

    uint32_t count_files(DIR *find_data)
    {
        uint32_t count = 0;
        rewinddir(find_data);
        struct dirent *de;
        while ((de = readdir(find_data)) != nullptr) {
            if (is_dot(de->d_name)) {
                continue;
            }
            count++;
        }
        return count;
    }

    const mtp_storage_properties_t *get_disk_properties(void *arg)
    {
        const auto fs = static_cast<struct mtp_fs *>(arg);
        struct statvfs stvfs {};
        const auto ret = statvfs(fs->root, &stvfs);

        if (ret == 0) {
            [[maybe_unused]] const auto freeSpace = stvfs.f_bavail * stvfs.f_bsize;
            const auto capacity                   = stvfs.f_blocks * stvfs.f_bsize;
            disk_properties.capacity              = capacity;

            log_debug("Capacity: %u MiB, free: %u MiB",
                      static_cast<unsigned>(capacity / 1024 / 1024),
                      static_cast<unsigned>(freeSpace / 1024 / 1024));
        }
        else {
            log_debug("Failed to vfsstat %s, error %d", fs->root, errno);
        }

        return &disk_properties;
    }

    uint64_t get_free_space(void *arg)
    {
        const auto fs = static_cast<struct mtp_fs *>(arg);
        uint64_t freeSpace = 0;
        struct statvfs stvfs {};
        const auto ret = statvfs(fs->root, &stvfs);

        if (ret == 0) {
            freeSpace = stvfs.f_bavail * stvfs.f_bsize;
            log_debug("Free space: %u MiB", static_cast<unsigned>(freeSpace / 1024 / 1024));
        }
        else {
            log_debug("Failed to vfsstat %s, error %d", fs->root, errno);
        }

        return freeSpace;
    }

    uint32_t fs_find_first(void *arg, uint32_t root, uint32_t *count)
    {
        const auto fs = static_cast<struct mtp_fs *>(arg);
        uint32_t handle;
        // TODO: list dir is not a good choose, as it allocates
        // memory for all files from directory
        // auto list = vfs.listdir(root);
        // it would be better to have iterator mapped to
        // filesystem.

        if (root != 0 && root != 0xFFFFFFFF) {
            return 0;
        }

        fs->find_data = opendir(fs->root);
        if (fs->find_data == nullptr) {
            log_error("Opendir failed");
            return 0;
        }
        *count = count_files(fs->find_data);
        log_debug("Found: %u files", static_cast<unsigned>(*count));
        if (*count == 0) {
            return 0; // empty directory
        }

        rewinddir(fs->find_data);
        struct dirent *de;
        while (((de = readdir(fs->find_data)) != nullptr) && is_dot(de->d_name)) {
            log_debug("Skip: '%s'", de->d_name);
        }
        if (de == nullptr) {
            log_error("No files found");
            *count = 0;
            return 0;
        }
        handle = mtp_db_add(fs->db, de->d_name);
        return handle;
    }

    uint32_t fs_find_next(void *arg)
    {
        uint32_t handle;
        const auto fs = static_cast<struct mtp_fs *>(arg);
        struct dirent *de;
        while ((de = readdir(fs->find_data)) != nullptr) {
            if (is_dot(de->d_name)) {
                continue;
            }
            handle = mtp_db_add(fs->db, de->d_name);
            return handle;
        }
        log_debug("Done, no more files");
        return 0;
    }

    uint16_t ext_to_format_code(const char *name)
    {
        const auto extension = std::filesystem::path(name).extension();
        const auto extensionLowercase = utils::stringToLowercase(extension);

        if (extensionLowercase == ".jpg" || extensionLowercase == ".jpeg") {
            return MTP_FORMAT_EXIF_JPEG;
        }
        if (extensionLowercase == ".txt") {
            return MTP_FORMAT_TEXT;
        }
        if (extensionLowercase == ".wav") {
            return MTP_FORMAT_WAV;
        }
        if (extensionLowercase == ".mp3") {
            return MTP_FORMAT_MP3;
        }
        if (extensionLowercase == ".flac") {
            return MTP_FORMAT_FLAC;
        }
        return MTP_FORMAT_UNDEFINED;
    }

    int fs_stat(void *arg, uint32_t handle, mtp_object_info_t *info)
    {
        struct stat statbuf {};
        const auto fs = static_cast<struct mtp_fs *>(arg);
        const auto filename = mtp_db_get(fs->db, handle);

        if (filename == nullptr) {
            return -1;
        }

        log_debug("[%u]: get info for %s", static_cast<unsigned int>(handle), filename);
        const auto absolutePath = std::string(fs->root) + std::string("/") + std::string(filename);

        if (stat(absolutePath.c_str(), &statbuf) == 0) {
            memset(info, 0, sizeof(mtp_object_info_t));
            info->storage_id                          = 0x00010001;
            info->created                             = statbuf.st_ctim.tv_sec;
            info->modified                            = statbuf.st_mtim.tv_sec;
            info->format_code                         = ext_to_format_code(filename);
            info->size                                = statbuf.st_size;
            *reinterpret_cast<uint32_t *>(info->uuid) = handle;

            strncpy(info->filename, filename, sizeof(info->filename));
            return 0;
        }

        log_error("[%u]: stat error %s: %d", static_cast<unsigned int>(handle), filename, errno);
        return -1;
    }

    int fs_rename(void *arg, uint32_t handle, const char *new_name)
    {
        const auto fs = static_cast<struct mtp_fs *>(arg);
        const auto filename    = mtp_db_get(fs->db, handle);
        int status;

        if (filename == nullptr) {
            log_error("[%u]: filename is nullptr", static_cast<unsigned>(handle));
            return -1;
        }

        status = rename(filename, new_name);
        if (status == 0) {
            mtp_db_update(fs->db, handle, new_name);
            log_debug("[%u]: rename: %s -> %s", static_cast<unsigned>(handle), filename, new_name);
        }
        else {
            log_error("[%u]: rename: %s -> %s FAILED", static_cast<unsigned>(handle), filename, new_name);
        }
        return status;
    }

    int fs_create(void *arg, const mtp_object_info_t *info, uint32_t *handle)
    {
        const auto fs = static_cast<struct mtp_fs *>(arg);
        uint32_t new_handle;
        const auto absolutePath = std::string(fs->root) + std::string("/") + std::string(info->filename);

        if (const auto freeSpace = get_free_space(arg); freeSpace < info->size) {
            log_error("There is not enough space for file %s (%llu < %llu)", info->filename, freeSpace, info->size);
            return -1;
        }
        new_handle = mtp_db_add(fs->db, info->filename);
        if (new_handle == 0) {
            log_error("Map is full. Can't create: %s", info->filename);
            return -1;
        }

        fs->file = std::fopen(absolutePath.c_str(), "w");
        if (fs->file == nullptr) {
            log_error("Failed to open %s", info->filename);
            mtp_db_del(fs->db, new_handle); // Remove the file previously added to DB if creation failed
            return -1;
        }
        std::fclose(fs->file);
        fs->file = nullptr;
        *handle  = new_handle;
        log_debug("[%u]: created: %s", static_cast<unsigned>(*handle), info->filename);
        return 0;
    }

    int fs_remove(void *arg, uint32_t handle)
    {
        const auto fs    = static_cast<struct mtp_fs *>(arg);
        const auto filename = mtp_db_get(fs->db, handle);
        if (filename == nullptr) {
            log_error("[%u]: filename is nullptr", static_cast<unsigned>(handle));
            return -1;
        }

        const auto absolutePath = std::string(fs->root) + std::string("/") + std::string(filename);
        unlink(absolutePath.c_str());

        log_debug("[%u]: removed: %s", static_cast<unsigned>(handle), filename);
        mtp_db_del(fs->db, handle);
        return 0;
    }

    int fs_open(void *arg, uint32_t handle, const char *mode)
    {
        const auto fs    = static_cast<struct mtp_fs *>(arg);
        const auto filename = mtp_db_get(fs->db, handle);
        if (filename == nullptr) {
            log_error("[%u]: filename is nullptr", static_cast<unsigned>(handle));
            return -1;
        }

        const auto absolutePath = std::string(fs->root) + std::string("/") + std::string(filename);
        fs->file                = std::fopen(absolutePath.c_str(), mode);
        if (fs->file == nullptr) {
            log_error("[%u]: fail to open: %s [%s]. Flush and wait", static_cast<unsigned>(handle), filename, mode);
            fs->iobuf = nullptr;
        }
        else {
            fs->iobuf = new (std::nothrow) char[iobuf_size];
            if (fs->iobuf != nullptr) {
                if (setvbuf(fs->file, fs->iobuf, _IOFBF, iobuf_size) != 0) {
                    log_error("[%u]: unable to setvbuf, errno %d", static_cast<uintptr_t>(handle), errno);
                }
            }
            else {
                log_error("[%u]: unable to allocate iobuffer", static_cast<uintptr_t>(handle));
            }
        }
        log_debug("[%u]: opened: %s [%s]", static_cast<unsigned>(handle), filename, mode);
        return static_cast<int>(fs->file == nullptr);
    }

    int fs_read(void *arg, void *buffer, size_t count)
    {
        const auto fs = static_cast<struct mtp_fs *>(arg);
        size_t read;

        if (fs->file == nullptr) {
            return -1;
        }

        while ((read = std::fread(buffer, 1, count, fs->file)) <= 0) {
            log_error("[]: fail to read");
        }
        return read;
    }

    int fs_write(void *arg, const void *buffer, size_t count)
    {
        const auto fs = static_cast<struct mtp_fs *>(arg);
        if (fs->file == nullptr) {
            return -1;
        }

        while (std::fwrite(buffer, 1, count, fs->file) != count) {
            log_error("[]: fail to write");
        }
        return 0;
    }

    void fs_close(void *arg)
    {
        const auto fs = static_cast<struct mtp_fs *>(arg);

        if (fs->file != nullptr) {
            std::fclose(fs->file);
            log_debug("[]: closed");
            fs->file = nullptr;
            delete[] fs->iobuf;
            fs->iobuf = nullptr;
        }
    }
} // namespace

extern "C" const struct mtp_storage_api simple_fs_api =
{
    .get_properties = get_disk_properties,
    .find_first = fs_find_first,
    .find_next = fs_find_next,
    .get_free_space = get_free_space,
    .stat = fs_stat,
    .rename = fs_rename,
    .create = fs_create,
    .remove = fs_remove,
    .open = fs_open,
    .read = fs_read,
    .write = fs_write,
    .close = fs_close
};

extern "C" struct mtp_fs* mtp_fs_alloc(void *mtpRootPath)
{
    struct mtp_fs *fs = (struct mtp_fs *) calloc(1, sizeof(struct mtp_fs));
    if (fs != NULL) {
        fs->db = mtp_db_alloc();
        if (fs->db == NULL) {
            free(fs);
            return NULL;
        }

        fs->root = (const char *)mtpRootPath;
        log_debug("[]: initializing MTP root at %s", fs->root);

        fs->find_data = opendir(fs->root);
        if (fs->find_data == NULL) {
            mtp_fs_free(fs);
            return NULL;
        }
    }
    return fs;
}

extern "C" void mtp_fs_free(struct mtp_fs *fs)
{
    mtp_db_free(fs->db);
    free(fs);
}
