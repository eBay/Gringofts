project(infra)

# Generated sources
set(proto_generated_dir "es/store/generated")
file(MAKE_DIRECTORY ${proto_generated_dir})

# store.proto
ADD_PROTO_RESOURCE(
        store_proto_library
        "es/store/store.proto"
        ${proto_generated_dir})

# sidecar.proto
set(sidecar_proto_generated_dir "sidecar/generated")
file(MAKE_DIRECTORY ${sidecar_proto_generated_dir})

ADD_PROTO_RESOURCE(
        sidecar_proto_library
        "sidecar/interop.proto"
        ${sidecar_proto_generated_dir})

# Include generated *.pb.h files
include_directories("${proto_generated_dir}")
include_directories("${sidecar_proto_generated_dir}")

add_subdirectory(raft)

# Source files
set(INFRA_SRC
        es/store/RaftCommandEventStore.cpp
        es/store/ReadonlyRaftCommandEventStore.cpp
        es/store/ReadonlySQLiteCommandEventStore.cpp
        es/store/SQLiteCommandEventStore.cpp
        es/store/SQLiteStoreDao.cpp)

set(MONITOR_SRC
        monitor/santiago/metrics_category.cpp
        monitor/santiago/metrics_collector.cpp
        monitor/santiago/metrics_monitor.cpp
        monitor/santiago/prometheus/PrometheusMetrics.cpp
        monitor/santiago/Metrics.cpp
        monitor/santiago/MetricsCenter.cpp
        monitor/MonitorCenter.cpp)

set(UTIL_SRC
        util/BigDecimal.cpp
        util/CryptoUtil.cpp
        util/IdGenerator.cpp)

# Libraries
add_library(infra_monitor STATIC
        ${MONITOR_SRC})
target_link_libraries(infra_monitor ${LIBRARIES})

add_library(infra_util STATIC
        ${UTIL_SRC})
add_dependencies(infra_util store_proto_library)
target_link_libraries(infra_util ${LIBRARIES})

add_library(infra STATIC
        ${INFRA_SRC})
add_dependencies(infra infra_raft store_proto_library)
target_link_libraries(infra infra_raft infra_monitor store_proto_library infra_util)

# Executables5
add_executable(RaftMain raft/RaftMain.cpp)
target_link_libraries(RaftMain infra)

add_executable(Sidecar sidecar/Sidecar.cpp)
target_link_libraries(Sidecar sidecar_proto_library infra)
