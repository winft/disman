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

qt5_add_dbus_interface(backend_SRCS
  org.freedesktop.DBus.Properties.xml
  freedesktop_interface
)

add_library(backend OBJECT ${backend_SRCS})
add_library(disman::backend ALIAS backend)

target_link_libraries(backend
  PRIVATE Qt5::DBus
  PUBLIC Disman::Disman
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

if(${XCB_RANDR_FOUND})
  message(STATUS "Will build xrandr backend.")
  add_subdirectory(xrandr)
else()
  message(STATUS "Not building xrandr backend, no XCB_RANDR_FOUND set.")
endif()
