#include <gtest/gtest.h>

#include <tools/utils.hpp>

#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/utils/datetime.hpp>

#include <utility>
#include <vector>

namespace {

TEST(TimePointToProtobufTest, HandlesModernDate)
{
    auto time_point =
        userver::utils::datetime::Stringtime("2023-10-05T12:00:00+0000");
    userver::storages::postgres::TimePointWithoutTz pg_time{ time_point };

    auto proto_ts = utils::TimePointToProtobuf(pg_time);

    EXPECT_EQ(proto_ts.seconds(), 1696507200);
    EXPECT_EQ(proto_ts.nanos(), 0);
}

TEST(TimePointToProtobufTest, HandlesEpochStart)
{

    auto time_point = std::chrono::system_clock::from_time_t(0);

    userver::storages::postgres::TimePointWithoutTz pg_time{ time_point };

    auto proto_ts = utils::TimePointToProtobuf(pg_time);

    EXPECT_EQ(proto_ts.seconds(), 0);
    EXPECT_EQ(proto_ts.nanos(), 0);
}

TEST(TimePointToProtobufTest, HandlesPreEpochDate)
{
    auto time_point =
        userver::utils::datetime::Stringtime("1969-12-31T23:59:59+0000");
    userver::storages::postgres::TimePointWithoutTz pg_time{ time_point };

    auto proto_ts = utils::TimePointToProtobuf(pg_time);

    EXPECT_EQ(proto_ts.seconds(), -1);
}

TEST(TimePointToProtobufTest, TruncatesSubSeconds)
{
    auto time_point =
        userver::utils::datetime::Stringtime("2023-10-05T12:00:00.999+0000");
    userver::storages::postgres::TimePointWithoutTz pg_time{ time_point };

    auto proto_ts = utils::TimePointToProtobuf(pg_time);

    EXPECT_EQ(proto_ts.seconds(), 1696507200);
    EXPECT_EQ(proto_ts.nanos(), 0);
}

TEST(GameStatusToStringTest, ConvertsAllKnownStatusesCorrectly)
{
    using Status = ::library::GameStatus;

    std::vector<std::pair<Status, std::string_view>> test_cases = {
        { Status::GAME_STATUS_PLAN, "plan" },
        { Status::GAME_STATUS_PLAYING, "playing" },
        { Status::GAME_STATUS_COMPLETED, "completed" },
        { Status::GAME_STATUS_DROPPED, "dropped" },
        { Status::GAME_STATUS_WAITING, "waiting" },
        { Status::GAME_STATUS_UNSPECIFIED, "unspecified" }
    };

    for (const auto& [status, expected_string] : test_cases)
    {
        EXPECT_EQ(utils::GameStatusToString(status), expected_string)
            << "Failed conversion for status ID: " << status;
    }
}

TEST(GameStatusToStringTest, HandlesUnknownStatusGracefully)
{
    auto unknown_status = static_cast<::library::GameStatus>(12345);

    EXPECT_EQ(utils::GameStatusToString(unknown_status), "unspecified");
}

TEST(GameStatusToStringTest, HandlesZeroValue)
{
    auto status_zero = static_cast<::library::GameStatus>(0);
    auto result = utils::GameStatusToString(status_zero);

    EXPECT_FALSE(result.empty());
}

} // namespace