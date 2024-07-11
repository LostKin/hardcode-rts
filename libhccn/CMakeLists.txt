set(target libhccn)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

qt_standard_project_setup()

qt_add_library("${target}" STATIC
    client_to_server.cpp
    internal.cpp
    server_to_client.cpp
)

target_link_libraries("${target}" PRIVATE Qt6::Core Qt6::Network)
target_include_directories("${target}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_options(${target} PRIVATE -Wall -Wextra)
