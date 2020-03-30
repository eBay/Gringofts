# Install golang
set(GOPATH "${PROJECT_SOURCE_DIR}/third_party/go")

function(ADD_GO_INSTALLABLE_PROGRAM NAME MAIN_SRC DST_DIR)
    get_filename_component(MAIN_SRC_ABS ${MAIN_SRC} ABSOLUTE)
    add_custom_target(${NAME})
    # go-build
    add_custom_command(TARGET ${NAME}
      COMMAND env GOPATH=${GOPATH} env GOPROXY=https://goproxy.io go build
            -o "${DST_DIR}/${NAME}"
            ${CMAKE_GO_FLAGS} ${MAIN_SRC}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            DEPENDS ${MAIN_SRC_ABS})

    # go-build .so
    add_custom_command(TARGET ${NAME}
            COMMAND env GOPATH=${GOPATH} go build
            -o "${DST_DIR}/${NAME}.so"
            -buildmode "c-shared"
            ${CMAKE_GO_FLAGS} ${MAIN_SRC}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            DEPENDS ${MAIN_SRC_ABS}
            )

    foreach(DEP ${ARGN})
        add_dependencies(${NAME} ${DEP})
    endforeach()
endfunction(ADD_GO_INSTALLABLE_PROGRAM)

function(GO_TEST NAME MAIN_SRC)
    get_filename_component(MAIN_SRC_ABS ${MAIN_SRC} ABSOLUTE)
    add_custom_target(${NAME})
    # go-test
    add_custom_command(TARGET ${NAME}
      COMMAND env GOPATH=${GOPATH} env GOPROXY=https://goproxy.io go test -v
            ${MAIN_SRC}/...
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            DEPENDS ${MAIN_SRC_ABS}
            )
endfunction(GO_TEST)
