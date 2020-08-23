set(fake_SRCS
  fake.cpp
  parser.cpp
  ../filer_controller.cpp
  ../logging.cpp
)

ecm_qt_declare_logging_category(fake_SRCS
  HEADER fake_logging.h
  IDENTIFIER DISMAN_FAKE
  CATEGORY_NAME disman.backend.fake
)

qt5_add_dbus_adaptor(fake_SRCS
  ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.fakebackend.xml
  fake.h
  Fake
)

add_library(fake MODULE ${fake_SRCS})

set_target_properties(fake PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman")
set_target_properties(fake PROPERTIES PREFIX "")
target_link_libraries(fake Qt5::Core Qt5::DBus Disman::Disman)

install(TARGETS fake DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/)
