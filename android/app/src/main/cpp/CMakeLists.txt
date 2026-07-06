cmake_minimum_required(VERSION 3.22.1)
project(fc_empire_datdb LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(datdb SHARED
        DatReader.cpp
        DatParser.cpp
        NativeDbBridge.cpp
        NativeMatchBridge.cpp
        match_engine.cpp
        player.cpp
        team.cpp
        ball.cpp
        tactics.cpp
        simulation.cpp)

target_include_directories(datdb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

find_library(LOG_LIB log REQUIRED)
find_library(ANDROID_LIB android REQUIRED)

# Android expose SQLite as libsqlite.so on many NDK/platform combinations.
# Some templates search sqlite3, which can make CMake die before compilation.
find_library(SQLITE_LIB NAMES sqlite sqlite3)

if(SQLITE_LIB)
    target_compile_definitions(datdb PRIVATE FCEMPIRE_HAS_SQLITE=1)
    target_link_libraries(datdb PRIVATE ${LOG_LIB} ${ANDROID_LIB} ${SQLITE_LIB})
else()
    # Fallback de compilation: la couche Java garde le cache SQLite Android.
    # Le lecteur C++ reste disponible pour inspecter les .dat sans bloquer le build.
    target_compile_definitions(datdb PRIVATE FCEMPIRE_HAS_SQLITE=0)
    target_link_libraries(datdb PRIVATE ${LOG_LIB} ${ANDROID_LIB})
endif()
