#pragma once

#include <boost/uuid/uuid.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

namespace entities {

enum class GameStatus : int
{
    kUnspecified = 0,
    kPlan = 1,
    kPlaying = 2,
    kCompleted = 3,
    kDropped = 4,
    kWaiting = 5
};

struct LibraryPostgres
{
    boost::uuids::uuid user_id;
    boost::uuids::uuid game_id;
    GameStatus game_status;
    
    userver::storages::postgres::TimePointWithoutTz created_at;
    userver::storages::postgres::TimePointWithoutTz updated_at;
};

} // namespace entities