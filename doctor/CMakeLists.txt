add_executable(disman-doctor main.cpp doctor.cpp dpmsclient.cpp)
target_link_libraries(disman-doctor Qt5::DBus Disman KF5::WaylandClient)
install(
  TARGETS disman-doctor
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
