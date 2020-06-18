project(app_demo)
MESSAGE(STATUS "processing app_demo")

#################################################################################################
#
# BEGIN src for app_demo
#################################################################################################

# Generated sources for demo app
set(proto_generated_dir "generated/grpc")
file(MAKE_DIRECTORY ${proto_generated_dir})

set(proto_file_list
        should_be_generated/domain/protos/publishEvents.proto
        should_be_generated/domain/protos/demo.proto)

ADD_PROTO_SET(demo_proto_library "${proto_file_list}" ${proto_generated_dir})

## src list
# app_demo
set(APP_DEMO_AUTOGEN
        should_be_generated/app/App.cpp
        should_be_generated/app/RequestCallData.cpp
        should_be_generated/app/RequestReceiver.cpp
        should_be_generated/domain/AppStateMachine_internal.cpp
        should_be_generated/domain/CommandDecoderImpl.cpp
        should_be_generated/domain/EventDecoderImpl.cpp
        should_be_generated/domain/IncreaseCommand.cpp
        should_be_generated/domain/ProcessedEvent.cpp
        should_be_generated/domain/post/BundleExposePublisher.cpp)

set(APP_DEMO_SRC
        v1/AppStateMachine.cpp
        v2/RocksDBBackedAppStateMachine.cpp
        execution/IncreaseApplier.cpp
        execution/IncreaseHandler.cpp)

#################################################################################################
#
# END src for app_demo
#################################################################################################


#################################################################################################
#
# BEGIN lib & executables for app_demo
#################################################################################################

add_library(app_demo STATIC
        ${APP_DEMO_AUTOGEN}
        ${APP_DEMO_SRC})

add_dependencies(app_demo demo_proto_library)
target_link_libraries(app_demo demo_proto_library)

# executables
add_executable(DemoApp
        should_be_generated/app/Main.cpp)
target_link_libraries(DemoApp app_demo gringofts_app_util gringofts_infra ${GRINGOFTS_LIBRARIES})

#################################################################################################
#
# END lib & executables for app_demo
#################################################################################################
