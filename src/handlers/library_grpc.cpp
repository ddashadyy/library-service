#include <handlers/library_grpc.hpp>

#include <userver/storages/postgres/component.hpp>

#include <boost/uuid/uuid_io.hpp>
#include <tools/utils.hpp>

namespace library_service {

LibraryService::LibraryService(std::string prefix,
                               const pg::ILibraryRepository& manager)
    : prefix_(std::move(prefix)), pg_manager_(manager)
{}

::library::LibraryServiceBase::UpdateLibraryEntryResult
LibraryService::UpdateLibraryEntry(
    CallContext& context, ::library::UpdateLibraryEntryRequest&& request)
{
    if (request.user_id().empty())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "user_id cannot be empty");
    }
    if (request.game_id().empty())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "game_id cannot be empty");
    }

    try
    {
        std::string_view db_status_str =
            utils::GameStatusToString(request.status());

        const auto kUpsertedLibraryEntry = pg_manager_.CreateLibraryEntry(
            request.user_id(), request.game_id(), db_status_str);

        if (kUpsertedLibraryEntry.user_id.is_nil())
        {
            LOG_ERROR()
                << "Repository returned empty result for UpdateLibraryEntry. "
                << "User: " << request.user_id()
                << ", Game: " << request.game_id();
            return grpc::Status(grpc::StatusCode::INTERNAL,
                                "Failed to update library entry");
        }

        ::library::UpdateLibraryEntryResponse response;
        FillLibraryEntry(kUpsertedLibraryEntry, *response.mutable_entry());

        return response;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR() << "Exception in UpdateLibraryEntry: " << e.what();
        return grpc::Status(grpc::StatusCode::INTERNAL,
                            "Internal service error");
    }
}

::library::LibraryServiceBase::GetUserLibraryResult
LibraryService::GetUserLibrary(CallContext& context,
                               ::library::GetUserLibraryRequest&& request)
{
    if (request.user_id().empty())
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "user_id cannot be empty");

    try
    {
        const auto db_entries = pg_manager_.GetLibraryEntries(
            request.user_id(), utils::GameStatusToString(request.status()),
            request.limit(), request.offset());

        ::library::GetUserLibraryResponse response;
        response.mutable_entries()->Reserve(db_entries.size());

        for (const auto& db_entry : db_entries)
        {
            auto* proto_entry = response.add_entries();
            FillLibraryEntry(db_entry, *proto_entry);
        }

        return response;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR() << "Failed to get user library: " << e.what();
        return grpc::Status(grpc::StatusCode::INTERNAL, "Database error");
    }
}

void LibraryService::FillLibraryEntry(const entities::LibraryPostgres& db_entry,
                                      ::library::LibraryEntry& proto)
{
    proto.set_user_id(boost::uuids::to_string(db_entry.user_id));
    proto.set_game_id(boost::uuids::to_string(db_entry.game_id));

    auto proto_status = static_cast<::library::GameStatus>(
        static_cast<int>(db_entry.game_status));
    proto.set_status(proto_status);

    *proto.mutable_created_at() =
        utils::TimePointToProtobuf(db_entry.created_at);
    *proto.mutable_updated_at() =
        utils::TimePointToProtobuf(db_entry.updated_at);
}

LibraryServiceComponent::LibraryServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::ugrpc::server::ServiceComponentBase(config, context),
      pg_manager_(
          context
              .FindComponent<userver::components::Postgres>("playhub-library-db")
              .GetCluster()),
      service_(config["library-prefix"].As<std::string>(), pg_manager_)
{
    RegisterService(service_);
}

userver::yaml_config::Schema LibraryServiceComponent::GetStaticConfigSchema()
{
    return userver::yaml_config::MergeSchemas<
        userver::ugrpc::server::ServiceComponentBase>(
        R"(
            type: object
            description: Library gRPC service component
            additionalProperties: false
            properties:
                library-prefix:
                    type: string
                    description: library prefix
                database:
                    type: object
                    description: Database connection settings
                    additionalProperties: false  
                    properties:
                        host:
                            type: string
                            description: Hostname or IP of the database server
                        port:
                            type: integer
                            description: Port of the database server
                        user:
                            type: string
                            description: Database user
                        password:
                            type: string
                            description: Database password
        )");
}

} // namespace library_service