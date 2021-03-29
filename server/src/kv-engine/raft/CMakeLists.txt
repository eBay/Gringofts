
# Generated sources

# proto

# Include generated *.pb.h files

# Source files
set(STORAGE_SRC
  RaftEventStore.cpp
  ReplyLoop.cpp
  )

# Libraries
add_library(kv_engine_raft ${STORAGE_SRC})
target_link_libraries(kv_engine_raft gringofts_infra kv_engine_model kv_engine_store)

# Executables

