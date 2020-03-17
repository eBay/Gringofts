cmake_minimum_required(VERSION 3.12)
project(app_util)
MESSAGE(STATUS "processing app_util")

# Generated sources for app_demo
set(proto_generated_dir "generated/grpc")
file(MAKE_DIRECTORY ${proto_generated_dir})

# netadmin.proto
ADD_PROTO_RESOURCE(
        netadmin_proto_library
        "protos/netadmin.proto"
        ${proto_generated_dir})

# Source files
set(APP_UTIL_SRC
        AppInfo.h
        CommandEventDecoderImpl.h
        CommandProcessLoop.h
        EventApplyLoop.h
        NetAdminServer.h
        NetAdminServiceProvider.h)

# Library
set(APP_UTIL_LIBS boost_system.a ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF} lib_inih)

add_library(app_util STATIC
        ${APP_UTIL_SRC})
add_dependencies(app_util netadmin_proto_library)
target_link_libraries(app_util netadmin_proto_library ${APP_UTIL_LIBS})