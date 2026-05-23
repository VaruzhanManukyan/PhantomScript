cmake_minimum_required(VERSION 3.20)
project(PhantomService LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(nlohmann_json REQUIRED)

find_package(PostgreSQL REQUIRED)

file(GLOB SOURCES "*.cpp")
add_executable(phantom_service ${SOURCES})

target_include_directories(phantom_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

target_link_libraries(phantom_service PRIVATE Boost::system Boost::thread nlohmann_json::nlohmann_json
    PostgreSQL::PostgreSQL
)
