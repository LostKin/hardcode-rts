set(target libapi)

set(proto_sources "")

foreach(prototype entities error requests responses)
    set(proto_name "${prototype}")
    get_filename_component(proto "${PROJECT_SOURCE_DIR}/api/${proto_name}.proto" ABSOLUTE)
    get_filename_component(proto_dir "${proto}" DIRECTORY)

    set(proto_src "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")
    set(proto_hdr "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")
    add_custom_command(
        OUTPUT "${proto_src}" "${proto_hdr}"
        COMMAND protoc -I"${proto_dir}" --cpp_out="${CMAKE_CURRENT_BINARY_DIR}" "${proto}"
        DEPENDS "${proto}"
    )

    list(APPEND proto_sources "${proto_src}")
endforeach()

add_library("${target}" STATIC "${proto_sources}")

target_include_directories("${target}" PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries("${target}" PRIVATE "-lprotobuf")

target_compile_options(${target} PRIVATE -Wall -Wextra)
