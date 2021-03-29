
# Generated sources
set(proto_generated_dir "../../../../protocols/generated")
file(MAKE_DIRECTORY ${proto_generated_dir})

# proto
set(proto_file_list
        ../../../../protocols/common.proto
        ../../../../protocols/control.proto
        ../../../../protocols/transfer.proto
        ../../../../protocols/userdefine.proto
        ../../../../protocols/service.proto
        ../../../../protocols/model.proto)
ADD_PROTO_SET(kv_engine_model_proto_library "${proto_file_list}" ${proto_generated_dir})

# Include generated *.pb.h files

# Source files
set(MODEL_SRC Event.cpp
        ConnectCommand.cpp
        KVPutCommand.cpp
        KVGetCommand.cpp
        KVDeleteCommand.cpp
        KVEvictCommand.cpp
        KVTransCommand.cpp
        KVGenerateCommand.cpp
        KVImportCommand.cpp)

# Libraries
add_library(kv_engine_model ${MODEL_SRC})
target_link_libraries(kv_engine_model kv_engine_store kv_engine_model_proto_library)

# Executables

