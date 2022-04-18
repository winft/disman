set(qscreen_SRCS
  qscreenbackend.cpp
  qscreenconfig.cpp
  qscreenscreen.cpp
  qscreenoutput.cpp
)

ecm_qt_declare_logging_category(qscreen_SRCS
  HEADER qscreen_logging.h
  IDENTIFIER DISMAN_QSCREEN
  CATEGORY_NAME disman.backend.qscreen
)

add_library(qscreen MODULE ${qscreen_SRCS})
set_target_properties(qscreen PROPERTIES
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman/"
)
target_compile_features(qscreen PRIVATE cxx_std_17)

target_link_libraries(qscreen
  PRIVATE disman::backend
)

install(TARGETS qscreen DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/)
