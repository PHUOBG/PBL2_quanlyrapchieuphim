cmake_minimum_required(VERSION 3.16)
project(PBL2-main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static libs
set(SFML_STATIC_LIBRARIES TRUE)

# Tìm SFML
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window Network System CONFIG)

# Tự động tìm tất cả file .cpp trong thư mục src/
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

# Tạo executable
add_executable(app ${SRC_FILES})

set_target_properties(app PROPERTIES WIN32_EXECUTABLE TRUE)

# Thêm đường dẫn header
target_include_directories(app PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/data-structures
    ${CMAKE_SOURCE_DIR}/include/models
    ${CMAKE_SOURCE_DIR}/include/services
    ${CMAKE_SOURCE_DIR}/include/utils
    ${CMAKE_SOURCE_DIR}/include/UI/components
    ${CMAKE_SOURCE_DIR}/include/UI/screens
)

# Link với SFML 3
target_link_libraries(app PRIVATE SFML::Graphics SFML::Window SFML::Network SFML::System)

# Warnings (helps spot unused code and risky patterns)
if(MSVC)
    target_compile_options(app PRIVATE /W4 /utf-8)
else()
    target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# Fix conflict cho Windows
if(WIN32)
    target_compile_definitions(app PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
