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

# split.proto
ADD_PROTO_RESOURCE(
        gringofts_split_proto_library
        "protos/split.proto"
        ${gringofts_proto_generated_dir})

set(GRINGOFTS_APP_UTIL_SRC
        split/RequestCallData.cpp
        split/RequestReceiver.cpp
        split/SplitCommand.cpp
        split/SplitEvent.cpp
        split/SplitManager.cpp
        )

# Library
set(GRINGOFTS_APP_UTIL_LIBS ${Boost_LIBRARIES} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF} lib_inih)

add_library(gringofts_app_util STATIC ${GRINGOFTS_APP_UTIL_SRC})
target_link_libraries(gringofts_app_util gringofts_netadmin_proto_library gringofts_split_proto_library ${GRINGOFTS_APP_UTIL_LIBS})
