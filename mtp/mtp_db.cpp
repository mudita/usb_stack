// Copyright (c) 2017-2023, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include <algorithm>
#include "mtp_db.hpp"

namespace mtp
{
    std::optional<std::filesystem::path> FileDatabase::get_filename(Handle handle) const
    {
        const auto result = handle_to_filename.find(handle);
        if (result != handle_to_filename.end()) {
            return result->second->first;
        }
        return std::nullopt;
    }
    bool FileDatabase::remove(const Handle handle)
    {
        const auto result = handle_to_filename.find(handle);
        if (result != handle_to_filename.end()) {
            filename_to_handle.erase(result->second);
            handle_to_filename.erase(result);
            return true;
        }
        return false;
    }
    Handle FileDatabase::insert(const char *filename)
    {
        static Handle handle_idx = 1;

        const auto result = filename_to_handle.insert({filename, handle_idx++});
        if (result.second) {
            handle_to_filename.insert({result.first->second, result.first});
            return result.first->second;
        }
        return result.first->second;
    }
    bool FileDatabase::update(const Handle handle, const char *filename)
    {
        const auto result = handle_to_filename.find(handle);
        if (result != handle_to_filename.end()) {
            filename_to_handle.erase(result->second);
            auto entry = filename_to_handle.insert({filename, handle});
            if (!entry.second) {
                entry.first->second = result->first;
            }
            result->second = entry.first;
            return true;
        }
        return false;
    }
} // namespace mtp
