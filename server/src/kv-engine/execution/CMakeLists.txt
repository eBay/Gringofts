
# Generated sources

# proto

# Include generated *.pb.h files

# Source files
set(EXECUTION_SRC
        EventApplyLoop.cpp
        Loop.cpp
        CommandProcessor.cpp)


# Libraries
add_library(kv_engine_execution ${EXECUTION_SRC})
target_link_libraries(kv_engine_execution kv_engine_store kv_engine_model gringofts_app_util)

# Executables

