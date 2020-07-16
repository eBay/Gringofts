cmake_minimum_required(VERSION 3.12)
project(gringofts)
set(Gringofts_VERSION_MAJOR 1)
set(Gringofts_VERSION_MINOR 0)

MESSAGE(STATUS "compiler is: " ${CMAKE_CXX_COMPILER_ID})

## Unit test
if (${CMAKE_CXX_COMPILER_ID} MATCHES "^(Apple)?Clang$")
    include(cmake/clang-6/CodeCoverage.cmake)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    include(cmake/g++-7/CodeCoverage.cmake)
endif ()

set(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage ${CMAKE_CXX_FLAGS}")
## Unit test

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-overflow -fPIC")

# treat warnings as errors
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG -Werror")

# it is the macro NDEBUG that makes assert a no-op, just remove it to
# enable assert() under release mode.
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG -Werror")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Werror")

# static link libgcc and libstdc++
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

file(MAKE_DIRECTORY build)

cmake_policy(SET CMP0015 NEW)
link_directories(build)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)



if (${CMAKE_CXX_COMPILER_ID} MATCHES "^(Apple)?Clang$")

    ## clang will always use c++17 as std
    set(CPP_STD c++17)
    # special for MAC OS
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        include_directories(SYSTEM /usr/local/include)
        link_directories(/usr/local/lib)
        add_compile_definitions(MAC_OS)
        set(CMAKE_THREAD_LIBS_INIT "-lpthread")
        set(CMAKE_HAVE_THREADS_LIBRARY 1)
        set(CMAKE_USE_PTHREADS_INIT 1)
        set(THREADS_PREFER_PTHREAD_FLAG ON)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
        # mac os not support -static-libgcc -static-libstdc++
        unset(CMAKE_EXE_LINKER_FLAGS)
    endif()
    set(FLAG_LIST
            -Wno-c++98-compat-pedantic
            -Wno-covered-switch-default
            -Wno-deprecated
            -Wno-disabled-macro-expansion
            -Wno-documentation-unknown-command
            -Wno-exit-time-destructors
            -Wno-float-equal
            -Wno-global-constructors
            -Wno-gnu-zero-variadic-macro-arguments
            -Wno-missing-noreturn
            -Wno-missing-prototypes
            -Wno-missing-variable-declarations
            -Wno-packed
            -Wno-padded
            -Wno-reserved-id-macro
            -Wno-shadow
            -Wno-shift-sign-overflow
            -Wno-switch-enum
            -Wno-undef
            -Wno-unknown-warning-option
            -Wno-unused-macros
            -Wno-unused-member-function
            -Wno-unused-parameter
            -Wno-used-but-marked-unused
            -Wno-vla
            -Wno-vla-extension
            -Wno-weak-vtables
            -Weverything
            -Wno-unreachable-code ## clang 3.4 is knon to emit warnings without -Wno-unreachable-code, I will move this out when I have idea to get compiler version
            )
    foreach (FLAG in ${FLAG_LIST})
        set(CMAKE_TRINIDAD_CXX_FLAGS "${TRINIDAD_CXX_FLAGS} ${FLAG}")
    endforeach ()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

    ## detect gcc version to decide wich c std should we use
    execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -v
            OUTPUT_VARIABLE COMPILER_VERSION_OUTPUT
            ERROR_VARIABLE COMPILER_VERSION_ERROR
    )

    string(REGEX MATCH "gcc version ([0-9]+)\\.([0-9]+)\\.([0-9]+)"
            GCC_COMPILER_VERSION "${COMPILER_VERSION_ERROR} ${COMPILER_VERSION_OUTPUT}")
    STRING(REPLACE "gcc version " "" GCC_VERSION_NUMBER ${GCC_COMPILER_VERSION})
    if (${GCC_VERSION_NUMBER} VERSION_LESS "4.7.0")
        set(CPP_STD c++0x)
    else ()
        set(CPP_STD c++17)
    endif ()

    ## gcc std setting end

    set(FLAG_LIST
            -Wall
            -Wextra
            -Wcast-align
            -Wcast-qual
            -Wconversion
            -Weffc++
            -Wformat=2
            -Wmissing-format-attribute
            -Wno-non-template-friend
            -Wno-unused-parameter
            -Woverloaded-virtual
            -Wwrite-strings
            )
    foreach (FLAG ${FLAG_LIST})
        set(CMAKE_TRINIDAD_CXX_FLAGS "${TRINIDAD_CXX_FLAGS} ${FLAG}")
    endforeach ()
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=${CPP_STD}")

