// Copyright (c) 2017-2023, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include <algorithm>
#include "mtp_db.hpp"

namespace mtp
{
    inline const Handle& h2f_handle(std::map<Handle, std::map<std::filesystem::path, Handle>::iterator>::const_iterator h2f_iter)
    {
        return h2f_iter->first;
    }

    inline const std::filesystem::path& h2f_filename(std::map<Handle, std::map<std::filesystem::path, Handle>::iterator>::const_iterator h2f_iter)
    {
        return h2f_iter->second->first;
    }

    inline auto& h2f_other_iter(std::map<Handle, std::map<std::filesystem::path, Handle>::iterator>::iterator h2f_iter)
    {
        return h2f_iter->second;
    }

    inline Handle& f2h_handle(std::map<std::filesystem::path, Handle>::iterator f2h_iter)
    {
        return f2h_iter->second;
    }

    std::optional<std::filesystem::path> FileDatabase::get_filename(Handle handle) const
    {
        const auto h2f_iter = handle_to_filename.find(handle);
        if (h2f_iter != handle_to_filename.end()) {
            return h2f_filename(h2f_iter);
        }
        return std::nullopt;
    }
    bool FileDatabase::remove(const Handle handle)
    {
        const auto h2f_iter = handle_to_filename.find(handle);
        if (h2f_iter != handle_to_filename.end()) {
            filename_to_handle.erase(h2f_other_iter(h2f_iter));
            handle_to_filename.erase(h2f_iter);
            return true;
        }
        return false;
    }
    Handle FileDatabase::insert(const char *filename)
    {
        static Handle handle_idx = 1;

        const auto entry = filename_to_handle.emplace(filename, handle_idx);
        if (entry.second) {
            handle_to_filename.emplace(handle_idx, entry.first);
            ++handle_idx;
        }
        return f2h_handle(entry.first);
    }
    bool FileDatabase::update(const Handle handle, const char *filename)
    {
        const auto h2f_iter = handle_to_filename.find(handle);
        if (h2f_iter != handle_to_filename.end()) {
            filename_to_handle.erase(h2f_other_iter(h2f_iter));
            auto entry = filename_to_handle.emplace(filename, handle);
            if (!entry.second) {
                f2h_handle(entry.first) = h2f_handle(h2f_iter);
            }
            h2f_other_iter(h2f_iter) = entry.first;
            return true;
        }
        return false;
    }
} // namespace mtp
