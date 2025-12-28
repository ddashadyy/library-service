#include <tools/utils.hpp>

#include <userver/utils/datetime.hpp>


::google::protobuf::Timestamp utils::TimePointToProtobuf(
    const userver::storages::postgres::TimePointWithoutTz& time_point)
{
    const auto time_string = userver::utils::datetime::Timestring(time_point);
    const auto system_time = userver::utils::datetime::Stringtime(time_string);

    const auto seconds_since_epoch =
        std::chrono::duration_cast<std::chrono::seconds>(
            system_time.time_since_epoch())
            .count();

    ::google::protobuf::Timestamp timestamp;
    timestamp.set_seconds(seconds_since_epoch);

    return timestamp;
}

std::string_view utils::GameStatusToString(::library::GameStatus status)
{
    using Status = ::library::GameStatus;

    switch (status)
    {
    case Status::GAME_STATUS_PLAN:
        return "plan";
    case Status::GAME_STATUS_PLAYING:
        return "playing";
    case Status::GAME_STATUS_COMPLETED:
        return "completed";
    case Status::GAME_STATUS_DROPPED:
        return "dropped";
    case Status::GAME_STATUS_WAITING:
        return "waiting";
    case Status::GAME_STATUS_UNSPECIFIED:
    default:
        return "unspecified";
    }
}