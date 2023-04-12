// Copyright (c) 2017-2023, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#pragma once

#include <map>
#include <filesystem>
#include <optional>

namespace mtp
{
    using Handle               = std::uint32_t;
    using PathToHandleMap      = std::map<std::filesystem::path, Handle>;
    using HandleToInteratorMap = std::map<Handle, std::map<std::filesystem::path, Handle>::iterator>;

    /// FileDatabase is a container used to store MTP object handles and corresponding data
    class FileDatabase
    {
      public:
        /// Try to fetch entry's filename by handle.
        std::optional<std::filesystem::path> get_filename(Handle handle) const;

        /// Try to remove entry by handle. Returns false in case of failure.
        bool remove(Handle handle);

        /// Try to insert entry with the specific filename. Returns existing handle if file exist and unique if it
        /// didn't exist.
        Handle insert_or_get(const char *filename);

        /// Try to insert entry with the specific filename. Returns assigned unique index in case of success.
        Handle insert(const char *filename);

        /// Try to update specific entry by unique handle. Returns false in case of failure
        bool update(Handle handle, const char *filename);

      private:
        Handle handle_idx = 1;
        PathToHandleMap filenameToHandle;
        HandleToInteratorMap handleToFilename;
    };

} // namespace mtp
