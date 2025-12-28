#pragma once

#include <string_view>
#include <vector>

#include <structs/library_postgres.hpp>

namespace pg {

class ILibraryRepository
{
public:
    using LibraryPostgres = entities::LibraryPostgres;
    using LibrariesPostgres = std::vector<entities::LibraryPostgres>;

    virtual ~ILibraryRepository() = default;

    virtual LibraryPostgres
    CreateLibraryEntry(std::string_view user_id, std::string_view game_id,
                       std::string_view game_status) const = 0;
    virtual LibrariesPostgres GetLibraryEntries(std::string_view user_id,
                                                std::int32_t limit,
                                                std::int32_t offset) const = 0;
    virtual std::int32_t GetLibraryStats(std::string_view user_id) const = 0;
};

} // namespace pg