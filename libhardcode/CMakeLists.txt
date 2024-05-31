set(target libhardcode)

add_library("${target}" STATIC noop.c)

add_custom_command(
    TARGET "${target}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/hc/hardcode"
    COMMAND "./bootstrap"
    COMMAND "./configure" "--prefix=${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst"
    COMMAND "make"
    COMMAND "make" "install"
)

target_include_directories("${target}" PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories("${target}" PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/include")
target_link_libraries("${target}" PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/lib/libhcparser.a"
    "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/lib/libhcmodule.a"
    "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/lib/libhcsynrpn.a"
    "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/lib/libhcsnapshot.a"
    "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/lib/libhcbackendc.a"
    "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/lib/libhcsemrpn.a"
    "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/lib/libhcsemanalyzer.a"
    "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/lib/libhcsem.a"
    "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/lib/libhcobjectstream.a"
    "${CMAKE_CURRENT_BINARY_DIR}/hardcode-inst/lib/libhcsyn.a"
)
