#pragma once

#include <library/library.pb.h>
#include <google/protobuf/timestamp.pb.h>
#include <userver/storages/postgres/io/chrono.hpp>

namespace utils {

::google::protobuf::Timestamp TimePointToProtobuf(
    const userver::storages::postgres::TimePointWithoutTz& time_point);

std::string_view GameStatusToString(::library::GameStatus status);

} // namespace utils