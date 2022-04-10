set(wlroots_SRCS
    wlroots_interface.cpp
    wlroots_output.cpp
)

ecm_qt_declare_logging_category(
    wlroots_SRCS
    HEADER wlroots_logging.h
    IDENTIFIER DISMAN_WAYLAND
    CATEGORY_NAME disman.wayland.wlroots
)

add_library(disman-wlroots MODULE ${wlroots_SRCS})
set_target_properties(disman-wlroots PROPERTIES
  OUTPUT_NAME wlroots
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman/wayland/"
)

target_compile_features(disman-wlroots PRIVATE cxx_std_17)
target_link_libraries(disman-wlroots
  PRIVATE
    disman::wllib
    Qt5::Core
    WraplandClient
)

install(
    TARGETS disman-wlroots
    DESTINATION ${PLUGIN_INSTALL_DIR}/disman/wayland/
)
