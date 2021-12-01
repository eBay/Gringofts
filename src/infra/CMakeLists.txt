project(infra)

# Generated sources
set(gringofts_proto_generated_dir "es/store/generated")
file(MAKE_DIRECTORY ${gringofts_proto_generated_dir})

# store.proto
ADD_PROTO_RESOURCE(
        gringofts_store_proto_library
        "es/store/store.proto"
        ${gringofts_proto_generated_dir})

# Include generated *.pb.h files
include_directories("${gringofts_proto_generated_dir}")

add_subdirectory(raft)

# Source files
set(GRINGOFTS_INFRA_SRC
        es/store/RaftCommandEventStore.cpp
        es/store/ReadonlyRaftCommandEventStore.cpp
        es/store/ReadonlySQLiteCommandEventStore.cpp
        es/store/SQLiteCommandEventStore.cpp
        es/store/SQLiteStoreDao.cpp)

set(GRINGOFTS_MONITOR_SRC
        monitor/santiago/prometheus/PrometheusMetrics.cpp
        monitor/santiago/Metrics.cpp
        monitor/santiago/MetricsCenter.cpp
        monitor/santiago/Server.cpp
        monitor/MonitorCenter.cpp)


set(GRINGOFTS_UTIL_SRC
        util/BigDecimal.cpp
        util/ClusterInfo.cpp
        util/CryptoUtil.cpp
        util/FileUtil.cpp
        util/TrackingMemoryResource.cpp
        util/IdGenerator.cpp
        util/KVClient.cpp
        util/MemoryPool.cpp
        util/PerfConfig.cpp
        util/IdGenerator.cpp
        util/Signal.cpp
        util/TlsUtil.cpp)

# Libraries
add_library(gringofts_infra_monitor STATIC
        ${GRINGOFTS_MONITOR_SRC})
# Prometheus
target_link_libraries(gringofts_infra_monitor ${GRINGOFTS_LIBRARIES})

add_library(gringofts_infra_util STATIC
        ${GRINGOFTS_UTIL_SRC})
add_dependencies(gringofts_infra_util gringofts_store_proto_library)
target_link_libraries(gringofts_infra_util gringofts_store_proto_library ${GRINGOFTS_LIBRARIES})

add_library(gringofts_infra STATIC
        ${GRINGOFTS_INFRA_SRC})
add_dependencies(gringofts_infra gringofts_infra_raft gringofts_store_proto_library)
target_link_libraries(gringofts_infra gringofts_infra_raft gringofts_store_proto_library gringofts_infra_util gringofts_infra_monitor)

# Executables
add_executable(RaftMain raft/RaftMain.cpp)
target_link_libraries(RaftMain gringofts_infra)
