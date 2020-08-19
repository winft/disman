set(kwinft_SRCS
    kwinft_interface.cpp
    kwinft_output.cpp
)

ecm_qt_declare_logging_category(
    kwinft_SRCS
    HEADER kwinft_logging.h
    IDENTIFIER DISMAN_WAYLAND
    CATEGORY_NAME disman.wayland.kwinft
)

add_library(kwinft MODULE ${kwinft_SRCS})

set_target_properties(kwinft
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin/disman/wayland/"
)

target_link_libraries(kwinft
    Qt5::Core
    DismanWayland
    WraplandClient
)

install(
    TARGETS kwinft
    DESTINATION ${PLUGIN_INSTALL_DIR}/disman/wayland/
)
