cmake_minimum_required(VERSION 3.20)

project(LZEnd LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(fse
    GIT_REPOSITORY https://github.com/Cyan4973/FiniteStateEntropy.git
    GIT_TAG 9ba32518ad023cd4c3726fb44bba5b768ef291c8
)
FetchContent_MakeAvailable(fse)

add_library(fse STATIC
    "${fse_SOURCE_DIR}/lib/entropy_common.c"
    "${fse_SOURCE_DIR}/lib/fse_compress.c"
    "${fse_SOURCE_DIR}/lib/fse_decompress.c"
    "${fse_SOURCE_DIR}/lib/fseU16.c"
    "${fse_SOURCE_DIR}/lib/huf_compress.c"
    "${fse_SOURCE_DIR}/lib/huf_decompress.c"
)

target_include_directories(fse SYSTEM PUBLIC
    "${fse_SOURCE_DIR}/lib"
)

file(GLOB_RECURSE LZEND_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(lzend ${LZEND_SOURCES})

target_link_libraries(lzend PRIVATE fse)

target_include_directories(lzend PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/lz_end"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/bit_utils"
)
