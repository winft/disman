set(wlroots_SRCS
    wlroots_interface.cpp
    wlroots_output.cpp
)

ecm_qt_declare_logging_category(
    wlroots_SRCS
    HEADER wlroots_logging.h
    IDENTIFIER DISMAN_WAYLAND
    CATEGORY_NAME org.kwinft.disman.wayland
)

add_library(wlroots-wrapland MODULE ${wlroots_SRCS})

set_target_properties(wlroots-wrapland
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin/disman/wayland/"
)

target_link_libraries(wlroots-wrapland
    Qt5::Core
    DismanWayland
    WraplandClient
)

install(
    TARGETS wlroots-wrapland
    DESTINATION ${PLUGIN_INSTALL_DIR}/disman/wayland/
)
