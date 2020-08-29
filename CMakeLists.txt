
cmake_minimum_required(VERSION 2.8.9)
project(PBLogger)

set (CMAKE_CXX_STANDARD 11)

# Include external libs
include_directories(
    lib
    lib/websocketpp
)

# Add all sources
file(GLOB SOURCES "src/*.cpp")

add_executable(PBLogger ${SOURCES})
target_link_libraries(PBLogger 
    boost_system
    pthread
)
