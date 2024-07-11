set(target librtsn)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup()

qt_add_library("${target}" STATIC
    parse.cpp
    serialize.cpp
)

target_link_libraries("${target}" PRIVATE Qt6::Core)
target_link_libraries("${target}" PRIVATE libapi libmatchstate)
target_include_directories("${target}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_options(${target} PRIVATE -Wall -Wextra)
