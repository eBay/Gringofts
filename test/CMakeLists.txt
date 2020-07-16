project(test)
MESSAGE(STATUS "gringofts test")

# Sources
set(UNIT_TEST_SRC
        TestRunner.cc
        infra/es/CommandEventStoreTest.cpp
        infra/es/CommandMetaDataTest.cpp
        infra/es/CommandTest.cpp
        infra/es/EventTest.cpp
        infra/es/LoopTest.cpp
        infra/es/ReadonlyCommandEventStoreTest.cpp
        infra/es/store/DefaultCommandEventStoreTest.cpp
        infra/es/store/RaftCommandEventStoreTest.cpp
        infra/es/store/SnapshotUtilTest.cpp
        infra/es/store/SQLiteCommandEventStoreTest.cpp
        infra/grpc/RequestHandleTest.cpp
        infra/monitor/MonitorCenterTest.cpp
        infra/monitor/santiago/MetricsCenterTest.cpp
        infra/mpscqueue/MpscDoubleBufferQueueTest.cpp
        infra/raft/monitor/RaftMonitorAdaptorTest.cpp
        infra/raft/storage/LogTest.cpp
        infra/raft/v2/RaftCoreTest.cpp
        infra/util/BigDecimalTest.cpp
        infra/util/CryptoUtilTest.cpp
        infra/util/FileUtilTest.cpp
        infra/util/IdGeneratorTest.cpp
        infra/util/RandomUtilTest.cpp
        infra/util/TimeUtilTest.cpp
        infra/util/TlsUtilTest.cpp
        infra/util/UtilTest.cpp)

# unit-test on googletest
include_directories(BEFORE ../third_party/gtest/googletest)
include_directories(BEFORE ../third_party/gtest/googletest/include)
# add gmock
include_directories(BEFORE ../third_party/gtest/googlemock)
include_directories(BEFORE ../third_party/gtest/googlemock/include)

# AppStateMachineTest->AppStateMachine->snapshot(protobuf)->store(protobuf)
include_directories(BEFORE ../src/infra/es/store/generated)

# Executable
add_executable(gringofts_TestRunner
        ${UNIT_TEST_SRC}
        ../third_party/gtest/googlemock/src/gmock-all.cc
        ../third_party/gtest/googletest/src/gtest-all.cc)
target_link_libraries(gringofts_TestRunner gringofts_infra ${GRINGOFTS_LIBRARIES})

# add coverage target here otherwise the app source code will not be covered
set(COVERAGE_LCOV_EXCLUDES '/usr/include/*' '/usr/local/include/*' '*/BullseyeCoverage-8.16.4/include/*' 'gtest/*' 'test/*' 'build/*' '*/*Test*'
        'third_party/*' 'CMakeFiles/*' 'src/app_demo/*' 'src/app_util/*'
        'src/infra/es/store/generated/*' 'src/infra/raft/generated/*' '*/*Main.cpp')
setup_target_for_coverage_lcov(
        NAME gringofts_test_coverage_lcov
        EXECUTABLE gringofts_TestRunner --gtest_output=xml:/var/tmp/tests/unitTestReport.xml --gtest_color=yes
        DEPENDENCIES gringofts_TestRunner)

set(COVERAGE_GCOVR_EXCLUDES '/usr/include/*','/usr/local/include/*','*/BullseyeCoverage-8.16.4/include/*','gtest/*','test/*','build/*','*/*Test*',
        'third_party/*','CMakeFiles/*','src/app_demo/*','src/app_util/*',
        'src/infra/es/store/generated/*','src/infra/raft/generated/*','*/*Main.cpp')
setup_target_for_coverage_gcovr_xml(
        NAME gringofts_test_coverage_cobertura
        EXECUTABLE gringofts_TestRunner --gtest_output=xml:/var/tmp/tests/unitTestReport.xml --gtest_color=yes
        DEPENDENCIES gringofts_TestRunner)