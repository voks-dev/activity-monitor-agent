cmake_minimum_required(VERSION 3.28)

project(activity-monitor-agent
    VERSION 1.0.0
    DESCRIPTION "Агент мониторинга активности пользователя"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.45.0
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(nlohmann_json cpp_httplib googletest)

add_executable(activity-monitor-agent
    src/main.cpp

    src/core/agent.cpp
    src/core/application.cpp
    src/core/backup.cpp
    src/core/packet_builder.cpp

    src/platform/windows/wide_to_utf8.cpp
    src/platform/windows/windows_metrics_collector.cpp
    src/platform/windows/windows_stop_signal.cpp
    src/platform/windows/windows_system_info.cpp

    src/transport/console/console_transport.cpp
    src/transport/http/http_transport.cpp
)

target_include_directories(activity-monitor-agent PRIVATE src)

target_link_libraries(activity-monitor-agent PRIVATE
    nlohmann_json::nlohmann_json
    httplib::httplib
)

if(WIN32)
    target_link_libraries(activity-monitor-agent PRIVATE
        kernel32
        user32
    )
endif()


list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(ClangFormat)


enable_testing()
add_subdirectory(tests)
