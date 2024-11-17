cmake_minimum_required(VERSION 3.15)
project(pizzarealgame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set vcpkg paths
set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows")
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Find required packages
find_package(CURL CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(pizzarealgame 
    src/main.cpp
    src/game.cpp
    src/pizza.cpp
    src/order.cpp
    src/player.cpp
)

target_include_directories(pizzarealgame 
    PRIVATE include
    PRIVATE ${CURL_INCLUDE_DIRS}
    PRIVATE ${unofficial_sqlite3_INCLUDE_DIRS}
)

target_link_libraries(pizzarealgame 
    PRIVATE 
    CURL::libcurl
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
)
