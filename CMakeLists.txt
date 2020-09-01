
cmake_minimum_required(VERSION 2.8.9)
project(PBLogger)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

if (${MACOSX})
    include_directories(/usr/local/include/)
endif()

set (CMAKE_CXX_STANDARD 11)

# Include external libs
include_directories(
    lib
    lib/websocketpp
    lib/json/include
)

add_subdirectory(
    lib/json
)

# Add all sources
file(GLOB SOURCES "src/*.cpp")

add_executable(PBLogger ${SOURCES})
target_link_libraries(PBLogger 
    boost_system
    boost_filesystem
    pthread
    nlohmann_json::nlohmann_json
)
