# Function for build proto & grpc header and src
# Example: ADD_PROTO_RESOURCE(
#       demo_proto_library  # Lib name
#       "should_be_generated/domain/protos/journal.proto"  # proto file
#       "generated/grpc"  # build output generate dir
#       execution_proto_library  # dependency-1
#       account_proto_library  # dependency-2
#       )
function(ADD_PROTO_RESOURCE LIB_NAME FILE_LOC GEN_DIR)
    set(proto_generated_dir "${GEN_DIR}")

    get_filename_component(PROTO_NAME "${FILE_LOC}" NAME_WE)

    get_filename_component(${PROTO_NAME}_proto "${FILE_LOC}" ABSOLUTE)
    set(PROTO_ROOT ${${PROTO_NAME}_proto})

    get_filename_component(${PROTO_NAME}_proto_path "${PROTO_ROOT}" PATH)
    set(PROTO_PATH ${${PROTO_NAME}_proto_path})

    set(${PROTO_NAME}_proto_src "${proto_generated_dir}/${PROTO_NAME}.pb.cc")
    set(${PROTO_NAME}_proto_hdr "${proto_generated_dir}/${PROTO_NAME}.pb.h")
    set(${PROTO_NAME}_grpc_src  "${proto_generated_dir}/${PROTO_NAME}.grpc.pb.cc")
    set(${PROTO_NAME}_grpc_hdr  "${proto_generated_dir}/${PROTO_NAME}.grpc.pb.h")

    set(PROTO_SRC ${${PROTO_NAME}_proto_src})
    set(PROTO_HDR ${${PROTO_NAME}_proto_hdr})
    set(GRPC_SRC ${${PROTO_NAME}_grpc_src})
    set(GRPC_HDR ${${PROTO_NAME}_grpc_hdr})

    add_custom_command(
            OUTPUT "${PROTO_SRC}" "${PROTO_HDR}" "${GRPC_SRC}" "${GRPC_HDR}"
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --grpc_out "${proto_generated_dir}"
            --cpp_out "${proto_generated_dir}"
            -I "${PROTO_PATH}"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${PROTO_ROOT}"
            DEPENDS ${PROTO_ROOT}
            COMMENT "Generating proto API for \"${FILE_LOC}\".")

    add_library(${LIB_NAME} STATIC "${PROTO_SRC}" "${PROTO_HDR}" "${GRPC_SRC}" "${GRPC_HDR}")

    # if has dependency
    foreach(dep ${ARGN})
        add_dependencies(${LIB_NAME} ${dep})
    endforeach(dep)

endfunction(ADD_PROTO_RESOURCE)

# Function for build proto & grpc header and src
# Example: ADD_PROTO_SET(
#       demo_proto_library  # Lib name
#       "should_be_generated/domain/protos/journal.proto"  # proto file set
#       "generated/grpc"  # build output generate dir
#       execution_proto_library  # dependency-1
#       account_proto_library  # dependency-2
#       )
function(ADD_PROTO_SET LIB_NAME FILE_SET GEN_DIR)
    set(GEN_FILES "")

    foreach(filename ${FILE_SET})
        get_filename_component(PROTO_NAME "${filename}" NAME_WE)
        get_filename_component(${PROTO_NAME}_proto "${filename}" ABSOLUTE)
        get_filename_component(${PROTO_NAME}_proto_path "${${PROTO_NAME}_proto}" PATH)

        set(${PROTO_NAME}_PROTO_SRC "${GEN_DIR}/${PROTO_NAME}.pb.cc")
        set(${PROTO_NAME}_PROTO_HDR "${GEN_DIR}/${PROTO_NAME}.pb.h")
        set(${PROTO_NAME}_GRPC_SRC  "${GEN_DIR}/${PROTO_NAME}.grpc.pb.cc")
        set(${PROTO_NAME}_GRPC_HDR  "${GEN_DIR}/${PROTO_NAME}.grpc.pb.h")

        set(GEN_FILES ${GEN_FILES}
                "${${PROTO_NAME}_PROTO_SRC}"
                "${${PROTO_NAME}_PROTO_HDR}"
                "${${PROTO_NAME}_GRPC_SRC}"
                "${${PROTO_NAME}_GRPC_HDR}")

        add_custom_command(
                OUTPUT "${${PROTO_NAME}_PROTO_SRC}" "${${PROTO_NAME}_PROTO_HDR}"
                "${${PROTO_NAME}_GRPC_SRC}" "${${PROTO_NAME}_GRPC_HDR}"
                COMMAND ${_PROTOBUF_PROTOC}
                ARGS --grpc_out "${GEN_DIR}"
                --cpp_out "${GEN_DIR}"
                -I "${${PROTO_NAME}_proto_path}"
                --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
                "${${PROTO_NAME}_proto}"
                DEPENDS ${${PROTO_NAME}_proto}
                COMMENT "Generating proto API for \"${filename}\".")
    endforeach(filename)

    add_library(${LIB_NAME} STATIC ${GEN_FILES})

    # if has dependency
    foreach(dep ${ARGN})
        add_dependencies(${LIB_NAME} ${dep})
    endforeach(dep)
endfunction(ADD_PROTO_SET)