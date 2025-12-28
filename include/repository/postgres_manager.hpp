#pragma once

#include <repository/repository.hpp>
#include <userver/storages/postgres/cluster.hpp>

namespace pg {

class PostgresManager final : public pg::ILibraryRepository
{
public:
    explicit PostgresManager(
        userver::storages::postgres::ClusterPtr pg_cluster);

    LibraryPostgres CreateLibraryEntry(std::string_view user_id,
                                       std::string_view game_id,
                                       std::string_view) const override;
    LibrariesPostgres GetLibraryEntries(std::string_view user_id,
                                        std::string_view status,
                                        std::int32_t limit,
                                        std::int32_t offset) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

} // namespace pg
