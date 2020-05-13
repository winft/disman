include_directories(${CMAKE_SOURCE_DIR}/src
                    ${CMAKE_BUILD_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
)

set(fake_SRCS
    fake.cpp
    parser.cpp
)

qt5_add_dbus_adaptor(fake_SRCS ${CMAKE_SOURCE_DIR}/interfaces/org.kwinft.disman.fakebackend.xml
                     fake.h Fake
)

add_library(fake MODULE ${fake_SRCS})

set_target_properties(fake PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/disman")
set_target_properties(fake PROPERTIES PREFIX "")
target_link_libraries(fake
    Qt5::Core
    Qt5::DBus
    Disman
)

install(TARGETS fake DESTINATION ${KDE_INSTALL_PLUGINDIR}/disman/)


