include(GoogleTest)

add_executable(unit_tests
    core/test_metrics_json.cpp
    core/test_thread_safe_queue.cpp
    core/test_packet_builder.cpp
    core/test_backup.cpp

    ${CMAKE_SOURCE_DIR}/src/core/backup.cpp
    ${CMAKE_SOURCE_DIR}/src/core/packet_builder.cpp
    ${CMAKE_SOURCE_DIR}/src/core/agent.cpp
    ${CMAKE_SOURCE_DIR}/src/core/thread_safe_queue.h)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
    nlohmann_json::nlohmann_json
)

gtest_discover_tests(unit_tests)