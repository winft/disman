include_directories(${CMAKE_SOURCE_DIR}/src
                    ${CMAKE_BUILD_DIR}
)

set(qscreen_SRCS
    qscreenbackend.cpp
    qscreenconfig.cpp
    qscreenscreen.cpp
    qscreenoutput.cpp
)

add_library(qscreen MODULE ${qscreen_SRCS})

set_target_properties(qscreen PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman")
set_target_properties(qscreen PROPERTIES PREFIX "")
target_link_libraries(qscreen
    Qt5::Core
    Qt5::Gui
    Qt5::X11Extras
    Disman
)

install(TARGETS qscreen DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/)
