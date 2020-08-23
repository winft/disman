find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS CoreAddons)

set(wayland_SRCS
  waylandbackend.cpp
  wayland_interface.cpp
  waylandoutput.cpp
  waylandscreen.cpp
  ../filer_controller.cpp
  ../logging.cpp
  ../utils.cpp
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
  Qt5::Core
  Qt5::DBus
  Disman
  KF5::CoreAddons
)

####################################################################################################
# Library
####################################################################################################
add_library(DismanWayland SHARED ${wayland_SRCS})
generate_export_header(DismanWayland BASE_NAME disman_wayland)

target_link_libraries(DismanWayland
  PUBLIC ${wayland_LIBS}
)

set_target_properties(DismanWayland PROPERTIES
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}"
  EXPORT_NAME DismanWayland
)

install(
  TARGETS DismanWayland
  EXPORT DismanWaylandTargets
  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/disman_wayland_export.h
    wayland_interface.h
    waylandoutput.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/disman/wayland
)

if(NOT WIN32)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/disman-wayland.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/disman-wayland.pc
    @ONLY
  )
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/disman-wayland.pc
    DESTINATION ${KDE_INSTALL_LIBDIR}/pkgconfig
  )
endif(NOT WIN32)

ecm_setup_version(${PROJECT_VERSION}
  VARIABLE_PREFIX DismanWayland
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/disman_wayland_version.h"
  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/DismanWaylandConfigVersion.cmake"
)

set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/DismanWayland")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/DismanWaylandConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/DismanWaylandConfig.cmake"
  INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/DismanWaylandConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/DismanWaylandConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(
  EXPORT DismanWaylandTargets
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  FILE DismanWaylandTargets.cmake
  COMPONENT Devel
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/disman_wayland_version.h"
  DESTINATION "${INCLUDE_INSTALL_DIR}"
  COMPONENT Devel
)

# Standard plugins
add_subdirectory(plugins)

####################################################################################################
# Wayland Plugin
####################################################################################################
add_library(wayland MODULE ${wayland_SRCS})

set_target_properties(wayland PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman")
set_target_properties(wayland PROPERTIES PREFIX "")
target_link_libraries(wayland ${wayland_LIBS})

install(
  TARGETS wayland
  DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/
)
