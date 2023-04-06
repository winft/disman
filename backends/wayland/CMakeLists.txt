set(wayland_SRCS
  waylandbackend.cpp
  wayland_interface.cpp
  waylandoutput.cpp
  waylandscreen.cpp
)

ecm_qt_declare_logging_category(wayland_SRCS
  HEADER wayland_logging.h
  IDENTIFIER DISMAN_WAYLAND
  CATEGORY_NAME disman.backend.wayland
)

qt5_add_dbus_interface(wayland_SRCS
  org.kde.KWin.TabletModeManager.xml
  tabletmodemanager_interface
)

set(wayland_LIBS
  disman::lib
  KF5::CoreAddons
)

####################################################################################################
# Wayland Plugin
####################################################################################################
add_library(disman-wayland MODULE ${wayland_SRCS})

set_target_properties(disman-wayland PROPERTIES
  OUTPUT_NAME wayland
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman/"
)
target_compile_features(disman-wayland PRIVATE cxx_std_20)

target_link_libraries(disman-wayland
  PRIVATE
    disman::backend
    ${wayland_LIBS}
    WraplandClient
)

install(
  TARGETS disman-wayland
  DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/
  COMPONENT disman
)
