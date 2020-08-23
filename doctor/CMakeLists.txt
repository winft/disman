add_executable(disman-doctor main.cpp doctor.cpp dpmsclient.cpp)
target_link_libraries(disman-doctor Qt5::DBus Disman KF5::WaylandClient)
install(TARGETS disman-doctor ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
