cmake_minimum_required(VERSION 3.12)
project(app_util)
MESSAGE(STATUS "processing app_util")

# Generated sources for app_demo
set(gringofts_proto_generated_dir "generated/grpc")
file(MAKE_DIRECTORY ${gringofts_proto_generated_dir})

# scale.proto
ADD_PROTO_RESOURCE(
        gringofts_scale_proto_library
        "protos/scale.proto"
        ${gringofts_proto_generated_dir})

# route.proto
ADD_PROTO_RESOURCE(
        gringofts_route_proto_library
        "protos/route.proto"
        ${gringofts_proto_generated_dir})

# netadmin.proto
ADD_PROTO_RESOURCE(
        gringofts_netadmin_proto_library
        "protos/netadmin.proto"
        ${gringofts_proto_generated_dir})

add_dependencies(gringofts_netadmin_proto_library gringofts_route_proto_library)
add_dependencies(gringofts_scale_proto_library gringofts_route_proto_library)

# Source files
set(GRINGOFTS_APP_UTIL_SRC
        AppInfo.cpp
        control/split/ScaleReceiver.cpp
        control/split/SplitCommand.cpp
        control/split/SplitEvent.cpp
        control/CtrlState.cpp
        control/Route.cpp
        sync/LogReader.cpp
        sync/LogSyncService.cpp)

# Library
set(GRINGOFTS_APP_UTIL_LIBS ${Boost_LIBRARIES} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF} lib_inih)

add_library(gringofts_app_util STATIC ${GRINGOFTS_APP_UTIL_SRC})
target_link_libraries(gringofts_app_util gringofts_netadmin_proto_library gringofts_scale_proto_library
        gringofts_route_proto_library ${GRINGOFTS_APP_UTIL_LIBS})