## gRPC & protobuf (cherry-picked from https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/CMakeLists.txt)
if (GRPC_AS_SUBMODULE)
    # One way to build a projects that uses gRPC is to just include the
    # entire gRPC project tree via "add_subdirectory".
    # This approach is very simple to use, but the are some potential
    # disadvantages:
    # * it includes gRPC's CMakeLists.txt directly into your build script
    #   without and that can make gRPC's internal setting interfere with your
    #   own build.
    # * depending on what's installed on your system, the contents of submodules
    #   in gRPC's third_party/* might need to be available (and there might be
    #   additional prerequisites required to build them). Consider using
    #   the gRPC_*_PROVIDER options to fine-tune the expected behavior.
    #
    # A more robust approach to add dependency on gRPC is using
    # cmake's ExternalProject_Add (see cmake_externalproject/CMakeLists.txt).

    # Include the gRPC's cmake build (normally grpc source code would live
    # in a git submodule called "third_party/grpc", but this example lives in
    # the same repository as gRPC sources, so we just look a few directories up)
    add_subdirectory(../../.. ${CMAKE_CURRENT_BINARY_DIR}/grpc EXCLUDE_FROM_ALL)
    message(STATUS "Using gRPC via add_subdirectory.")

    # After using add_subdirectory, we can now use the grpc targets directly from
    # this build.
    set(_PROTOBUF_LIBPROTOBUF libprotobuf)
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
    set(_GRPC_GRPCPP grpc++)
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
else ()
    # This branch assumes that gRPC and all its dependencies are already installed
    # on this system, so they can be located by find_package().

    # Find Protobuf installation
    # Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
    set(protobuf_MODULE_COMPATIBLE TRUE)
    find_package(Protobuf CONFIG REQUIRED)
    message(STATUS "Using protobuf ${Protobuf_VERSION}")

    set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

    # Find gRPC installation
    # Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
    find_package(gRPC CONFIG REQUIRED)
    message(STATUS "Using gRPC ${gRPC_VERSION}")

    set(_GRPC_GRPCPP gRPC::grpc++)
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif ()

# log
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSPDLOG_NO_EXCEPTIONS=ON")
include_directories(third_party/spdlog/include)

# ini parser
include_directories(third_party/inih/cpp)

# abseil-cpp
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(third_party/abseil-cpp EXCLUDE_FROM_ALL)
include_directories(third_party/abseil-cpp)
set(ABSL_LIBS absl::strings absl::base)

# prometheus
add_subdirectory(third_party/prometheus-cpp EXCLUDE_FROM_ALL)
include_directories(third_party/prometheus-cpp/core/include)
include_directories(third_party/prometheus-cpp/pull/include)
set(MONITOR_LIBS prometheus-cpp::pull prometheus-cpp::core)

# boost
find_package(Boost COMPONENTS system filesystem thread iostreams REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

# openssl
find_package(OpenSSL COMPONENTS Crypto SSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
# TODO Fix version of OpenSSL::Crypto
set(OPENSSL_LIBS
        OpenSSL::SSL
        OpenSSL::Crypto)

set(ROCKSDB_LIBS
        rocksdb.a
        gflags.a
        snappy.a
        bz2.a
        lz4.a
        zstd.a)

set(INIH_SRC
        third_party/inih/ini.c
        third_party/inih/cpp/INIReader.cpp)

add_library(lib_inih STATIC
        ${INIH_SRC})

# check library exists
function(check_lib_exist LIB_NAME)
    set(PATH_VAR_NAME "${LIB_NAME}_PATH")
    find_library(${PATH_VAR_NAME} NAMES ${LIB_NAME})
    if ("${${PATH_VAR_NAME}}" STREQUAL "${PATH_VAR_NAME}-NOTFOUND")
        ## sending warrning allows user to compile the lib without unnecessary dependency
        message(WARNING "can not find library ${LIB_NAME}")
    else ()
        message(STATUS "found library ${LIB_NAME} at ${${PATH_VAR_NAME}}")
    endif ()
endfunction(check_lib_exist)

check_lib_exist(pthread)
check_lib_exist(sqlite3)


# collect all libs when they all exist
set(GRINGOFTS_LIBRARIES pthread sqlite3.a dl ${ABSL_LIBS} ${OPENSSL_LIBS} ${Boost_LIBRARIES}
        ${MONITOR_LIBS} ${ROCKSDB_LIBS} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF} lib_inih)

# Protobuf build by cmake
include(cmake/protobuf/ProtoLibBuild.cmake)

add_subdirectory(src/infra)
add_subdirectory(src/app_util)
add_subdirectory(src/app_demo)

add_subdirectory(test)

# expose to parent
get_directory_property(hasParent PARENT_DIRECTORY)
if(hasParent)
    message(STATUS "Gringofts as a library")
    set(GRINGOFTS_INCL
            Gringofts/src
            Gringofts/test
            Gringofts/third_party/abseil-cpp
            Gringofts/third_party/prometheus-cpp/core/include
            Gringofts/third_party/prometheus-cpp/pull/include
            Gringofts/third_party/spdlog/include
            Gringofts/third_party/inih/cpp PARENT_SCOPE)
    set(GRINGOFTS_LIB ${GRINGOFTS_LIBRARIES} PARENT_SCOPE)
endif()


set(ALL_SRC
        src
        test)

# custom targets
add_custom_target(gringofts_check
        COMMAND third_party/cpplint/cpplint.py --recursive ${ALL_SRC} 2>&1 > /var/tmp/cpplint.out)
add_custom_target(gringofts_docs
        COMMAND doxygen docs/Doxyfile)
