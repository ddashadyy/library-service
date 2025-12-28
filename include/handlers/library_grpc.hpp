#pragma once

#include <library/library_service.usrv.pb.hpp>
#include <repository/postgres_manager.hpp>

namespace library_service {

class LibraryService final : public ::library::LibraryServiceBase
{
public:
    explicit LibraryService(std::string prefix,
                            const pg::ILibraryRepository& manager);

    UpdateLibraryEntryResult
    UpdateLibraryEntry(CallContext& context,
                       ::library::UpdateLibraryEntryRequest&& request) override;
    GetUserLibraryResult
    GetUserLibrary(CallContext& context,
                   ::library::GetUserLibraryRequest&& request) override;

private:
    void FillLibraryEntry(const entities::LibraryPostgres& db_entry,
                          ::library::LibraryEntry& proto);

    std::string prefix_;
    const pg::ILibraryRepository& pg_manager_;
};

class LibraryServiceComponent final
    : public userver::ugrpc::server::ServiceComponentBase
{
public:
    static constexpr std::string_view kName = "library-service";

    LibraryServiceComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context);

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    pg::PostgresManager pg_manager_;
    LibraryService service_;
};

} // namespace library_service