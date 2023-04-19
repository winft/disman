add_definitions(-DTEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/configs/")

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/tests/wayland/)

macro(DISMAN_ADD_TEST)
    foreach(_testfile ${ARGN})
        set(_testname test_${_testfile})
        set(test_SRCS ${_testfile}.cpp ${DISMAN_WAYLAND_SRCS})
        qt6_add_dbus_interface(test_SRCS ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.fakebackend.xml fakebackendinterface)
        add_executable(${_testname} ${test_SRCS})
        target_compile_features(${_testname} PRIVATE cxx_std_17)
        target_link_libraries(${_testname}
          disman::backend
          Qt6::Test
          Qt6::DBus
          ${DISMAN_WAYLAND_LIBS}
        )
        add_test(NAME disman-${_testname}
                 COMMAND dbus-launch $<TARGET_FILE:${_testname}>
        )
        ecm_mark_as_test(${_testname})
    endforeach(_testfile)
endmacro(DISMAN_ADD_TEST)

macro(DISMAN_ADD_TEST2)
    foreach(_testname ${ARGN})
        set(test_SRCS ${_testname}.cpp ${DISMAN_WAYLAND_SRCS})
        qt6_add_dbus_interface(test_SRCS
            ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.fakebackend.xml fakebackendinterface)
        add_executable(test-${_testname} ${test_SRCS})
        target_compile_features(test-${_testname} PRIVATE cxx_std_17)
        target_link_libraries(test-${_testname}
          disman::backend
          Qt6::Test
          Qt6::DBus
          ${DISMAN_WAYLAND_LIBS}
        )
        add_test(NAME disman-test-${_testname}
                 COMMAND dbus-launch $<TARGET_FILE:test-${_testname}>
        )
        ecm_mark_as_test(test-${_testname})
    endforeach(_testname)
endmacro(DISMAN_ADD_TEST2)

disman_add_test2(config)
disman_add_test2(generator)
disman_add_test(testscreenconfig)
disman_add_test(testqscreenbackend)
disman_add_test(testconfigserializer)
disman_add_test(testconfigmonitor)
disman_add_test(testinprocess)
disman_add_test(testbackendloader)
disman_add_test(testlog)
disman_add_test(testmodelistchange)
disman_add_test(testedid)

set(DISMAN_WAYLAND_LIBS
    WraplandServer
    WraplandClient
)

# For WaylandConfigReader and TestServer
set(DISMAN_WAYLAND_SRCS
    ${CMAKE_SOURCE_DIR}/tests/wayland/config_reader.cpp
    ${CMAKE_SOURCE_DIR}/tests/wayland/server.cpp
)

disman_add_test(wayland_backend)
disman_add_test(wayland_config)
disman_add_test(wayland_dpms)

set(DISMAN_WAYLAND_LIBS "")
set(DISMAN_WAYLAND_SRCS "")


if (ENABLE_XRANDR_TESTS)
    disman_add_test(textxrandr)
endif()
