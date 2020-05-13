find_package(XCB REQUIRED COMPONENTS XCB RANDR)

include_directories(${CMAKE_SOURCE_DIR}/src
                    ${KDE4_INCLUDES}
                    ${CMAKE_BUILD_DIR}
                    ${QT_INCLUDES}
)

set(xrandr11_SRCS
    xrandr11.cpp
    ../xcbeventlistener.cpp
    ../xcbwrapper.cpp
)

add_library(randr11 MODULE ${xrandr11_SRCS})

set_target_properties(randr11 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman")
set_target_properties(randr11 PROPERTIES PREFIX "")
target_link_libraries(randr11
    Qt5::Core
    Qt5::Gui
    Qt5::X11Extras
    ${XCB_LIBRARIES}
    Disman
)

install(TARGETS randr11 DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/)
