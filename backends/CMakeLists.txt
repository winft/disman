####################################################################################################
# Internal library for backends
####################################################################################################
set(backend_SRCS
  backend_impl.cpp
  device.cpp
  edid.cpp
  filer_controller.cpp
  logging.cpp
  utils.cpp
)

qt6_add_dbus_interface(backend_SRCS
  org.freedesktop.DBus.Properties.xml
  freedesktop_interface
)

add_library(backend OBJECT ${backend_SRCS})
add_library(disman::backend ALIAS backend)

target_link_libraries(backend
  PRIVATE Qt6::DBus
  PUBLIC disman::lib
)

set_target_properties(backend PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
target_compile_features(backend PUBLIC cxx_std_17)

target_include_directories(backend
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

####################################################################################################
# Plugins
####################################################################################################
add_subdirectory(fake)
add_subdirectory(qscreen)
add_subdirectory(wayland)
add_subdirectory(xrandr)
