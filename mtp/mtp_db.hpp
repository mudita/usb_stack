// Copyright (c) 2017-2023, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#pragma once

#include <set>
#include <filesystem>
#include <optional>

namespace mtp
{
    using Handle = std::uint32_t;
    class DatabaseEntry
    {
      public:
        DatabaseEntry(Handle handle, std::filesystem::path filename);
        explicit DatabaseEntry(Handle handle);

        Handle get_handle() const;
        std::filesystem::path get_filename() const;

        bool operator<(const DatabaseEntry &rhs) const;
        bool operator==(const DatabaseEntry &rhs) const;

      private:
        Handle handle;
        std::filesystem::path filename;
    };

    /// FileDatabase is a container used to store MTP object handles and corresponding data
    class FileDatabase
    {
      public:
        /// Try to fetch entry's filename by handle.
        std::optional<std::filesystem::path> get_filename(Handle handle) const;

        /// Try to remove entry by handle. Returns false in case of failure.
        bool remove(Handle handle);

        /// Try to insert entry with the specific filename. Returns assigned unique index in case of success.
        std::optional<Handle> insert(const char *filename);

        /// Try to update specific entry by unique handle. Returns false in case of failure
        bool update(Handle handle, const char *filename);

      private:
        std::set<DatabaseEntry> entries;
    };

} // namespace mtp
