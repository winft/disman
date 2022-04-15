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

add_library(disman-kwinft MODULE ${kwinft_SRCS})
set_target_properties(disman-kwinft PROPERTIES
  OUTPUT_NAME kwinft
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman/wayland/"
)
target_compile_features(disman-kwinft PRIVATE cxx_std_17)

target_link_libraries(disman-kwinft
  PRIVATE
    disman::wllib
    Qt5::Core
    WraplandClient
)

install(
  TARGETS disman-kwinft
  DESTINATION ${PLUGIN_INSTALL_DIR}/disman/wayland/
  COMPONENT disman-kwinft
)
