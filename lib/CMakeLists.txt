set(disman_SRCS
  backend.cpp
  backendmanager.cpp
  config.cpp
  configoperation.cpp
  getconfigoperation.cpp
  setconfigoperation.cpp
  configmonitor.cpp
  configserializer.cpp
  generator.cpp
  screen.cpp
  output.cpp
  mode.cpp
  log.cpp
)

qt6_add_dbus_interface(
  disman_SRCS ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.backend.xml backendinterface
)

ecm_qt_declare_logging_category(disman_SRCS
  HEADER disman_debug.h
  IDENTIFIER DISMAN
  CATEGORY_NAME disman
)

add_library(disman-lib SHARED ${disman_SRCS})
add_library(disman::lib ALIAS disman-lib)

generate_export_header(disman-lib
  BASE_NAME disman
)
target_compile_features(disman-lib
    PUBLIC cxx_std_11
    PRIVATE cxx_std_17
)

target_link_libraries(disman-lib
  PUBLIC
    Qt6::Core
    Qt6::Gui
  PRIVATE
    Qt6::DBus
)

set_target_properties(disman-lib PROPERTIES
  OUTPUT_NAME disman
  EXPORT_NAME lib
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}"
)

target_include_directories(disman-lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(
  TARGETS disman-lib
  EXPORT disman-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT disman
)

set(disman_HEADERS
  ${CMAKE_CURRENT_BINARY_DIR}/disman_export.h
  backendmanager_p.h # needed for Disman unit tests
  config.h
  configmonitor.h
  configoperation.h
  generator.h
  getconfigoperation.h
  log.h
  mode.h
  output.h
  screen.h
  setconfigoperation.h
  types.h
)

install(
  FILES ${disman_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/disman
  COMPONENT disman
)

ecm_setup_version(${PROJECT_VERSION}
  VARIABLE_PREFIX Disman
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/disman_version.h"
  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/disman-config-version.cmake"
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/disman.pc.in ${CMAKE_CURRENT_BINARY_DIR}/disman.pc @ONLY)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/disman.pc
  DESTINATION ${KDE_INSTALL_LIBDIR}/pkgconfig
  COMPONENT disman
)

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/disman")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/disman-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/disman-config.cmake" INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/disman-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/disman-config-version.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT disman
)

install(
  EXPORT disman-targets
  FILE disman-targets.cmake
  NAMESPACE disman::
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT disman
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/disman_version.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT disman
)
