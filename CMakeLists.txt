cmake_minimum_required(VERSION 3.10)

project(osscli_test)

file(GLOB_RECURSE CPP_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)

LIST(APPEND _sources ${CPP_FILES})

LIST(APPEND _libpath sdk/lib third-party/boost/lib)

LIST(APPEND _headers sdk/include include third-party/boost/include third-party/nlohmann/include)

LIST(APPEND _libs alibabacloud-oss-cpp-sdk ssl curl crypto pthread dl boost_program_options)

include_directories(${_headers})

link_directories(${_libpath})

add_executable(osscli main.cpp ${_sources})

target_compile_options(osscli PRIVATE "-std=c++14")

target_link_libraries(osscli PRIVATE ${_libs})