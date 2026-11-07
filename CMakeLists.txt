cmake_minimum_required(VERSION 3.20)
project(RecoverX VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(RecoverX WIN32
    src/main.cpp
    res/RecoverX.rc
)

target_include_directories(RecoverX PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/res")

if(MSVC)
    target_compile_options(RecoverX PRIVATE /utf-8 /W4 /permissive-)
    target_link_options(RecoverX PRIVATE /MANIFEST:NO)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /I\"${CMAKE_CURRENT_SOURCE_DIR}/res\"")
endif()

target_link_libraries(RecoverX PRIVATE
    user32
    gdi32
    shell32
    advapi32
    comctl32
    comdlg32
    winhttp
    powrprof
    uxtheme
)

set_target_properties(RecoverX PROPERTIES
    OUTPUT_NAME "RecoverX"
    WIN32_EXECUTABLE TRUE
)
