set(kwayland_SRCS
    kwayland_interface.cpp
    kwayland_output.cpp
    kwayland_logging.cpp
)

ecm_qt_declare_logging_category(
    kwayland_SRCS
    HEADER kwayland_logging.h
    IDENTIFIER DISMAN_WAYLAND
    CATEGORY_NAME disman.wayland.kwayland
)

add_library(disman-kwayland MODULE ${kwayland_SRCS})
set_target_properties(disman-kwayland PROPERTIES
  OUTPUT_NAME kwayland
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman/wayland/"
)
target_compile_features(disman-kwayland PRIVATE cxx_std_17)

target_link_libraries(disman-kwayland
  PRIVATE
    disman::wllib
    KF5::WaylandClient
)

install(
  TARGETS disman-kwayland
  DESTINATION ${PLUGIN_INSTALL_DIR}/disman/wayland/
  COMPONENT disman-kwayland
)
