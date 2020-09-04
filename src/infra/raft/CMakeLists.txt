cmake_minimum_required(VERSION 3.12)
project(infra_raft)
MESSAGE(STATUS "processing infra_raft")

# Generated sources
set(gringofts_raft_generated_dir "generated")
file(MAKE_DIRECTORY ${gringofts_raft_generated_dir})

# raft.proto and streaming.proto
set(gringofts_proto_file_list raft.proto streaming.proto)
ADD_PROTO_SET(gringofts_raft_proto_library "${gringofts_proto_file_list}" ${gringofts_raft_generated_dir})

# Include generated *.pb.h files
include_directories("${gringofts_raft_generated_dir}")

# Source files
set(GRINGOFTS_INFRA_RAFT_SRC
        metrics/RaftMonitorAdaptor.cpp
        RaftLogStore.cpp
        RaftReplyLoop.cpp
        StreamingService.cpp
        storage/Segment.cpp
        storage/SegmentLog.cpp
        v2/RaftCore.cpp
        v2/RaftService.cpp)

# Library
add_library(gringofts_infra_raft STATIC
        ${GRINGOFTS_INFRA_RAFT_SRC})
add_dependencies(gringofts_infra_raft gringofts_raft_proto_library)
target_link_libraries(gringofts_infra_raft gringofts_raft_proto_library)

# Executables
add_executable(StorageMain StorageMain.cpp)
target_link_libraries(StorageMain gringofts_infra_raft gringofts_raft_proto_library gringofts_infra_util ${GRINGOFTS_LIBRARIES})
