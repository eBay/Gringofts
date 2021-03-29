
# Generated sources
set(proto_generated_dir "../../../../protocols/generated")
file(MAKE_DIRECTORY ${proto_generated_dir})

# proto
set(proto_file_list
        ../../../../protocols/storage.proto
        ../../../../protocols/oldstorage.proto)
ADD_PROTO_SET(store_proto_library "${proto_file_list}" ${proto_generated_dir})

# Include generated *.pb.h files

# Source files
set(STORE_SRC 
  RocksDBKVStore.cpp
  InMemoryKVStore.cpp
  ConcurInMemoryKVStore.cpp
  WSConcurInMemoryKVStore.cpp
  TemporalKVStore.cpp
  VersionStore.cpp)

# Libraries
add_library(kv_engine_store ${STORE_SRC})
target_link_libraries(kv_engine_store store_proto_library ${GRINGOFTS_LIB})

# Executables

