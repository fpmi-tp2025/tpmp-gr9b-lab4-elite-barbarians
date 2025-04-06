cmake_minimum_required(VERSION 3.8)
project("test")

# Включаем тестирование
enable_testing()

# Добавляем Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

# Основной исполняемый файл
add_executable(test 
    "src/main.cpp" 
    "src/sql.h" 
    "src/sqlite3.h" 
    "src/sqlite3.c"
)

# Тесты
add_executable(test_main
    "tests/test_main.cpp"
    "src/sql.h"
    "src/sqlite3.h"
    "src/sqlite3.c"
)

target_link_libraries(test_main GTest::gtest_main)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET test PROPERTY CXX_STANDARD 20)
    set_property(TARGET test_main PROPERTY CXX_STANDARD 20)
endif()

include(GoogleTest)
gtest_discover_tests(test_main)