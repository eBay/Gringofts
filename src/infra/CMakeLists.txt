project(infra)

# Generated sources
set(proto_generated_dir "es/store/generated")
file(MAKE_DIRECTORY ${proto_generated_dir})

# store.proto
ADD_PROTO_RESOURCE(
        store_proto_library
        "es/store/store.proto"
        ${proto_generated_dir})

# Include generated *.pb.h files
include_directories("${proto_generated_dir}")

add_subdirectory(raft)

# Source files
set(INFRA_SRC
        es/store/RaftCommandEventStore.cpp
        es/store/ReadonlyRaftCommandEventStore.cpp
        es/store/ReadonlySQLiteCommandEventStore.cpp
        es/store/SQLiteCommandEventStore.cpp
        es/store/SQLiteStoreDao.cpp)


set(UTIL_SRC
        util/BigDecimal.cpp
        util/CryptoUtil.cpp
        util/IdGenerator.cpp)

# Libraries
add_library(infra_util STATIC
        ${UTIL_SRC})
add_dependencies(infra_util store_proto_library)
target_link_libraries(infra_util ${LIBRARIES})

add_library(infra STATIC
        ${INFRA_SRC})
add_dependencies(infra infra_raft store_proto_library)
target_link_libraries(infra infra_raft store_proto_library infra_util)

# Executables
add_executable(RaftMain raft/RaftMain.cpp)
target_link_libraries(RaftMain infra)
