#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <library/library_client.usrv.pb.hpp>
#include <library/library_service.usrv.pb.hpp>

#include <handlers/library_grpc.hpp>
#include <repository/postgres_manager.hpp>
#include <structs/library_postgres.hpp>
#include <tools/utils.hpp>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace testing;

namespace library_service::test {

class MockLibraryRepository : public pg::ILibraryRepository
{
public:
    MOCK_METHOD(entities::LibraryPostgres, CreateLibraryEntry,
                (std::string_view user_id, std::string_view game_id,
                 std::string_view status),
                (const, override));

    MOCK_METHOD(std::vector<entities::LibraryPostgres>, GetLibraryEntries,
                (std::string_view user_id, std::int32_t limit,
                 std::int32_t offset),
                (const, override));

    MOCK_METHOD(std::int32_t, GetLibraryStats, (std::string_view user_id),
                (const, override));
};

entities::LibraryPostgres CreateFakeLibraryEntry(std::string_view user_id_str)
{
    entities::LibraryPostgres entry;

    if (!user_id_str.empty())
    {
        try
        {
            entry.user_id =
                boost::uuids::string_generator()(std::string(user_id_str));
        }
        catch (...)
        {
            entry.user_id = boost::uuids::random_generator()();
        }
    }
    else
    {
        entry.user_id = boost::uuids::random_generator()();
    }

    entry.game_id = boost::uuids::random_generator()();
    entry.game_status = static_cast<entities::GameStatus>(
        ::library::GameStatus::GAME_STATUS_PLAYING);

    auto now = std::chrono::system_clock::now();
    entry.created_at = userver::storages::postgres::TimePointWithoutTz{ now };
    entry.updated_at = userver::storages::postgres::TimePointWithoutTz{ now };

    return entry;
}

} // namespace library_service::test

class LibraryServiceTest : public userver::ugrpc::tests::ServiceFixtureBase
{
protected:
    std::string prefix_{ "library-prefix" };

    library_service::test::MockLibraryRepository mock_repo_;
    library_service::LibraryService service_;

    LibraryServiceTest() : service_(prefix_, mock_repo_)
    {
        RegisterService(service_);
        StartServer();
    }
};

UTEST_F(LibraryServiceTest, GetLibraryStats_Success)
{
    ::library::GetLibraryStatsRequest request;
    std::string user_id = "11111111-1111-1111-1111-111111111111";
    request.set_user_id(user_id);

    EXPECT_CALL(mock_repo_, GetLibraryStats(testing::Eq(user_id)))
        .WillOnce(testing::Return(42));

    auto client = MakeClient<::library::LibraryServiceClient>();
    auto response = client.GetLibraryStats(request);

    EXPECT_EQ(response.count_library_entries(), 42);
}

UTEST_F(LibraryServiceTest, GetLibraryStats_EmptyUser)
{
    ::library::GetLibraryStatsRequest request;
    request.set_user_id("");

    auto client = MakeClient<::library::LibraryServiceClient>();

    try
    {
        client.GetLibraryStats(request);
        FAIL() << "Expected INVALID_ARGUMENT";
    }
    catch (const userver::ugrpc::client::ErrorWithStatus& e)
    {
        EXPECT_EQ(e.GetStatus().error_code(),
                  grpc::StatusCode::INVALID_ARGUMENT);
    }
}

UTEST_F(LibraryServiceTest, GetLibraryStats_DbError)
{
    ::library::GetLibraryStatsRequest request;
    request.set_user_id("valid-uuid");

    EXPECT_CALL(mock_repo_, GetLibraryStats(_))
        .WillOnce(testing::Throw(std::runtime_error("DB connection failed")));

    auto client = MakeClient<::library::LibraryServiceClient>();

    try
    {
        client.GetLibraryStats(request);
        FAIL() << "Expected INTERNAL error";
    }
    catch (const userver::ugrpc::client::ErrorWithStatus& e)
    {
        EXPECT_EQ(e.GetStatus().error_code(), grpc::StatusCode::INTERNAL);
    }
}

