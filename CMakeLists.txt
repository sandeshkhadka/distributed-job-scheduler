cmake_minimum_required(VERSION 3.20)
project(distributed_job_scheduler CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS "shared/*.cpp")

# shared library
find_package(SQLite3 REQUIRED)
add_library(shared STATIC ${SHARED_SOURCES} )
target_include_directories(shared PUBLIC "${CMAKE_SOURCE_DIR}/shared")

# for older cmake version where SQLite::SQLite3 is the target package
if(TARGET SQLite::SQLite3 AND NOT TARGET SQLite3::SQLite3)
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif()

target_link_libraries(shared PUBLIC SQLite3::SQLite3)

# GRPC and PROTOBUF stuff
find_package(gRPC CONFIG REQUIRED)


set(PROTO_FILE ${CMAKE_SOURCE_DIR}/proto/scheduler.proto)
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(PROTO_SRCS ${GENERATED_DIR}/scheduler.pb.cc)
set(PROTO_HDRS ${GENERATED_DIR}/scheduler.pb.h)
set(GRPC_SRCS ${GENERATED_DIR}/scheduler.grpc.pb.cc)
set(GRPC_HDRS ${GENERATED_DIR}/scheduler.grpc.pb.h)

add_custom_command(
  OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
  COMMAND protobuf::protoc
  ARGS --cpp_out=${GENERATED_DIR}
       -I ${CMAKE_SOURCE_DIR}/proto
       ${PROTO_FILE}
  DEPENDS ${PROTO_FILE}
)

add_custom_command(
  OUTPUT ${GRPC_SRCS} ${GRPC_HDRS}
  COMMAND protobuf::protoc
  ARGS --grpc_out=${GENERATED_DIR}
       --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
       -I ${CMAKE_SOURCE_DIR}/proto
       ${PROTO_FILE}
  DEPENDS ${PROTO_FILE}
)

add_library(djs_proto STATIC
  ${PROTO_SRCS}
  ${GRPC_SRCS}
)

target_include_directories(djs_proto PUBLIC ${GENERATED_DIR})
target_link_libraries(djs_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf )

# Executor library (shared between worker and djs-executor)
file(GLOB_RECURSE EXECUTOR_SOURCES CONFIGURE_DEPENDS "worker/executors/*.cpp")
add_library(executor_impl STATIC ${EXECUTOR_SOURCES})
target_include_directories(executor_impl PUBLIC "${CMAKE_SOURCE_DIR}/worker")

option(USE_PG "Use PostgreSQL backend (scheduler + admin)" OFF)

if(USE_PG)
  find_package(PostgreSQL REQUIRED)
endif()

# eBPF monitor target
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBBPF libbpf REQUIRED)

set(BPF_SOURCE ${CMAKE_SOURCE_DIR}/worker/ebpf/bpf/job_monitor.bpf.c)
set(BPF_OBJECT ${CMAKE_BINARY_DIR}/bpf/job_monitor.bpf.o)
add_custom_command(
  OUTPUT ${BPF_OBJECT}
  COMMAND clang -g -O2 -target bpf -c ${BPF_SOURCE} -o ${BPF_OBJECT}
            -I${CMAKE_SOURCE_DIR}/worker/ebpf/bpf
            -I/usr/include/bpf
  DEPENDS ${BPF_SOURCE}
  COMMENT "Compiling BPF object"
)
add_custom_target(bpf_objects DEPENDS ${BPF_OBJECT})

add_executable(djs-ebpf-monitor
  worker/ebpf/ebpf_main.cpp
  worker/ebpf/ebpf_monitor.cpp
)
target_include_directories(djs-ebpf-monitor PRIVATE
  "${CMAKE_SOURCE_DIR}/worker"
  "${CMAKE_SOURCE_DIR}/shared"
  ${LIBBPF_INCLUDE_DIRS}
)
target_link_libraries(djs-ebpf-monitor PRIVATE ${LIBBPF_LIBRARIES} ${LIBBPF_LDFLAGS} shared)
add_dependencies(djs-ebpf-monitor bpf_objects)
add_custom_command(TARGET djs-ebpf-monitor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${BPF_OBJECT}
          $<TARGET_FILE_DIR:djs-ebpf-monitor>/bpf/job_monitor.bpf.o
)

# helper macro to add target
macro(add_component NAME)
  file(GLOB_RECURSE _SOURCES CONFIGURE_DEPENDS "${NAME}/*.cpp")
  if ("${NAME}" STREQUAL "worker")
    list(FILTER _SOURCES EXCLUDE REGEX ".*/executors/.*")
    list(FILTER _SOURCES EXCLUDE REGEX ".*/ebpf/ebpf_monitor\\.cpp$")
    list(FILTER _SOURCES EXCLUDE REGEX ".*/ebpf/ebpf_main\\.cpp$")
  endif()
  if(USE_PG)
    if("${NAME}" STREQUAL "scheduler")
      list(FILTER _SOURCES EXCLUDE REGEX ".*/scheduler_db\\.cpp$")
    elseif("${NAME}" STREQUAL "admin")
      list(FILTER _SOURCES EXCLUDE REGEX ".*/admin_db\\.cpp$")
    endif()
  else()
    if("${NAME}" STREQUAL "scheduler" OR "${NAME}" STREQUAL "admin")
      list(FILTER _SOURCES EXCLUDE REGEX ".*/pg_.*\\.cpp$")
    endif()
  endif()
  add_executable(${NAME} ${_SOURCES})
  target_include_directories(${NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/${NAME}"
    "${CMAKE_SOURCE_DIR}/shared"
    ${GENERATED_DIR}
  )
  target_link_libraries(${NAME} PUBLIC shared djs_proto )
endmacro()


# Targets
add_component(scheduler)
set_target_properties(scheduler PROPERTIES OUTPUT_NAME "djs-sc")
add_component(cli)
set_target_properties(cli PROPERTIES OUTPUT_NAME "djs-cli")
add_component(worker)
set_target_properties(worker PROPERTIES OUTPUT_NAME "djs-wk")
add_component(admin)

# Wire executor_impl into worker
target_link_libraries(worker PUBLIC executor_impl)

if(USE_PG)
  target_compile_definitions(scheduler PRIVATE USE_PG=1)
  target_compile_definitions(admin PRIVATE USE_PG=1)
  target_link_libraries(scheduler PRIVATE PostgreSQL::PostgreSQL)
  target_link_libraries(admin PRIVATE PostgreSQL::PostgreSQL)
  target_include_directories(admin PRIVATE "${CMAKE_SOURCE_DIR}/scheduler")
  target_sources(admin PRIVATE "${CMAKE_SOURCE_DIR}/scheduler/pg_db.cpp")
else()
  target_compile_definitions(scheduler PRIVATE USE_PG=0)
  target_compile_definitions(admin PRIVATE USE_PG=0)
endif()

# djs-executor: standalone binary for isolated job execution
add_executable(djs-executor executor/main.cpp)
target_include_directories(djs-executor PRIVATE "${CMAKE_SOURCE_DIR}/worker")
target_link_libraries(djs-executor executor_impl)
