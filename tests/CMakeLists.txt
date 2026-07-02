find_package(GTest REQUIRED)

# Selector sources
set(SELECTOR_SOURCES
    ${CMAKE_SOURCE_DIR}/scheduler/selectors/fcfs_selector.cpp
    ${CMAKE_SOURCE_DIR}/scheduler/selectors/sjf_selector.cpp
    ${CMAKE_SOURCE_DIR}/scheduler/selectors/adaptive_selector.cpp
)

# DB + store + metrics implementation sources
set(IMPL_SOURCES
    ${CMAKE_SOURCE_DIR}/scheduler/scheduler_db.cpp
    ${CMAKE_SOURCE_DIR}/worker/ebpf_metric_store.cpp
    ${CMAKE_SOURCE_DIR}/worker/metrics_collector.cpp
)

add_executable(run_tests
    test_selectors.cpp
    test_database.cpp
    test_metric_store.cpp
    test_metrics.cpp
    ${SELECTOR_SOURCES}
    ${IMPL_SOURCES}
)

target_include_directories(run_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/scheduler
    ${CMAKE_SOURCE_DIR}/shared
    ${CMAKE_SOURCE_DIR}/worker
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(run_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    shared
    djs_proto
    SQLite3::SQLite3
)
