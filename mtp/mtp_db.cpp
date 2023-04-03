// Copyright (c) 2017-2023, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include <algorithm>
#include "mtp_db.hpp"

namespace mtp
{
    namespace filename_to_handle
    {
        inline Handle &getHandle(PathToHandleMap::iterator iter)
        {
            return iter->second;
        }
    } // namespace filename_to_handle

    namespace handle_to_filename
    {
        inline auto &getIter(HandleToInteratorMap::iterator iter)
        {
            return iter->second;
        }
        inline const Handle &getHandle(HandleToInteratorMap::const_iterator iter)
        {
            return iter->first;
        }
        inline const std::filesystem::path &getFilename(HandleToInteratorMap::const_iterator iter)
        {
            return iter->second->first;
        }

    } // namespace handle_to_filename

    std::optional<std::filesystem::path> FileDatabase::get_filename(Handle handle) const
    {
        const auto handleToFilenameIter = handleToFilename.find(handle);
        if (handleToFilenameIter != handleToFilename.end()) {
            return handle_to_filename::getFilename(handleToFilenameIter);
        }
        return std::nullopt;
    }
    bool FileDatabase::remove(const Handle handle)
    {
        const auto handleToFilenameIter = handleToFilename.find(handle);
        if (handleToFilenameIter == handleToFilename.end()) {
            return false;
        }
        filenameToHandle.erase(handle_to_filename::getIter(handleToFilenameIter));
        handleToFilename.erase(handleToFilenameIter);
        return true;
    }
    Handle FileDatabase::insert(const char *filename)
    {
        static Handle handle_idx = 1;

        const auto entry = filenameToHandle.emplace(filename, handle_idx);
        if (entry.second) {
            handleToFilename.emplace(handle_idx, entry.first);
            ++handle_idx;
        }
        return filename_to_handle::getHandle(entry.first);
    }
    bool FileDatabase::update(const Handle handle, const char *filename)
    {
        const auto handleToFilenameIter = handleToFilename.find(handle);
        if (handleToFilenameIter == handleToFilename.end()) {
            return false;
        }
        filenameToHandle.erase(handle_to_filename::getIter(handleToFilenameIter));
        auto entry = filenameToHandle.emplace(filename, handle);
        if (!entry.second) {
            filename_to_handle::getHandle(entry.first) = handle_to_filename::getHandle(handleToFilenameIter);
        }
        handle_to_filename::getIter(handleToFilenameIter) = entry.first;
        return true;
    }
} // namespace mtp
