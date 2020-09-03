add_executable(dismanctl main.cpp doctor.cpp dpmsclient.cpp)
target_link_libraries(dismanctl Qt5::DBus Disman::Disman KF5::WaylandClient)
install(
  TARGETS dismanctl
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