UTEST_F(LibraryServiceTest, GetUserLibrary_FoundEntries)
{
    std::string user_id = "22222222-2222-2222-2222-222222222222";

    ::library::GetUserLibraryRequest request;
    request.set_user_id(user_id);
    request.set_limit(10);
    request.set_offset(0);

    std::vector<entities::LibraryPostgres> db_entries;

    db_entries.push_back(
        library_service::test::CreateFakeLibraryEntry(user_id));
    db_entries.push_back(
        library_service::test::CreateFakeLibraryEntry(user_id));

    EXPECT_CALL(mock_repo_, GetLibraryEntries(testing::Eq(user_id),
                                              testing::Eq(10), testing::Eq(0)))
        .WillOnce(testing::Return(db_entries));

    auto client = MakeClient<::library::LibraryServiceClient>();
    auto response = client.GetUserLibrary(request);

    EXPECT_EQ(response.entries_size(), 2);
    EXPECT_EQ(response.entries(0).user_id(), user_id);
    EXPECT_EQ(response.entries(0).status(),
              ::library::GameStatus::GAME_STATUS_PLAYING);
}

UTEST_F(LibraryServiceTest, GetUserLibrary_Empty)
{
    ::library::GetUserLibraryRequest request;
    request.set_user_id("valid-uuid");

    EXPECT_CALL(mock_repo_, GetLibraryEntries(_, _, _))
        .WillOnce(testing::Return(std::vector<entities::LibraryPostgres>{}));

    auto client = MakeClient<::library::LibraryServiceClient>();
    auto response = client.GetUserLibrary(request);

    EXPECT_EQ(response.entries_size(), 0);
}

UTEST_F(LibraryServiceTest, GetUserLibrary_Validation)
{
    ::library::GetUserLibraryRequest request;

    auto client = MakeClient<::library::LibraryServiceClient>();

    try
    {
        client.GetUserLibrary(request);
        FAIL() << "Expected INVALID_ARGUMENT";
    }
    catch (const userver::ugrpc::client::ErrorWithStatus& e)
    {
        EXPECT_EQ(e.GetStatus().error_code(),
                  grpc::StatusCode::INVALID_ARGUMENT);
    }
}

UTEST_F(LibraryServiceTest, UpdateLibraryEntry_Success)
{
    std::string user_id = "11111111-1111-1111-1111-111111111111";
    std::string game_id = "99999999-9999-9999-9999-999999999999";

    ::library::UpdateLibraryEntryRequest request;
    request.set_user_id(user_id);
    request.set_game_id(game_id);
    request.set_status(::library::GameStatus::GAME_STATUS_COMPLETED);

    auto success_result =
        library_service::test::CreateFakeLibraryEntry(user_id);

    EXPECT_CALL(mock_repo_, CreateLibraryEntry(testing::Eq(user_id),
                                               testing::Eq(game_id), _))
        .Times(1)
        .WillOnce(testing::Return(success_result));

    auto client = MakeClient<::library::LibraryServiceClient>();

    EXPECT_NO_THROW(client.UpdateLibraryEntry(request));
}

UTEST_F(LibraryServiceTest, UpdateLibraryEntry_MissingFields)
{
    ::library::UpdateLibraryEntryRequest request;
    request.set_user_id("user-uuid");

    auto client = MakeClient<::library::LibraryServiceClient>();

    try
    {
        client.UpdateLibraryEntry(request);
        FAIL() << "Expected INVALID_ARGUMENT";
    }
    catch (const userver::ugrpc::client::ErrorWithStatus& e)
    {
        EXPECT_EQ(e.GetStatus().error_code(),
                  grpc::StatusCode::INVALID_ARGUMENT);
    }
}

UTEST_F(LibraryServiceTest, UpdateLibraryEntry_DbError)
{
    ::library::UpdateLibraryEntryRequest request;
    request.set_user_id("u");
    request.set_game_id("g");
    request.set_status(::library::GameStatus::GAME_STATUS_PLAN);

    EXPECT_CALL(mock_repo_, CreateLibraryEntry(_, _, _))
        .WillOnce(testing::Throw(
            std::runtime_error("Unique violation or something")));

    auto client = MakeClient<::library::LibraryServiceClient>();

    try
    {
        client.UpdateLibraryEntry(request);
        FAIL() << "Expected INTERNAL";
    }
    catch (const userver::ugrpc::client::ErrorWithStatus& e)
    {
        EXPECT_EQ(e.GetStatus().error_code(), grpc::StatusCode::INTERNAL);
    }
}