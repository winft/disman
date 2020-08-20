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

add_library(kwayland MODULE ${kwayland_SRCS})

set_target_properties(kwayland
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin/disman/wayland/"
)

target_link_libraries(kwayland
    Qt5::Core
    DismanWayland
    KF5::WaylandClient
)

install(
    TARGETS kwayland
    DESTINATION ${PLUGIN_INSTALL_DIR}/disman/wayland/
)
