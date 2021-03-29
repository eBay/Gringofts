project(KVEngine)

# Generated sources

# proto

# Include generated *.pb.h files

add_subdirectory(utils)
add_subdirectory(store)
add_subdirectory(raft)
add_subdirectory(model)
add_subdirectory(execution)
add_subdirectory(strategy)

# Source files
set(KV_ENGINE_SRC
      KVEngineImpl.cpp
      KVEngine.cpp)

# Libraries
add_library(kv_engine STATIC
      ${KV_ENGINE_SRC})

#add_dependencies(kv_engine)
target_link_libraries(kv_engine
        gringofts_infra
        kv_engine_model
        kv_engine_raft
        kv_engine_execution
        kv_engine_strategy)

