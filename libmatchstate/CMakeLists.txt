set(target libmatchstate)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup()

qt_add_library("${target}" STATIC
    matchstate.cpp
    matchstate_clientfromplayer.cpp
    matchstate_clientfromserver.cpp
    matchstate_serverfromclient.cpp
    matchstate_tick.cpp
)

target_link_libraries("${target}" PRIVATE Qt6::Core)
target_link_libraries("${target}" PRIVATE libhardcode)
target_include_directories("${target}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
