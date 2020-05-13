include_directories(${CMAKE_SOURCE_DIR}/src
                    ${CMAKE_BUILD_DIR}
)

set(xrandr_SRCS
    xrandr.cpp
    xrandrconfig.cpp
    xrandrcrtc.cpp
    xrandroutput.cpp
    xrandrmode.cpp
    xrandrscreen.cpp
    ../xcbwrapper.cpp
    ../xcbeventlistener.cpp
    ../utils.cpp
)

add_library(randr MODULE ${xrandr_SRCS})

set_target_properties(randr PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman")
set_target_properties(randr PROPERTIES PREFIX "")
target_link_libraries(randr
    Qt5::Core
    Qt5::Gui
    Qt5::X11Extras
    ${XCB_LIBRARIES}
    Disman
)

install(TARGETS randr DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/)
