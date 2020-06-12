cmake_minimum_required(VERSION 3.12)
project(infra_raft)
MESSAGE(STATUS "processing infra_raft")

# Generated sources
set(raft_generated_dir "generated")
file(MAKE_DIRECTORY ${raft_generated_dir})

# raft.proto and streaming.proto
set(proto_file_list raft.proto streaming.proto)
ADD_PROTO_SET(raft_proto_library "${proto_file_list}" ${raft_generated_dir})

# Include generated *.pb.h files
include_directories("${raft_generated_dir}")

# Source files
set(INFRA_RAFT_SRC
        metrics/RaftMonitorAdaptor.cpp
        RaftLogStore.cpp
        RaftReplyLoop.cpp
        StreamingService.cpp
        storage/Segment.cpp
        storage/SegmentLog.cpp
        v2/RaftCore.cpp
        v2/RaftService.cpp)

# Library
add_library(infra_raft STATIC
        ${INFRA_RAFT_SRC})
add_dependencies(infra_raft raft_proto_library)
target_link_libraries(infra_raft raft_proto_library)

# Executables
add_executable(StorageMain StorageMain.cpp)
target_link_libraries(StorageMain infra_raft raft_proto_library infra_util ${LIBRARIES})
