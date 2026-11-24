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
list(FILTER SRC_FILES EXCLUDE REGEX ".*tempCodeRunnerFile.*")

# Tạo executable
# Core library (everything except main.cpp) so the app and the automated tests share the same code
set(CORE_SRC_FILES ${SRC_FILES})
list(FILTER CORE_SRC_FILES EXCLUDE REGEX ".*/src/main\\.cpp$")
add_library(cinecore_core STATIC ${CORE_SRC_FILES})

add_executable(app src/main.cpp)
if(WIN32)
    set_target_properties(app PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(cinecore_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/data-structures
    ${CMAKE_SOURCE_DIR}/include/models
    ${CMAKE_SOURCE_DIR}/include/repositories
    ${CMAKE_SOURCE_DIR}/include/services
    ${CMAKE_SOURCE_DIR}/include/utils
    ${CMAKE_SOURCE_DIR}/include/UI
    ${CMAKE_SOURCE_DIR}/include/UI/components
    ${CMAKE_SOURCE_DIR}/include/UI/screens
)

target_link_libraries(cinecore_core PUBLIC SFML::Graphics SFML::Window SFML::Network SFML::System)
target_link_libraries(app PRIVATE cinecore_core)

# Warnings (helps spot unused code and risky patterns)
if(MSVC)
    target_compile_options(cinecore_core PRIVATE /W4 /utf-8)
    target_compile_options(app PRIVATE /W4 /utf-8)
else()
    target_compile_options(cinecore_core PRIVATE -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8)
    target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# Fix conflict cho Windows
if(WIN32)
    target_compile_definitions(cinecore_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ---- Automated tests (run: ctest --test-dir build --output-on-failure) ----
enable_testing()
add_executable(core_tests tests/test_main.cpp)
target_link_libraries(core_tests PRIVATE cinecore_core)
add_test(NAME core_tests COMMAND core_tests)
