find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS CoreAddons)

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
  Qt5::Core
  Qt5::DBus
  KF5::CoreAddons
)

####################################################################################################
# Library
####################################################################################################
add_library(disman-wllib SHARED ${wayland_SRCS})
add_library(disman::wllib ALIAS disman-wllib)

generate_export_header(disman-wllib
  BASE_NAME dismanwl
)

target_link_libraries(disman-wllib
  PRIVATE disman::backend
  PUBLIC ${wayland_LIBS}
)

set_target_properties(disman-wllib PROPERTIES
  OUTPUT_NAME dismanwl
  EXPORT_NAME wllib
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}"
)
target_compile_features(disman-wllib PRIVATE cxx_std_17)

install(
  TARGETS disman-wllib
  EXPORT dismanwl-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT disman
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dismanwl_export.h
    wayland_interface.h
    waylandoutput.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/disman/wayland
  COMPONENT disman
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dismanwl.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/dismanwl.pc @ONLY
)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/dismanwl.pc
  DESTINATION ${KDE_INSTALL_LIBDIR}/pkgconfig
  COMPONENT disman
)

ecm_setup_version(${PROJECT_VERSION}
  VARIABLE_PREFIX dismanwl
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/dismanwl_version.h"
  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/dismanwl-config-version.cmake"
)

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/dismanwl")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/dismanwl-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/dismanwl-config.cmake"
  INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/dismanwl-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/dismanwl-config-version.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT disman
)

install(
  EXPORT dismanwl-targets
  FILE dismanwl-targets.cmake
  NAMESPACE disman::
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT disman
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/dismanwl_version.h"
  DESTINATION "${INCLUDE_INSTALL_DIR}"
  COMPONENT disman
)

# Standard plugins
add_subdirectory(plugins)

####################################################################################################
# Wayland Plugin
####################################################################################################
add_library(disman-wlplugin MODULE ${wayland_SRCS})

set_target_properties(disman-wlplugin PROPERTIES
  OUTPUT_NAME wayland
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman/"
)
target_compile_features(disman-wlplugin PRIVATE cxx_std_17)

target_link_libraries(disman-wlplugin
  PRIVATE
    disman::backend
    ${wayland_LIBS}
)

install(
  TARGETS disman-wlplugin
  DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/
  COMPONENT disman
)
