
# Generated sources

# proto

# Include generated *.pb.h files

# Source files
set(STRATEGY_SRC 
  CacheEviction.cpp)

# Libraries
add_library(kv_engine_strategy ${STRATEGY_SRC})
target_link_libraries(kv_engine_strategy kv_engine_store kv_engine_model kv_engine_execution)

# Executables

