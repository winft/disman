add_executable(dismanctl main.cpp doctor.cpp)

target_link_libraries(dismanctl Qt5::DBus Disman::Disman)

install(
  TARGETS dismanctl
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
