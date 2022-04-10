find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
  X11Extras
)

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

add_library(randr MODULE ${xrandr_SRCS})

set_target_properties(randr PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman")
set_target_properties(randr PROPERTIES PREFIX "")
target_compile_features(randr PRIVATE cxx_std_17)

target_link_libraries(randr
  PRIVATE
    disman::backend
    Qt5::Core
    Qt5::Gui
    Qt5::X11Extras
    ${XCB_LIBRARIES}
)

install(TARGETS randr DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/)
