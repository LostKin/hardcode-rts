set(target rts_server)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

qt_standard_project_setup()

qt_add_executable("${target}"
    main.cpp
    application.cpp
    network_manager.cpp
    network_thread.cpp
    room.cpp
    room_thread.cpp
)

target_link_libraries("${target}" PRIVATE Qt6::Core Qt6::Network)
target_link_libraries("${target}" PRIVATE libhccn libmatchstate librtsn libapi)
