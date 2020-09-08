####################################################################################################
# Internal library for backends
####################################################################################################
set(backend_SRCS
  backend_impl.cpp
  edid.cpp
  filer_controller.cpp
  logging.cpp
  utils.cpp
)

add_library(DismanBackend OBJECT ${backend_SRCS})
add_library(Disman::Backend ALIAS DismanBackend)

target_link_libraries(DismanBackend
  PUBLIC Disman::Disman
)

target_compile_features(DismanBackend PUBLIC cxx_std_17)

target_include_directories(DismanBackend
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
