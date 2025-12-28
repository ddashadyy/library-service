#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/clients/http/middlewares/pipeline_component.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/ugrpc/server/component_list.hpp>

#include <userver/utils/daemon_run.hpp>

#include <handlers/library_grpc.hpp>

int main(int argc, char* argv[])
{
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<userver::server::handlers::Ping>()
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::components::HttpClient>()
            .Append<userver::components::HttpClientCore>()
            .Append<userver::clients::http::MiddlewarePipelineComponent>()
            .Append<userver::clients::dns::Component>()
            .Append<userver::server::handlers::TestsControl>()
            .AppendComponentList(userver::ugrpc::server::MinimalComponentList())
            .Append<userver::components::Postgres>("playhub-library-db")
            .Append<library_service::LibraryServiceComponent>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}