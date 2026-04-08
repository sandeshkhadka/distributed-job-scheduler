cmake_minimum_required(VERSION 3.20)
project(distributed_job_scheduler CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS "shared/*.cpp")

add_library(shared STATIC ${SHARED_SOURCES})
target_include_directories(shared PUBLIC "${CMAKE_SOURCE_DIR}/shared")

# helper macro to add target
macro(add_component NAME)
  file(GLOB_RECURSE _SOURCES CONFIGURE_DEPENDS "${NAME}/*.cpp")
  add_executable(${NAME} ${_SOURCES})
  target_include_directories(${NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/${NAME}"
    "${CMAKE_SOURCE_DIR}/shared"
  )
endmacro()

# Targets
add_component(scheduler)
add_component(cli)
add_component(worker)
