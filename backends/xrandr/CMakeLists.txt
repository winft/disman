set(xrandr_SRCS
  xrandr.cpp
  xrandrconfig.cpp
  xrandrcrtc.cpp
  xrandroutput.cpp
  xrandrmode.cpp
  xrandrscreen.cpp
  xcbwrapper.cpp
  xcbeventlistener.cpp
)

ecm_qt_declare_logging_category(xrandr_SRCS
  HEADER xrandr_logging.h
  IDENTIFIER DISMAN_XRANDR
  CATEGORY_NAME disman.backend.xrandr
)

add_library(disman-randr MODULE ${xrandr_SRCS})
set_target_properties(disman-randr PROPERTIES
  OUTPUT_NAME randr
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman/"
)
target_compile_features(disman-randr PRIVATE cxx_std_17)

target_link_libraries(disman-randr
  PRIVATE
    disman::backend
    Qt5::X11Extras
    XCB::RANDR
)

install(
  TARGETS disman-randr
  DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/
  COMPONENT disman-randr
)
