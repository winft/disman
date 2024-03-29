set(fake_SRCS
  fake.cpp
  parser.cpp
)

ecm_qt_declare_logging_category(fake_SRCS
  HEADER fake_logging.h
  IDENTIFIER DISMAN_FAKE
  CATEGORY_NAME disman.backend.fake
)

qt6_add_dbus_adaptor(fake_SRCS
  ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.fakebackend.xml
  fake.h
  Fake
)

add_library(fake MODULE ${fake_SRCS})
set_target_properties(fake PROPERTIES
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman/"
)
target_compile_features(fake PRIVATE cxx_std_17)

target_link_libraries(fake
  PRIVATE
    disman::backend
    Qt6::DBus
)

install(TARGETS fake DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/)
