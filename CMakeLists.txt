cmake_minimum_required(VERSION 3.9)
project(event-emitter)

set(CMAKE_CXX_STANDARD 17)

file(GLOB LIB_HDR_FILES "include/*.h")
file(GLOB LIB_SRC_FILES "src/*.cpp")

add_library(event-emitter STATIC ${LIB_HDR_FILES} ${LIB_SRC_FILES})
target_include_directories(event-emitter PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

add_subdirectory(examples)

enable_testing()

add_executable(TEST_event-emitter "tests/main.cpp")
target_link_libraries(TEST_event-emitter event-emitter)
add_test(NAME alltests COMMAND TEST_event-emitter)
