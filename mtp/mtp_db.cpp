// Copyright (c) 2017-2023, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include "mtp_db.hpp"

namespace mtp
{
    std::optional<std::filesystem::path> FileDatabase::get_filename(Handle handle) const
    {
        if (const auto result = entries.find(DatabaseEntry{handle}); result != entries.end()) {
            return result->get_filename();
        }
        return std::nullopt;
    }
    bool FileDatabase::remove(const Handle handle)
    {
        return entries.erase(DatabaseEntry{handle}) != 0;
    }
    std::optional<Handle> FileDatabase::insert(const char *filename)
    {
        static Handle handle_idx = 1;
        if (const auto result = entries.insert(DatabaseEntry{handle_idx++, filename}); result.second) {
            return result.first->get_handle();
        }
        return std::nullopt;
    }
    bool FileDatabase::update(const Handle handle, const char *filename)
    {
        if (const auto result = entries.find(DatabaseEntry{handle}); result != entries.end()) {
            remove(result->get_handle());
            entries.insert(DatabaseEntry{result->get_handle(), filename});
            return true;
        }
        return false;
    }
    DatabaseEntry::DatabaseEntry(const Handle handle, std::filesystem::path filename)
        : handle{handle}, filename{std::move(filename)}
    {}
    DatabaseEntry::DatabaseEntry(const Handle handle) : handle{handle}
    {}
    Handle DatabaseEntry::get_handle() const
    {
        return handle;
    }
    std::filesystem::path DatabaseEntry::get_filename() const
    {
        return filename;
    }
    bool DatabaseEntry::operator<(const DatabaseEntry &rhs) const
    {
        return (handle < rhs.handle);
    }
    bool DatabaseEntry::operator==(const DatabaseEntry &rhs) const
    {
        return (handle == rhs.handle);
    }
} // namespace mtp
