cmake_minimum_required(VERSION 3.12)
project(app_util)
MESSAGE(STATUS "processing app_util")

# Generated sources for app_demo
set(gringofts_proto_generated_dir "generated/grpc")
file(MAKE_DIRECTORY ${gringofts_proto_generated_dir})

# netadmin.proto
ADD_PROTO_RESOURCE(
        gringofts_netadmin_proto_library
        "protos/netadmin.proto"
        ${gringofts_proto_generated_dir})

# Library
set(GRINGOFTS_APP_UTIL_LIBS ${Boost_LIBRARIES} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF} lib_inih)

add_library(gringofts_app_util INTERFACE)
target_link_libraries(gringofts_app_util INTERFACE gringofts_netadmin_proto_library ${GRINGOFTS_APP_UTIL_LIBS})
