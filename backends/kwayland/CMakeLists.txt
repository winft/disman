set(wayland_SRCS
    waylandbackend.cpp
    waylandconfig.cpp
    waylandoutput.cpp
    waylandscreen.cpp
    ../utils.cpp
)
qt5_add_dbus_interface(wayland_SRCS org.kde.KWin.TabletModeManager.xml tabletmodemanager_interface)

add_library(wayland MODULE ${wayland_SRCS})

set_target_properties(wayland PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman")
set_target_properties(wayland PROPERTIES PREFIX "")
target_link_libraries(wayland
    Qt5::Core
    Qt5::DBus
    Qt5::Gui
    Disman
    KF5::WaylandClient
)

install(TARGETS wayland DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/)
