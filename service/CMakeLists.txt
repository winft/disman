set(backendlauncher_SRCS
  main.cpp
  backendloader.cpp
  backenddbuswrapper.cpp
)

ecm_qt_declare_logging_category(backendlauncher_SRCS
  HEADER disman_backend_launcher_debug.h
  IDENTIFIER DISMAN_BACKEND_LAUNCHER
  CATEGORY_NAME disman.backendlauncher
)

qt5_add_dbus_adaptor(backendlauncher_SRCS
  ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.backend.xml
  backenddbuswrapper.h
  BackendDBusWrapper
  backendadaptor
  BackendAdaptor
)
qt5_add_dbus_adaptor(backendlauncher_SRCS
  ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.xml
  backendloader.h
  BackendLoader
  backendloaderadaptor
  BackendLoaderAdaptor
)

add_executable(disman-launcher ${backendlauncher_SRCS})
target_compile_features(disman-launcher PRIVATE cxx_std_17)

target_link_libraries(disman-launcher
  disman::lib
  Qt5::Core
  Qt5::Gui
  Qt5::DBus
)

install(
  TARGETS disman-launcher
  DESTINATION ${CMAKE_INSTALL_FULL_LIBEXECDIR}
  COMPONENT disman
)

configure_file(org.kwinft.disman.service.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/org.kwinft.disman.service
  @ONLY
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kwinft.disman.service
  DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
  COMPONENT disman
)
