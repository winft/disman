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

qt5_add_dbus_interface(
  disman_SRCS ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.backend.xml backendinterface
)

ecm_qt_declare_logging_category(disman_SRCS
  HEADER disman_debug.h
  IDENTIFIER DISMAN
  CATEGORY_NAME disman
)

add_library(Disman SHARED ${disman_SRCS})
add_library(Disman::Disman ALIAS Disman)

generate_export_header(Disman)
target_compile_features(Disman
    PUBLIC cxx_std_11
    PRIVATE cxx_std_17
)

target_link_libraries(Disman
  PUBLIC
    Qt5::Core
    Qt5::Gui
  PRIVATE
    Qt5::DBus
)

set_target_properties(Disman PROPERTIES
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${Disman_SOVERSION}"
  EXPORT_NAME Disman
)

target_include_directories(Disman
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
    $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
)

install(
  TARGETS Disman
  EXPORT DismanTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
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
  DESTINATION ${INCLUDE_INSTALL_DIR}/disman
  COMPONENT Devel
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/disman.pc.in ${CMAKE_CURRENT_BINARY_DIR}/disman.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/disman.pc DESTINATION ${KDE_INSTALL_LIBDIR}/pkgconfig)
