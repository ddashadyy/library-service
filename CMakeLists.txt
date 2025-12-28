cmake_minimum_required(VERSION 3.12...3.31)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

project(library-service CXX)


if(ENABLE_COVERAGE)
  message(STATUS "Enabling Code Coverage with Clang/GCC")

  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(userver COMPONENTS core postgresql grpc QUIET)
if(NOT userver_FOUND)
  download_userver(TRY_DIR third_party/userver VERSION 2.8)
endif()

userver_setup_environment()

include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_library(${PROJECT_NAME}_objs OBJECT
    include/repository/postgres_manager.hpp
    include/repository/repository.hpp
    src/repository/postgres_manager.cpp

    include/handlers/library_grpc.hpp
    src/handlers/library_grpc.cpp

    include/tools/utils.hpp
    src/tools/utils.cpp

    include/structs/library_postgres.hpp
)

# Линкуем стандартные зависимости userver
target_link_libraries(${PROJECT_NAME}_objs PUBLIC 
    userver::core
    userver::postgresql 
    userver::grpc
)

target_include_directories(${PROJECT_NAME}_objs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(${PROJECT_NAME}_objs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/playhub-proto/proto")
set(LIBRARY_SERVICE_PROTO_FILE "${PROTO_ROOT}/library/library.proto") 
set(GENERATED_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/generated")

userver_add_grpc_library(
    ${PROJECT_NAME}_proto
    PROTOS ${LIBRARY_SERVICE_PROTO_FILE}
    SOURCE_PATH ${PROTO_ROOT}
    OUTPUT_PATH ${GENERATED_ROOT}
)

target_link_libraries(${PROJECT_NAME}_objs PUBLIC ${PROJECT_NAME}_proto)
target_include_directories(${PROJECT_NAME}_objs PUBLIC ${GENERATED_ROOT})

# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

# unittests
add_library(${PROJECT_NAME}_tests OBJECT
    tests/library_service_test.cpp
    tests/utils_test.cpp
)

target_include_directories(${PROJECT_NAME}_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GENERATED_ROOT} 
)

target_link_libraries(${PROJECT_NAME}_tests PUBLIC
    userver::utest
    ${PROJECT_NAME}_proto
)

add_executable(${PROJECT_NAME}-unittest)
target_link_libraries(${PROJECT_NAME}-unittest
    PRIVATE
    ${PROJECT_NAME}_tests
    ${PROJECT_NAME}_objs
    userver::utest
    userver::grpc-utest
)

add_google_tests(${PROJECT_NAME}-unittest)

include(GNUInstallDirs)

if(DEFINED ENV{PREFIX})
  message(STATUS "Set install prefix: $ENV{PREFIX}")
  file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
  set(CMAKE_INSTALL_PREFIX "${PREFIX_PATH}")
endif()

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${PROJECT_NAME})
install(FILES ${CONFIGS_FILES} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME} COMPONENT ${PROJECT_NAME})