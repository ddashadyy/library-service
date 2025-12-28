#include <repository/postgres_manager.hpp>

#include <userver/storages/postgres/io/enum_types.hpp>
#include <userver/storages/postgres/io/io_fwd.hpp>
#include <userver/utils/trivial_map.hpp>

#include <library/library_service.usrv.pb.hpp>

template <>
struct userver::storages::postgres::io::CppToUserPg<::library::GameStatus>
{
    static constexpr DBTypeName postgres_name = "library.game_status";
    static constexpr USERVER_NAMESPACE::utils::TrivialBiMap enumerators =
        [](auto selector) {
            return selector()
                .Case("unspecified",
                      ::library::GameStatus::GAME_STATUS_UNSPECIFIED)
                .Case("plan", ::library::GameStatus::GAME_STATUS_PLAN)
                .Case("playing", ::library::GameStatus::GAME_STATUS_PLAYING)
                .Case("dropped", ::library::GameStatus::GAME_STATUS_DROPPED)
                .Case("completed", ::library::GameStatus::GAME_STATUS_COMPLETED)
                .Case("waiting", ::library::GameStatus::GAME_STATUS_WAITING);
        };
};

template <>
struct userver::storages::postgres::io::CppToUserPg<entities::GameStatus>
{
    static constexpr DBTypeName postgres_name = "library.game_status";
    static constexpr USERVER_NAMESPACE::utils::TrivialBiMap enumerators =
        [](auto selector) {
            return selector()
                .Case("unspecified", entities::GameStatus::kUnspecified)
                .Case("plan", entities::GameStatus::kPlan)
                .Case("playing", entities::GameStatus::kPlaying)
                .Case("dropped", entities::GameStatus::kDropped)
                .Case("completed", entities::GameStatus::kCompleted)
                .Case("waiting", entities::GameStatus::kWaiting);
        };
};

namespace pg {

const userver::storages::postgres::Query kUpsertLibraryEntry{
    "INSERT INTO library.library_entries ("
    "  user_id, game_id, game_status"
    ") "
    "VALUES ("
    "  $1, $2, $3::library.game_status"
    ") "
    "ON CONFLICT (user_id, game_id) DO UPDATE SET "
    "  game_status = EXCLUDED.game_status, "
    "  created_at = NOW(), "
    "  updated_at = NOW() "
    "RETURNING "
    "  user_id, game_id, game_status"
};

const userver::storages::postgres::Query kGetLibraryEntries{
    "SELECT user_id, game_id, game_status, created_at, updated_at "
    "FROM library.library_entries "
    "WHERE user_id = $1 "
    "ORDER BY updated_at DESC "
    "LIMIT $2 OFFSET $3"
};

const userver::storages::postgres::Query kGetLibraryStats{
    "SELECT COUNT(*) "
    "FROM playhub.library "
    "WHERE user_id = $1"
};

PostgresManager::PostgresManager(
    std::shared_ptr<userver::storages::postgres::Cluster> cluster)
    : pg_cluster_(std::move(cluster))
{}

PostgresManager::LibraryPostgres
PostgresManager::CreateLibraryEntry(std::string_view user_id,
                                    std::string_view game_id,
                                    std::string_view game_status) const
{
    try
    {
        const auto kResult = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            kUpsertLibraryEntry, user_id, game_id, game_status);

        return kResult.AsSingleRow<LibraryPostgres>(
            userver::storages::postgres::kRowTag);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR() << e.what() << '\n';
    }

    return {};
}

PostgresManager::LibrariesPostgres
PostgresManager::GetLibraryEntries(std::string_view user_id, std::int32_t limit,
                                   std::int32_t offset) const
{
    try
    {
        const auto kResult = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            kGetLibraryEntries, user_id, limit, offset);

        return kResult.AsContainer<LibrariesPostgres>(
            userver::storages::postgres::kRowTag);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR() << e.what() << '\n';
    }

    return {};
}

std::int32_t PostgresManager::GetLibraryStats(std::string_view user_id) const
{
    try
    {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            kGetLibraryStats, user_id);

        return result.AsSingleRow<std::int64_t>();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR() << "Error getting library stats: " << e.what();
    }

    return 0;
}

} // namespace pg