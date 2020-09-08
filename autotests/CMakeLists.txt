add_definitions(-DTEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/configs/")

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/tests/kwayland/)

macro(DISMAN_ADD_TEST)
    foreach(_testname ${ARGN})
        set(test_SRCS ${_testname}.cpp ${DISMAN_WAYLAND_SRCS})
        qt5_add_dbus_interface(test_SRCS ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.fakebackend.xml fakebackendinterface)
        add_executable(${_testname} ${test_SRCS})
        target_compile_features(${_testname} PRIVATE cxx_std_17)
        target_link_libraries(${_testname}
          Qt5::Core
          Qt5::Gui
          Qt5::Test
          Qt5::DBus
          Disman::Disman
          Disman::Backend
          ${DISMAN_WAYLAND_LIBS}
        )
        add_test(NAME disman-${_testname}
                 COMMAND dbus-launch $<TARGET_FILE:${_testname}>
        )
        ecm_mark_as_test(${_testname})
    endforeach(_testname)
endmacro(DISMAN_ADD_TEST)

macro(DISMAN_ADD_TEST2)
    foreach(_testname ${ARGN})
        set(test_SRCS ${_testname}.cpp ${DISMAN_WAYLAND_SRCS})
        qt5_add_dbus_interface(test_SRCS
            ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.fakebackend.xml fakebackendinterface)
        add_executable(test-${_testname} ${test_SRCS})
        target_compile_features(test-${_testname} PRIVATE cxx_std_17)
        target_link_libraries(test-${_testname}
          Qt5::Core
          Qt5::Gui
          Qt5::Test
          Qt5::DBus
          Disman::Disman
          Disman::Backend
          ${DISMAN_WAYLAND_LIBS}
        )
        add_test(NAME disman-test-${_testname}
                 COMMAND dbus-launch $<TARGET_FILE:test-${_testname}>
        )
        ecm_mark_as_test(test-${_testname})
    endforeach(_testname)
endmacro(DISMAN_ADD_TEST2)

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
    KF5::WaylandServer KF5::WaylandClient
)

# For WaylandConfigReader and TestServer
set(DISMAN_WAYLAND_SRCS
    ${CMAKE_SOURCE_DIR}/tests/kwayland/waylandconfigreader.cpp
    ${CMAKE_SOURCE_DIR}/tests/kwayland/waylandtestserver.cpp
)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../backends/kwayland)

disman_add_test(testkwaylandbackend)
disman_add_test(testkwaylandconfig)
disman_add_test(testkwaylanddpms)

set(DISMAN_WAYLAND_LIBS "")
set(DISMAN_WAYLAND_SRCS "")


if (ENABLE_XRANDR_TESTS)
    disman_add_test(textxrandr)
endif()
