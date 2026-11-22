cmake_minimum_required(VERSION 3.16)
project(frame_stream CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -O2)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-g)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB APP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(frame_stream ${APP_SRC})

# 压测客户端（第 2 周加入 tools/bench.cpp 后自动生效）
file(GLOB BENCH_SRC ${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp)
if(BENCH_SRC)
  add_executable(bench ${BENCH_SRC})
endif()

enable_testing()
file(GLOB TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
foreach(t ${TEST_SRC})
  get_filename_component(name ${t} NAME_WE)
  add_executable(${name} ${t})
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  add_test(NAME ${name} COMMAND ${name})
endforeach()
